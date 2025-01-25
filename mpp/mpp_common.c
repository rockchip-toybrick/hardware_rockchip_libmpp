// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *	Alpha Lin, alpha.lin@rock-chips.com
 *	Randy Li, randy.li@rock-chips.com
 *	Ding Wei, leo.ding@rock-chips.com
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/proc_fs.h>
#include <linux/pm_runtime.h>
#include <linux/poll.h>
#include <linux/regmap.h>
#include <linux/rwsem.h>
#include <linux/mfd/syscon.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/nospec.h>
#include <linux/clk-provider.h>
#include <linux/version.h>

#include <soc/rockchip/pm_domains.h>

#include "rk-mpp.h"

#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_iommu.h"
#include "mpp_osal.h"

/* input parmater structure for version 1 */
struct mpp_msg_v1 {
	__u32 cmd;
	__u32 flags;
	__u32 size;
	__u32 offset;
	__u64 data_ptr;
};

#define MPP_BAT_MSG_DONE		(0x00000001)

#if 0
static void mpp_attach_workqueue(struct mpp_dev *mpp,
				 struct mpp_taskqueue *queue);
#endif

static int
mpp_taskqueue_pop_pending(struct mpp_taskqueue *queue,
			  struct mpp_task *task)
{
	if (!task->session || !task->session->mpp)
		return -EINVAL;

	mutex_lock(&queue->pending_lock);
	list_del_init(&task->queue_link);
	mutex_unlock(&queue->pending_lock);
	kref_put(&task->ref, mpp_free_task);

	return 0;
}

static struct mpp_task *
mpp_session_get_idle_task(struct mpp_session *session)
{
	struct mpp_task *task, *n;
	u32 found = 0;

	mutex_lock(&session->pending_lock);
	list_for_each_entry_safe(task, n, &session->pending_list, pending_link) {
		if (task)
			mpp_debug_func(DEBUG_TASK_INFO, "task %d %#lx session %p chan %d find session %p\n",
				       task->task_index, task->state, task->session, task->session->chn_id, session);
		if (task && !test_bit(TASK_STATE_RUNNING, &task->state)) {
			found = 1;
			break;
		}
	}
	mutex_unlock(&session->pending_lock);

	if (!found)
		task = NULL;

	return task;
}

static bool
mpp_taskqueue_is_running(struct mpp_taskqueue *queue)
{
	unsigned long flags;
	bool flag;

	spin_lock_irqsave(&queue->running_lock, flags);
	flag = !list_empty(&queue->running_list);
	spin_unlock_irqrestore(&queue->running_lock, flags);

	return flag;
}

#if 0
int mpp_power_on(struct mpp_dev *mpp)
{
	if (!atomic_xchg(&mpp->power_enabled, 1)) {
		pm_runtime_get_sync(mpp->dev);
		mpp_pm_stay_awake(mpp->dev);

		if (mpp->hw_ops->clk_on)
			mpp->hw_ops->clk_on(mpp);
	}

	return 0;
}

int mpp_power_off(struct mpp_dev *mpp)
{
	if (mpp->always_on)
		return 0;
	if (atomic_xchg(&mpp->power_enabled, 0)) {
		if (mpp->hw_ops->clk_off)
			mpp->hw_ops->clk_off(mpp);

		mpp_pm_relax(mpp->dev);
		if (mpp_taskqueue_get_pending_task(mpp->queue) ||
		    mpp_taskqueue_get_running_task(mpp->queue)) {
			pm_runtime_mark_last_busy(mpp->dev);
			pm_runtime_put_autosuspend(mpp->dev);
		} else
			pm_runtime_put_sync_suspend(mpp->dev);
	}

	return 0;
}
#endif

static int mpp_session_clear(struct mpp_dev *mpp,
			     struct mpp_session *session)
{
	struct mpp_task *task = NULL, *n;

	/* clear session pending list */
	mutex_lock(&session->pending_lock);
	list_for_each_entry_safe(task, n, &session->pending_list, pending_link) {
		/* abort task in taskqueue */
		atomic_inc(&task->abort_request);
		list_del_init(&task->pending_link);
		kref_put(&task->ref, mpp_free_task);
	}
	mutex_unlock(&session->pending_lock);

	return 0;
}

static void mpp_session_clear_online_task(struct mpp_dev *mpp, struct mpp_session *session)
{
	struct mpp_task *mpp_task;

	while (1) {
		struct mpp_taskqueue *queue = mpp->queue;

		mpp_task = mpp_session_get_idle_task(session);

		if (!mpp_task)
			break;
		atomic_inc(&mpp_task->abort_request);
		mutex_lock(&queue->pending_lock);
		list_del_init(&mpp_task->queue_link);
		mutex_unlock(&queue->pending_lock);
		kref_put(&mpp_task->ref, mpp_free_task);
		mpp_session_pop_pending(session, mpp_task);
	}
}

static void mpp_session_deinit_default(struct mpp_session *session)
{
	if (session->mpp) {
		struct mpp_dev *mpp = session->mpp;

		if (mpp->dev_ops->free_session)
			mpp->dev_ops->free_session(session);

		mpp_session_clear(mpp, session);

	}

	if (session->srv) {
		struct mpp_service *srv = session->srv;

		mutex_lock(&srv->session_lock);
		list_del_init(&session->service_link);
		mutex_unlock(&srv->session_lock);
	}

	list_del_init(&session->session_link);
}

void mpp_session_clean_detach(struct mpp_taskqueue *queue)
{
	mutex_lock(&queue->session_lock);
	while (atomic_read(&queue->detach_count)) {
		struct mpp_session *session = NULL;

		session = list_first_entry_or_null(&queue->session_detach, struct mpp_session,
						   session_link);

		if (!session)
			break;

		if (session->online)
			mpp_session_clear_online_task(session->mpp, session);

		if (atomic_read(&session->task_count))
			break;

		if (session) {
			list_del_init(&session->session_link);
			atomic_dec(&queue->detach_count);
		}

		mutex_unlock(&queue->session_lock);

		if (session) {
			mpp_dbg_session("%s detach count %d\n", dev_name(session->mpp->dev),
							atomic_read(&queue->detach_count));
			mpp_session_deinit(session);
		}

		mutex_lock(&queue->session_lock);
	}
	mutex_unlock(&queue->session_lock);
}

static void mpp_session_attach_workqueue(struct mpp_session *session,
					 struct mpp_taskqueue *queue)
{
	mpp_dbg_session("session %p:%d attach\n", session, session->index);

	mutex_lock(&queue->session_lock);
	list_add_tail(&session->session_link, &queue->session_attach);
	mutex_unlock(&queue->session_lock);
}

static void mpp_session_detach_workqueue(struct mpp_session *session)
{
	struct mpp_taskqueue *queue;
	struct mpp_dev *mpp;

	if (!session->mpp || !session->mpp->queue)
		return;

	mpp_dbg_session("session %p:%d detach\n", session, session->index);
	mpp = session->mpp;
	queue = mpp->queue;

	mutex_lock(&queue->session_lock);
	list_del_init(&session->session_link);
	list_add_tail(&session->session_link, &queue->session_detach);
	atomic_inc(&queue->detach_count);
	mutex_unlock(&queue->session_lock);

	mpp_session_clean_detach(queue);
}

static int
mpp_session_push_pending(struct mpp_session *session,
			 struct mpp_task *task)
{
	kref_get(&task->ref);
	mutex_lock(&session->pending_lock);
	list_add_tail(&task->pending_link, &session->pending_list);
	mutex_unlock(&session->pending_lock);

	return 0;
}

static void mpp_task_timeout_work(struct work_struct *work_s)
{
	struct mpp_dev *mpp;
	struct mpp_session *session;
	struct mpp_task *task = container_of(to_delayed_work(work_s),
					     struct mpp_task,
					     timeout_work);

	u32 clbk_en = task->clbk_en;

	if (test_and_set_bit(TASK_STATE_HANDLE, &task->state)) {
		mpp_err("task has been handled\n");
		return;
	}

	mpp_err("task %d processing time out!\n", task->task_index);

	if (!task->session) {
		mpp_err("task %p, task->session is null.\n", task);
		return;
	}
	session = task->session;

	if (!session->mpp) {
		mpp_err("session %p, session->mpp is null.\n", session);
		return;
	}

	mpp = mpp_get_task_used_device(task, session);

	if (mpp && mpp->var->dev_ops->dump_dev)
		mpp->var->dev_ops->dump_dev(mpp);
	/* hardware maybe dead, reset it */
	mpp_reset_up_read(mpp->reset_group);
	mpp_dev_reset(mpp);
	mpp_power_off(mpp);

	set_bit(TASK_STATE_TIMEOUT, &task->state);
	set_bit(TASK_STATE_DONE, &task->state);
	/* Wake up the GET thread */
	wake_up(&task->wait);

	/* remove task from taskqueue running list */
	mpp_taskqueue_pop_running(mpp->queue, task);
	if (session->callback && clbk_en)
		session->callback(session->chn_id, MPP_VCODEC_EVENT_FRAME, NULL);

}

static int mpp_process_task_default(struct mpp_session *session,
				    struct mpp_task_msgs *msgs)
{
	struct mpp_task *task = NULL;
	struct mpp_dev *mpp = session->mpp;

	if (unlikely(!mpp)) {
		mpp_err("pid %d client %d found invalid process function\n",
			session->pid, session->device_type);
		return -EINVAL;
	}

	if (mpp->dev_ops->alloc_task)
		task = mpp->dev_ops->alloc_task(session, msgs);
	if (!task) {
		mpp_err("alloc_task failed.\n");
		return -ENOMEM;
	}
	/* ensure current device */
	mpp = mpp_get_task_used_device(task, session);

	kref_init(&task->ref);
	init_waitqueue_head(&task->wait);
	atomic_set(&task->abort_request, 0);
	task->task_index = atomic_fetch_inc(&mpp->task_index);
	INIT_DELAYED_WORK(&task->timeout_work, mpp_task_timeout_work);

	if (mpp->auto_freq_en && mpp->hw_ops->get_freq)
		mpp->hw_ops->get_freq(mpp, task);

	msgs->queue = mpp->queue;
	msgs->task = task;
	msgs->mpp = mpp;

	/*
	 * Push task to session should be in front of push task to queue.
	 * Otherwise, when mpp_task_finish finish and worker_thread call
	 * task worker, it may be get a task who has push in queue but
	 * not in session, cause some errors.
	 */
	atomic_inc(&session->task_count);
	mpp_session_push_pending(session, task);
	mpp_debug_func(DEBUG_TASK_INFO,
		       "session %d:%d task %d state 0x%lx\n",
		       session->device_type, session->index,
		       task->task_index, task->state);
	return 0;
}

static int mpp_process_task(struct mpp_session *session,
			    struct mpp_task_msgs *msgs)
{
	if (likely(session->process_task))
		return session->process_task(session, msgs);

	pr_err("invalid NULL process task function\n");
	return -EINVAL;
}

#if 0
struct reset_control *
mpp_reset_control_get(struct mpp_dev *mpp, enum MPP_RESET_TYPE type, const char *name)
{
	int index;
	struct reset_control *rst = NULL;
	char shared_name[32] = "shared_";
	struct mpp_reset_group *group;
	void *of_node = mpp_dev_of_node(mpp->dev);

	/* check reset whether belone to device alone */
	index = of_property_match_string(of_node, "reset-names", name);
	if (index >= 0) {
		rst = devm_reset_control_get(mpp->dev, name);
		mpp_safe_unreset(rst);

		return rst;
	}

	/* check reset whether is shared */
	strncat(shared_name, name,
		sizeof(shared_name) - strlen(shared_name) - 1);
	index = of_property_match_string(of_node,
					 "reset-names", shared_name);
	if (index < 0) {
		dev_err(mpp->dev, "%s is not found!\n", shared_name);
		return NULL;
	}

	if (!mpp->reset_group) {
		dev_err(mpp->dev, "reset group is empty!\n");
		return NULL;
	}
	group = mpp->reset_group;

	down_write(&group->rw_sem);
	rst = group->resets[type];
	if (!rst) {
		rst = devm_reset_control_get(mpp->dev, shared_name);
		mpp_safe_unreset(rst);
		group->resets[type] = rst;
		group->queue = mpp->queue;
	}
	/* if reset not in the same queue, it means different device
	 * may reset in the same time, then rw_sem_on should set true.
	 */
	group->rw_sem_on |= (group->queue != mpp->queue) ? true : false;
	dev_info(mpp->dev, "reset_group->rw_sem_on=%d\n", group->rw_sem_on);
	up_write(&group->rw_sem);

	return rst;
}

int mpp_dev_reset(struct mpp_dev *mpp)
{
	mpp_dbg_warning("resetting...\n");

	if (mpp->auto_freq_en && mpp->hw_ops->reduce_freq)
		mpp->hw_ops->reduce_freq(mpp);
	/* FIXME lock resource lock of the other devices in combo */
	mpp_reset_down_write(mpp->reset_group);
	atomic_set(&mpp->reset_request, 0);

	if (mpp->hw_ops->reset)
		mpp->hw_ops->reset(mpp);

	mpp_reset_up_write(mpp->reset_group);

	mpp_dbg_warning("reset done\n");

	return 0;
}

static int mpp_task_run(struct mpp_dev *mpp,
			struct mpp_task *task)
{
	struct mpp_session *session = task->session;

	mpp_debug_enter();

	mpp_power_on(mpp);
	mpp_time_record(task);
	mpp_debug_func(DEBUG_TASK_INFO,
		       "%s session %d:%d task=%d state=0x%lx\n",
		       dev_name(mpp->dev), session->device_type,
		       session->index, task->task_index, task->state);

	if (mpp->auto_freq_en && mpp->hw_ops->set_freq)
		mpp->hw_ops->set_freq(mpp, task);
	/*
	 * TODO: Lock the reader locker of the device resource lock here,
	 * release at the finish operation
	 */
	mpp_reset_down_read(mpp->reset_group);

	set_bit(TASK_STATE_START, &task->state);
	schedule_delayed_work(&task->timeout_work,
			      msecs_to_jiffies(MPP_WORK_TIMEOUT_DELAY));
	if (mpp->dev_ops->run)
		mpp->dev_ops->run(mpp, task);

	mpp_debug_leave();

	return 0;
}

static void mpp_task_worker_default(struct kthread_work *work_s)
{
	struct mpp_task *task;
	struct mpp_dev *mpp = container_of(work_s, struct mpp_dev, work);
	struct mpp_taskqueue *queue = mpp->queue;

	mpp_debug_enter();
	mutex_lock(&queue->dev_lock);
again:
	task = mpp_taskqueue_get_pending_task(queue);
	if (!task)
		goto done;

	/* if task timeout and aborted, remove it */
	if (atomic_read(&task->abort_request) > 0) {
		mpp_taskqueue_pop_pending(queue, task);
		goto again;
	}

	/* get device for current task */
	mpp = task->session->mpp;

	/*
	 * In the link table mode, the prepare function of the device
	 * will check whether I can insert a new task into device.
	 * If the device supports the task status query(like the HEVC
	 * encoder), it can report whether the device is busy.
	 * If the device does not support multiple task or task status
	 * query, leave this job to mpp service.
	 */
	if (mpp->dev_ops->prepare)
		task = mpp->dev_ops->prepare(mpp, task);
	else if (mpp_taskqueue_is_running(queue))
		task = NULL;

	/*
	 * FIXME if the hardware supports task query, but we still need to lock
	 * the running list and lock the mpp service in the current state.
	 */
	/* Push a pending task to running queue */
	if (task) {
		struct mpp_dev *task_mpp = mpp_get_task_used_device(task, task->session);
		mpp_taskqueue_pending_to_run(queue, task);
		set_bit(TASK_STATE_RUNNING, &task->state);
		if (mpp_task_run(task_mpp, task))
			mpp_taskqueue_pop_running(queue, task);
		else
			goto again;
	}

done:
	mpp_session_clean_detach(queue);
	mutex_unlock(&queue->dev_lock);
}
#endif

int mpp_chnl_is_running(struct mpp_session *session)
{
	struct mpp_dev *mpp = session->mpp;
	struct mpp_taskqueue *queue = mpp->queue;
	struct mpp_task *task, *n;
	int running = 0;

	mutex_lock(&queue->dev_lock);
	mutex_lock(&queue->pending_lock);

	list_for_each_entry_safe(task, n, &session->pending_list, pending_link) {
		if (test_bit(TASK_STATE_RUNNING, &task->state)) {
			running = 1;
			break;
		}
	}

	mutex_unlock(&queue->pending_lock);
	mutex_unlock(&queue->dev_lock);

	return running;
}

int mpp_chnl_run_task(struct mpp_session *session, void *param)
{
	struct mpp_dev *mpp = session->mpp;
	struct mpp_taskqueue *queue = mpp->queue;
	struct mpp_task *task = NULL;
	int ret = 0;
	struct mpp_task_info *info = (struct mpp_task_info *)param;

	mutex_lock(&queue->dev_lock);
	mpp_debug_func(DEBUG_TASK_INFO, "chan_id %d ++\n", session->chn_id);

again:
	task = mpp_session_get_idle_task(session);
	if (!task) {
		ret = -EAGAIN;
		goto done;
	}

	/* when resolution switch case,
	 * need drop task if resolution mismatch.
	 */
	if (info) {
		if ((ALIGN(info->width, 8) != task->width) ||
		    (ALIGN(info->height, 8) != task->height)) {
			mpp_dbg_warning("chan %d task %d pipe %d frame %d wxh [%d %d] != [%d %d]\n",
					session->chn_id, task->task_index,
					info->pipe_id, info->frame_id,
					info->width, info->height,
					task->width, task->height);
			atomic_inc(&task->abort_request);
			set_bit(TASK_STATE_DONE, &task->state);
			if (session->callback)
				session->callback(session->chn_id, MPP_VCODEC_EVENT_FRAME, NULL);
		}
	}

	/* if task timeout and aborted, remove it */
	if (atomic_read(&task->abort_request) > 0) {
		mpp_taskqueue_pop_pending(queue, task);
		goto again;
	}

	/*
	 * In the link table mode, the prepare function of the device
	 * will check whether I can insert a new task into device.
	 * If the device supports the task status query(like the HEVC
	 * encoder), it can report whether the device is busy.
	 * If the device does not support multiple task or task status
	 * query, leave this job to mpp service.
	 */
	if (mpp->dev_ops->prepare)
		task = mpp->dev_ops->prepare(mpp, task);
	else if (mpp_taskqueue_is_running(queue))
		task = NULL;

	if (!task) {
		ret = -EBUSY;
		mpp_debug_func(DEBUG_TASK_INFO, "chan_id %d device is busy now\n",
			       session->chn_id);
		goto done;
	}

	if (info) {
		task->pipe_id = info->pipe_id;
		task->frame_id = info->frame_id;
	}

	if (atomic_read(&mpp->suspend_en))
		goto done;
	down(&mpp->work_sem);

	/* run task */
	mpp = mpp_get_task_used_device(task, task->session);
	mpp_taskqueue_pending_to_run(queue, task);
	mpp_reset_down_read(mpp->reset_group);
	mpp_time_record(task);
	if (mpp->dev_ops->run)
		mpp->dev_ops->run(mpp, task);
	set_bit(TASK_STATE_START, &task->state);

done:
	mpp_session_clean_detach(queue);

	mpp_debug_func(DEBUG_TASK_INFO, "chan_id %d --\n", session->chn_id);
	mutex_unlock(&queue->dev_lock);
	return ret;
}

static int mpp_wait_result_default(struct mpp_session *session,
				   struct mpp_task_msgs *msgs)
{
	int ret;
	struct mpp_task *task;
	struct mpp_dev *mpp;

	task = mpp_session_get_pending_task(session);
	if (!task) {
		mpp_err("session %p pending list is empty!\n", session);
		return -EIO;
	}
	mpp = mpp_get_task_used_device(task, session);

	ret = wait_event_interruptible(task->wait, test_bit(TASK_STATE_DONE, &task->state));
	if (ret == -ERESTARTSYS)
		mpp_err("wait task break by signal\n");

	if (mpp->dev_ops->result)
		ret = mpp->dev_ops->result(mpp, task, msgs);
	mpp_debug_func(DEBUG_TASK_INFO,
		       "session %d:%d task %d state 0x%lx kref_rd %d ret %d\n",
		       session->device_type,
		       session->index, task->task_index, task->state,
		       kref_read(&task->ref), ret);

	mpp_session_pop_pending(session, task);

	return ret;
}

static int mpp_wait_result(struct mpp_session *session,
			   struct mpp_task_msgs *msgs)
{
	if (likely(session->wait_result))
		return session->wait_result(session, msgs);

	pr_err("invalid NULL wait result function\n");
	return -EINVAL;
}

#if 0
static int mpp_attach_service(struct mpp_dev *mpp, struct device *dev)
{
	u32 taskqueue_node = 0;
	u32 reset_group_node = 0;
	struct device_node *np = NULL;
	struct platform_device *pdev = NULL;
	struct mpp_taskqueue *queue = NULL;
	int ret = 0;

	np = of_parse_phandle(mpp_dev_of_node(dev), "rockchip,srv", 0);
	if (!np || !of_device_is_available(np)) {
		dev_err(dev, "failed to get the mpp service node\n");
		return -ENODEV;
	}

	pdev = of_find_device_by_node(np);
	of_node_put(np);
	if (!pdev) {
		dev_err(dev, "failed to get mpp service from node\n");
		return -ENODEV;
	}

	mpp->srv = platform_get_drvdata(pdev);
	platform_device_put(pdev);
	if (!mpp->srv) {
		dev_err(dev, "failed attach service\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(mpp_dev_of_node(dev),
				   "rockchip,taskqueue-node", &taskqueue_node);
	if (ret) {
		dev_err(dev, "failed to get taskqueue-node\n");
		return ret;
	} else if (taskqueue_node >= mpp->srv->taskqueue_cnt) {
		dev_err(dev, "taskqueue-node %d must less than %d\n",
			taskqueue_node, mpp->srv->taskqueue_cnt);
		return -ENODEV;
	}
	/* set taskqueue according dtsi */
	queue = mpp->srv->task_queues[taskqueue_node];
	if (!queue) {
		dev_err(dev, "taskqueue attach to invalid node %d\n",
			taskqueue_node);
		return -ENODEV;
	}
	mpp_attach_workqueue(mpp, queue);

	ret = of_property_read_u32(mpp_dev_of_node(dev),
				   "rockchip,resetgroup-node", &reset_group_node);
	if (!ret) {
		/* set resetgroup according dtsi */
		if (reset_group_node >= mpp->srv->reset_group_cnt) {
			dev_err(dev, "resetgroup-node %d must less than %d\n",
				reset_group_node, mpp->srv->reset_group_cnt);
			return -ENODEV;
		} else
			mpp->reset_group = mpp->srv->reset_groups[reset_group_node];
	}

	return 0;
}

struct mpp_taskqueue *mpp_taskqueue_init(struct device *dev)
{
	struct mpp_taskqueue *queue = devm_kzalloc(dev, sizeof(*queue),
						   GFP_KERNEL);
	if (!queue)
		return NULL;

	mutex_init(&queue->session_lock);
	mutex_init(&queue->pending_lock);
	spin_lock_init(&queue->running_lock);
	mutex_init(&queue->dev_lock);
	INIT_LIST_HEAD(&queue->session_attach);
	INIT_LIST_HEAD(&queue->session_detach);
	INIT_LIST_HEAD(&queue->pending_list);
	INIT_LIST_HEAD(&queue->running_list);
	INIT_LIST_HEAD(&queue->dev_list);

	/* default taskqueue has max 16 task capacity */
	queue->task_capacity = MPP_MAX_TASK_CAPACITY;
	atomic_set(&queue->reset_request, 0);

	return queue;
}

static void mpp_attach_workqueue(struct mpp_dev *mpp,
				 struct mpp_taskqueue *queue)
{
	s32 core_id;

	INIT_LIST_HEAD(&mpp->queue_link);

	mutex_lock(&queue->dev_lock);

	if (mpp->core_id >= 0)
		core_id = mpp->core_id;
	else
		core_id = queue->core_count;

	if (core_id < 0 || core_id >= MPP_MAX_CORE_NUM) {
		dev_err(mpp->dev, "invalid core id %d\n", core_id);
		goto done;
	}

	if (queue->cores[core_id]) {
		dev_err(mpp->dev, "can not attach device with same id %d", core_id);
		goto done;
	}

	queue->cores[core_id] = mpp;
	queue->core_count++;

	set_bit(core_id, &queue->core_idle);
	list_add_tail(&mpp->queue_link, &queue->dev_list);

	mpp->core_id = core_id;
	mpp->queue = queue;

	mpp_dbg_core("%s attach queue as core %d\n",
		     dev_name(mpp->dev), mpp->core_id);

	if (queue->task_capacity > mpp->task_capacity)
		queue->task_capacity = mpp->task_capacity;

done:
	mutex_unlock(&queue->dev_lock);
}

static void mpp_detach_workqueue(struct mpp_dev *mpp)
{
	struct mpp_taskqueue *queue = mpp->queue;

	if (queue) {
		mutex_lock(&queue->dev_lock);

		queue->cores[mpp->core_id] = NULL;
		queue->core_count--;

		clear_bit(queue->core_count, &queue->core_idle);
		list_del_init(&mpp->queue_link);

		mpp->queue = NULL;

		mutex_unlock(&queue->dev_lock);
	}
}
#endif

static int mpp_check_cmd_v1(__u32 cmd)
{
	bool found;

	found = (cmd < MPP_CMD_QUERY_BUTT) ? true : false;
	found = (cmd >= MPP_CMD_INIT_BASE && cmd < MPP_CMD_INIT_BUTT) ? true : found;
	found = (cmd >= MPP_CMD_SEND_BASE && cmd < MPP_CMD_SEND_BUTT) ? true : found;
	found = (cmd >= MPP_CMD_POLL_BASE && cmd < MPP_CMD_POLL_BUTT) ? true : found;
	found = (cmd >= MPP_CMD_CONTROL_BASE && cmd < MPP_CMD_CONTROL_BUTT) ? true : found;

	return found ? 0 : -EINVAL;
}

static int mpp_parse_msg_v1(struct mpp_msg_v1 *msg,
			    struct mpp_request *req)
{
	int ret = 0;

	req->cmd = msg->cmd;
	req->flags = msg->flags;
	req->size = msg->size;
	req->offset = msg->offset;
	req->data = (void *)(unsigned long)msg->data_ptr;

	mpp_debug(DEBUG_IOCTL, "cmd %x, flags %08x, size %d, offset %x\n",
		  req->cmd, req->flags, req->size, req->offset);

	ret = mpp_check_cmd_v1(req->cmd);
	if (ret)
		mpp_err("mpp cmd %x is not supproted.\n", req->cmd);

	return ret;
}

static inline int mpp_msg_is_last(struct mpp_request *req)
{
	int flag;

	if (req->flags & MPP_FLAGS_MULTI_MSG)
		flag = (req->flags & MPP_FLAGS_LAST_MSG) ? 1 : 0;
	else
		flag = 1;

	return flag;
}

static __u32 mpp_get_cmd_butt(__u32 cmd)
{
	__u32 mask = 0;

	switch (cmd) {
	case MPP_CMD_QUERY_BASE:
		mask = MPP_CMD_QUERY_BUTT;
		break;
	case MPP_CMD_INIT_BASE:
		mask = MPP_CMD_INIT_BUTT;
		break;

	case MPP_CMD_SEND_BASE:
		mask = MPP_CMD_SEND_BUTT;
		break;
	case MPP_CMD_POLL_BASE:
		mask = MPP_CMD_POLL_BUTT;
		break;
	case MPP_CMD_CONTROL_BASE:
		mask = MPP_CMD_CONTROL_BUTT;
		break;
	default:
		mpp_err("unknown dev cmd 0x%x\n", cmd);
		break;
	}

	return mask;
}

static int mpp_process_request(struct mpp_session *session,
			       struct mpp_service *srv,
			       struct mpp_request *req,
			       struct mpp_task_msgs *msgs)
{
	int ret;
	struct mpp_dev *mpp;

	mpp_debug(DEBUG_IOCTL, "cmd %x process\n", req->cmd);

	switch (req->cmd) {
	case MPP_CMD_QUERY_HW_SUPPORT: {
		u32 hw_support = srv->hw_support;

		mpp_debug(DEBUG_IOCTL, "hw_support %08x\n", hw_support);
		if (put_user(hw_support, (u32 __user *)req->data))
			return -EFAULT;
	} break;
	case MPP_CMD_QUERY_HW_ID: {
		struct mpp_hw_info *hw_info;

		mpp = NULL;
		if (session && session->mpp)
			mpp = session->mpp;

		else {
			u32 client_type;

			if (get_user(client_type, (u32 __user *)req->data))
				return -EFAULT;

			mpp_debug(DEBUG_IOCTL, "client %d\n", client_type);
			client_type = array_index_nospec(client_type, MPP_DEVICE_BUTT);
			if (test_bit(client_type, &srv->hw_support))
				mpp = srv->sub_devices[client_type];
		}

		if (!mpp)
			return -EINVAL;

		hw_info = mpp->var->hw_info;
		mpp_debug(DEBUG_IOCTL, "hw_id %08x\n", hw_info->hw_id);
		if (put_user(hw_info->hw_id, (u32 __user *)req->data))
			return -EFAULT;
	} break;
	case MPP_CMD_QUERY_CMD_SUPPORT: {
		__u32 cmd = 0;

		if (get_user(cmd, (u32 __user *)req->data))
			return -EINVAL;

		if (put_user(mpp_get_cmd_butt(cmd), (u32 __user *)req->data))
			return -EFAULT;
	} break;
	case MPP_CMD_INIT_CLIENT_TYPE: {
		u32 client_type;

		if (get_user(client_type, (u32 __user *)req->data))
			return -EFAULT;

		mpp_debug(DEBUG_IOCTL, "client %d\n", client_type);
		if (client_type >= MPP_DEVICE_BUTT) {
			mpp_err("client_type must less than %d\n",
				MPP_DEVICE_BUTT);
			return -EINVAL;
		}
		client_type = array_index_nospec(client_type, MPP_DEVICE_BUTT);
		mpp = srv->sub_devices[client_type];
		if (!mpp)
			return -EINVAL;

		session->device_type = (enum MPP_DEVICE_TYPE)client_type;
		session->mpp = mpp;
		if (mpp->dev_ops) {
			if (mpp->dev_ops->process_task)
				session->process_task =
					mpp->dev_ops->process_task;

			if (mpp->dev_ops->wait_result)
				session->wait_result =
					mpp->dev_ops->wait_result;

			if (mpp->dev_ops->deinit)
				session->deinit = mpp->dev_ops->deinit;
		}
		session->index = atomic_fetch_inc(&mpp->session_index);
		if (mpp->dev_ops && mpp->dev_ops->init_session) {
			ret = mpp->dev_ops->init_session(session);
			if (ret)
				return ret;
		}

		mpp_session_attach_workqueue(session, mpp->queue);
	} break;
	case MPP_CMD_INIT_DRIVER_DATA: {
		u32 val;

		mpp = session->mpp;
		if (!mpp)
			return -EINVAL;
		if (get_user(val, (u32 __user *)req->data))
			return -EFAULT;
	} break;
	case MPP_CMD_INIT_TRANS_TABLE: {
	} break;
	case MPP_CMD_SET_REG_WRITE:
	case MPP_CMD_SET_REG_READ:
	case MPP_CMD_SET_REG_ADDR_OFFSET:
	case MPP_CMD_SET_RCB_INFO: {
		msgs->flags |= req->flags;
		msgs->set_cnt++;
	} break;
	case MPP_CMD_POLL_HW_FINISH: {
		msgs->flags |= req->flags;
		msgs->poll_cnt++;
	} break;
	case MPP_CMD_RESET_SESSION: {
		int ret;
		int val;

		ret = readx_poll_timeout(atomic_read, &session->task_count,
					 val, val == 0, 1000, 500000);
		if (ret == -ETIMEDOUT) {
			mpp_err("wait task running time out\n");
			return ret;
		}

		mpp = session->mpp;
		if (!mpp)
			return -EINVAL;

		mpp_session_clear(mpp, session);

		return ret;
	} break;
	case MPP_CMD_VEPU_SET_ONLINE_MODE: {
		u32 mode = *(u32*)req->data;

		session->online = mode;
		mpp = session->mpp;
		mpp->online_mode = mode;
		mpp_err("set online mode %d\n", mode);
	} break;
	default: {
		mpp = session->mpp;
		if (!mpp) {
			mpp_err("pid %d not find client %d\n",
				session->pid, session->device_type);
			return -EINVAL;
		}
		if (mpp->dev_ops->ioctl)
			return mpp->dev_ops->ioctl(session, req);

		mpp_debug(DEBUG_IOCTL, "unknown mpp ioctl cmd %x\n", req->cmd);
	} break;
	}

	return 0;
}

#if 0
static long mpp_dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	(void)filp;
	(void)cmd;
	(void)arg;
	return 0;
}

static int mpp_dev_open(struct inode *inode, struct file *filp)
{
	struct mpp_session *session = NULL;
	struct mpp_service *srv = container_of(inode->i_cdev,
					       struct mpp_service,
					       mpp_cdev);
	mpp_debug_enter();

	session = mpp_session_init();
	if (!session)
		return -ENOMEM;

	session->srv = srv;

	if (session->srv) {
		mutex_lock(&srv->session_lock);
		list_add_tail(&session->service_link, &srv->session_list);
		mutex_unlock(&srv->session_lock);
	}
	session->process_task = mpp_process_task_default;
	session->wait_result = mpp_wait_result_default;
	session->deinit = mpp_session_deinit_default;
	filp->private_data = (void *)session;

	mpp_debug_leave();

	return nonseekable_open(inode, filp);
}

static int mpp_dev_release(struct inode *inode, struct file *filp)
{
	struct mpp_session *session = filp->private_data;

	mpp_debug_enter();

	if (!session) {
		mpp_err("session is null\n");
		return -EINVAL;
	}

	/* wait for task all done */
	atomic_inc(&session->release_request);

	if (session->mpp)
		mpp_session_detach_workqueue(session);
	else
		mpp_session_deinit(session);

	filp->private_data = NULL;

	mpp_debug_leave();
	return 0;
}

const struct file_operations rockchip_mpp_fops = {
	.open		= mpp_dev_open,
	.release	= mpp_dev_release,
	.unlocked_ioctl = mpp_dev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = mpp_dev_ioctl,
#endif
};

int mpp_check_req(struct mpp_request *req, int base,
		  int max_size, u32 off_s, u32 off_e)
{
	int req_off;

	if (req->offset < base) {
		mpp_err("error: base %x, offset %x\n",
			base, req->offset);
		return -EINVAL;
	}
	req_off = req->offset - base;
	if ((req_off + req->size) < off_s) {
		mpp_err("error: req_off %x, req_size %x, off_s %x\n",
			req_off, req->size, off_s);
		return -EINVAL;
	}
	if (max_size < off_e) {
		mpp_err("error: off_e %x, max_size %x\n",
			off_e, max_size);
		return -EINVAL;
	}
	if (req_off > max_size) {
		mpp_err("error: req_off %x, max_size %x\n",
			req_off, max_size);
		return -EINVAL;
	}
	if ((req_off + req->size) > max_size) {
		mpp_err("error: req_off %x, req_size %x, max_size %x\n",
			req_off, req->size, max_size);
		req->size = req_off + req->size - max_size;
	}

	return 0;
}

int mpp_task_init(struct mpp_session *session, struct mpp_task *task)
{
	INIT_LIST_HEAD(&task->pending_link);
	INIT_LIST_HEAD(&task->queue_link);
	task->state = 0;
	task->session = session;

	return 0;
}

int mpp_task_finish(struct mpp_session *session,
		    struct mpp_task *task)
{
	struct mpp_dev *mpp = mpp_get_task_used_device(task, session);
	u32 clbk_en = task->clbk_en;
	if (mpp->dev_ops->finish)
		mpp->dev_ops->finish(mpp, task);

	mpp_reset_up_read(mpp->reset_group);
	if (atomic_read(&mpp->reset_request) > 0)
		mpp_dev_reset(mpp);
	mpp_power_off(mpp);

	set_bit(TASK_STATE_FINISH, &task->state);
	set_bit(TASK_STATE_DONE, &task->state);
	/* Wake up the GET thread */
	wake_up(&task->wait);
	mpp_taskqueue_pop_running(mpp->queue, task);

	if (session->callback && clbk_en)
		session->callback(session->chn_id, MPP_VCODEC_EVENT_FRAME, NULL);

	return 0;
}

int mpp_task_finalize(struct mpp_session *session,
		      struct mpp_task *task)
{
	task->state = 0;
	return 0;
}

/* The device will do more probing work after this */
int mpp_dev_probe(struct mpp_dev *mpp,
		  struct platform_device *pdev)
{
	int ret;
	struct resource *res = NULL;
	struct device *dev = &pdev->dev;
	struct mpp_hw_info *hw_info = mpp->var->hw_info;

	mpp->auto_freq_en = 1;
	mpp->skip_idle = 0;
	mpp->task_capacity = 1;
	mpp->dev = dev;
	mpp->hw_ops = mpp->var->hw_ops;
	mpp->dev_ops = mpp->var->dev_ops;
	atomic_set(&mpp->power_enabled, 0);
	mpp->always_on = 0;
	atomic_set(&mpp->suspend_en, 0);
	sema_init(&mpp->work_sem, 1);

	/* Get and attach to service */
	ret = mpp_attach_service(mpp, dev);
	if (ret) {
		dev_err(dev, "failed to attach service\n");
		return -ENODEV;
	}

	if (mpp->task_capacity == 1) {
		/* power domain autosuspend delay 2s */
		pm_runtime_set_autosuspend_delay(dev, 2000);
		pm_runtime_use_autosuspend(dev);
	} else {
		dev_info(dev, "link mode task capacity %d\n",
			 mpp->task_capacity);
		/* do not setup autosuspend on multi task device */
	}

	kthread_init_work(&mpp->work, mpp_task_worker_default);

	atomic_set(&mpp->reset_request, 0);
	atomic_set(&mpp->session_index, 0);
	atomic_set(&mpp->task_count, 0);
	atomic_set(&mpp->task_index, 0);

	mpp_device_init_wakeup(dev, true);
	pm_runtime_enable(dev);

	mpp->irq = platform_get_irq(pdev, 0);
	if (mpp->irq < 0) {
		dev_err(dev, "No interrupt resource found\n");
		ret = -ENODEV;
		goto failed;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no memory resource defined\n");
		ret = -ENODEV;
		goto failed;
	}
	/*
	 * Tips: here can not use function devm_ioremap_resource. The resion is
	 * that hevc and vdpu map the same register address region in rk3368.
	 * However, devm_ioremap_resource will call function
	 * devm_request_mem_region to check region. Thus, use function
	 * devm_ioremap can avoid it.
	 */
	mpp->reg_base = devm_ioremap(dev, res->start, resource_size(res));
	if (!mpp->reg_base) {
		dev_err(dev, "ioremap failed for resource %pR\n", res);
		ret = -ENOMEM;
		goto failed;
	}

	pm_runtime_get_sync(dev);

	if (mpp->hw_ops->init) {
		ret = mpp->hw_ops->init(mpp);
		if (ret)
			goto failed_init;
	}

	/* read hardware id */
	if (hw_info->reg_id >= 0) {
		if (mpp->hw_ops->clk_on)
			mpp->hw_ops->clk_on(mpp);

		hw_info->hw_id = mpp_read(mpp, hw_info->reg_id);
		if (mpp->hw_ops->clk_off)
			mpp->hw_ops->clk_off(mpp);
	}

	pm_runtime_put_sync(dev);

	return ret;
failed_init:
	pm_runtime_put_sync(dev);
failed:
	mpp_detach_workqueue(mpp);
	mpp_device_init_wakeup(dev, false);
	pm_runtime_disable(dev);

	return ret;
}

int mpp_dev_remove(struct mpp_dev *mpp)
{
	if (mpp->hw_ops->exit)
		mpp->hw_ops->exit(mpp);

	mpp_detach_workqueue(mpp);
	mpp_device_init_wakeup(mpp->dev, false);
	pm_runtime_disable(mpp->dev);

	return 0;
}

void mpp_dev_shutdown(struct platform_device *pdev)
{
	int ret;
	int val;
	struct device *dev = &pdev->dev;
	struct mpp_dev *mpp = dev_get_drvdata(dev);

	dev_info(dev, "shutdown device\n");

	atomic_inc(&mpp->srv->shutdown_request);
	ret = readx_poll_timeout(atomic_read,
				 &mpp->task_count,
				 val, val == 0, 20000, 200000);
	if (ret == -ETIMEDOUT)
		dev_err(dev, "wait total %d running time out\n",
			atomic_read(&mpp->task_count));
	else
		dev_info(dev, "shutdown success\n");
}

int mpp_dev_register_srv(struct mpp_dev *mpp, struct mpp_service *srv)
{
	enum MPP_DEVICE_TYPE device_type = mpp->var->device_type;

	srv->sub_devices[device_type] = mpp;
	set_bit(device_type, &srv->hw_support);

	return 0;
}

irqreturn_t mpp_dev_irq(int irq, void *param)
{
	struct mpp_dev *mpp = param;
	struct mpp_task *task = mpp->cur_task;
	irqreturn_t irq_ret = IRQ_NONE;

	if (mpp->dev_ops->irq)
		irq_ret = mpp->dev_ops->irq(mpp);

	if (task) {
		if (irq_ret == IRQ_WAKE_THREAD) {
			/* if wait or delayed work timeout, abort request will turn on,
			 * isr should not to response, and handle it in delayed work
			 */
			if (test_and_set_bit(TASK_STATE_HANDLE, &task->state)) {
				mpp_err("error, task has been handled, irq_status %08x\n",
					mpp->irq_status);
				irq_ret = IRQ_HANDLED;
				goto done;
			}
			cancel_delayed_work(&task->timeout_work);
			/* normal condition, set state and wake up isr thread */
			set_bit(TASK_STATE_IRQ, &task->state);
		}
	} else
		mpp_debug(DEBUG_IRQ_CHECK, "error, task is null\n");
done:
	return irq_ret;
}

irqreturn_t mpp_dev_isr_sched(int irq, void *param)
{
	irqreturn_t ret = IRQ_NONE;
	struct mpp_dev *mpp = param;

	if (mpp->auto_freq_en &&
	    mpp->hw_ops->reduce_freq &&
	    list_empty(&mpp->queue->pending_list))
		mpp->hw_ops->reduce_freq(mpp);

	if (mpp->dev_ops->isr)
		ret = mpp->dev_ops->isr(mpp);

	/* trigger current queue to run next task */
	mpp_taskqueue_trigger_work(mpp);

	return ret;
}

int mpp_time_record(struct mpp_task *task)
{
	if (mpp_debug_unlikely(DEBUG_TIMING) && task) {
		task->start = ktime_get();
		task->part = task->start;
	}

	return 0;
}

int mpp_time_part_diff(struct mpp_task *task)
{
	if (mpp_debug_unlikely(DEBUG_TIMING)) {
		ktime_t end;
		struct mpp_dev *mpp = mpp_get_task_used_device(task, task->session);

		end = ktime_get();
		mpp_debug(DEBUG_PART_TIMING, "%s:%d session %d:%d part time: %lld us\n",
			dev_name(mpp->dev), task->core_id, task->session->pid,
			task->session->index, ktime_us_delta(end, task->part));
		task->part = end;
	}

	return 0;
}

int mpp_time_diff(struct mpp_task *task)
{
	if (mpp_debug_unlikely(DEBUG_TIMING)) {
		ktime_t end;
		struct mpp_dev *mpp = mpp_get_task_used_device(task, task->session);

		end = ktime_get();
		mpp_debug(DEBUG_TIMING, "%s:%d session %d:%d time: %lld us\n",
			dev_name(mpp->dev), task->core_id, task->session->pid,
			task->session->index, ktime_us_delta(end, task->start));
	}

	return 0;
}

int mpp_write_req(struct mpp_dev *mpp, u32 *regs,
		  u32 start_idx, u32 end_idx, u32 en_idx)
{
	int i;

	for (i = start_idx; i < end_idx; i++) {
		if (i == en_idx)
			continue;
		mpp_write_relaxed(mpp, i * sizeof(u32), regs[i]);
	}

	return 0;
}

int mpp_read_req(struct mpp_dev *mpp, u32 *regs,
		 u32 start_idx, u32 end_idx)
{
	int i;

	for (i = start_idx; i < end_idx; i++)
		regs[i] = mpp_read_relaxed(mpp, i * sizeof(u32));

	return 0;
}

int mpp_get_clk_info(struct mpp_dev *mpp,
		     struct mpp_clk_info *clk_info,
		     const char *name)
{
	void *of_node = mpp_dev_of_node(mpp->dev);
	int index = of_property_match_string(of_node,
					     "clock-names", name);

	if (index < 0)
		return -EINVAL;

	clk_info->clk = devm_clk_get(mpp->dev, name);
	of_property_read_u32_index(of_node,
				   "rockchip,normal-rates",
				   index,
				   &clk_info->normal_rate_hz);
	of_property_read_u32_index(of_node,
				   "rockchip,advanced-rates",
				   index,
				   &clk_info->advanced_rate_hz);

	return 0;
}

int mpp_set_clk_info_rate_hz(struct mpp_clk_info *clk_info,
			     enum MPP_CLOCK_MODE mode,
			     unsigned long val)
{
	if (!clk_info->clk || !val)
		return 0;

	switch (mode) {
	case CLK_MODE_DEBUG:
		clk_info->debug_rate_hz = val;
		break;
	case CLK_MODE_REDUCE:
		clk_info->reduce_rate_hz = val;
		break;
	case CLK_MODE_NORMAL:
		clk_info->normal_rate_hz = val;
		break;
	case CLK_MODE_ADVANCED:
		clk_info->advanced_rate_hz = val;
		break;
	case CLK_MODE_DEFAULT:
		clk_info->default_rate_hz = val;
		break;
	default:
		mpp_err("error mode %d\n", mode);
		break;
	}

	return 0;
}

#define MPP_REDUCE_RATE_HZ (50 * MHZ)

unsigned long mpp_get_clk_info_rate_hz(struct mpp_clk_info *clk_info,
				       enum MPP_CLOCK_MODE mode)
{
	unsigned long clk_rate_hz = 0;

	if (!clk_info->clk)
		return 0;

	if (clk_info->debug_rate_hz)
		return clk_info->debug_rate_hz;

	switch (mode) {
	case CLK_MODE_REDUCE: {
		if (clk_info->reduce_rate_hz)
			clk_rate_hz = clk_info->reduce_rate_hz;
		else
			clk_rate_hz = MPP_REDUCE_RATE_HZ;
	} break;
	case CLK_MODE_NORMAL: {
		if (clk_info->normal_rate_hz)
			clk_rate_hz = clk_info->normal_rate_hz;
		else
			clk_rate_hz = clk_info->default_rate_hz;
	} break;
	case CLK_MODE_ADVANCED: {
		if (clk_info->advanced_rate_hz)
			clk_rate_hz = clk_info->advanced_rate_hz;
		else if (clk_info->normal_rate_hz)
			clk_rate_hz = clk_info->normal_rate_hz;
		else
			clk_rate_hz = clk_info->default_rate_hz;
	} break;
	case CLK_MODE_DEFAULT:
	default: {
		clk_rate_hz = clk_info->default_rate_hz;
	} break;
	}

	return clk_rate_hz;
}

int mpp_clk_set_rate(struct mpp_clk_info *clk_info,
		     enum MPP_CLOCK_MODE mode)
{
	unsigned long clk_rate_hz;

	if (!clk_info->clk)
		return -EINVAL;

	clk_rate_hz = mpp_get_clk_info_rate_hz(clk_info, mode);
	if (clk_rate_hz) {
		clk_info->used_rate_hz = clk_rate_hz;
		clk_set_rate(clk_info->clk, clk_rate_hz);
	}

	return 0;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0))
#define PDE_DATA(inode)         pde_data(inode)
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
/* already has the Macro definition */
#else /* (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)) */
#define PDE_DATA(inode)         NULL
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)) */

#ifdef CONFIG_PROC_FS
#if 0
static int fops_show_u32(struct seq_file *file, void *v)
{
	u32 *val = file->private;

	seq_printf(file, "%d\n", *val);

	return 0;
}

static int fops_open_u32(struct inode *inode, struct file *file)
{
	return single_open(file, fops_show_u32, PDE_DATA(inode));
}

static ssize_t fops_write_u32(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	int rc;
	struct seq_file *priv = file->private_data;

	rc = kstrtou32_from_user(buf, count, 0, priv->private);
	if (rc)
		return rc;

	return count;
}

static const struct proc_ops procfs_fops_u32 = {
	.proc_open = fops_open_u32,
	.proc_read = seq_read,
	.proc_release = single_release,
	.proc_write = fops_write_u32,
};

struct proc_dir_entry *
mpp_procfs_create_u32(const char *name, umode_t mode,
		      struct proc_dir_entry *parent, void *data)
{
	return proc_create_data(name, mode, parent, &procfs_fops_u32, data);
}
#endif

static int fops_show_clk(struct seq_file *file, void *v)
{
	struct mpp_clk_info *mpp_clk = file->private;
	u32 clk_rate = clk_get_rate(mpp_clk->clk);
	struct clk_hw *hw = __clk_get_hw(mpp_clk->clk);

	if (hw) {
		u32 parent_num = clk_hw_get_num_parents(hw);
		struct clk *p_clk = clk_get_parent(mpp_clk->clk);
		u32 i;

		seq_printf(file, "%s %dHz\n",
			   __clk_get_name(mpp_clk->clk), clk_rate);

		if (p_clk) {
			seq_printf(file, "------ parents info ------\n");
			seq_printf(file, "current parent: %s %ldHz\n",
				   __clk_get_name(p_clk), clk_get_rate(p_clk));
		}

		for (i = 0; i < parent_num; i++) {
			struct clk_hw *p_hw = clk_hw_get_parent_by_index(hw, i);

			if (!p_hw)
				continue;

			seq_printf(file, "parent[%d]: %s %ldHz ",
				   i, clk_hw_get_name(p_hw), clk_hw_get_rate(p_hw));

			do {
				p_hw = clk_hw_get_parent(p_hw);
				if (p_hw)
					seq_printf(file, "-> %s %ldHz ",
						   clk_hw_get_name(p_hw), clk_hw_get_rate(p_hw));
			} while (p_hw);
			seq_printf(file, "\n");
		}
	} else
		seq_printf(file, "%s %dHz\n", __clk_get_name(mpp_clk->clk), clk_rate);

	return 0;
}

static int fops_open_clk(struct inode *inode, struct file *file)
{
	return single_open(file, fops_show_clk, PDE_DATA(inode));
}

static ssize_t fops_write_clk(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	int i = 0, j = 0;
	struct seq_file *priv = file->private_data;
	u32 clk_rate, cur_rate;
	struct mpp_clk_info *mpp_clk = priv->private;
	char kbuf[64];
	char *cur, *token;
	u32 values[3] = {0};

	if (count >= sizeof(kbuf))
		return -EINVAL;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';
	cur = kbuf;

	while ((token = strsep(&cur, " ")) != NULL && i < 3) {
		if (kstrtouint(token, 10, &values[i]) == 0) {
			values[i] = values[i] < MHZ ? values[i] * MHZ : values[i];
			i++;
		}
	}

	for (j = i - 1; j > 0 ; j--) {
		u32 p_idx = j;
		struct clk *p_clk = mpp_clk->clk;

		while (p_idx--)
			p_clk = clk_get_parent(p_clk);

		if (p_clk)
			clk_set_rate(p_clk, values[j]);
	}

	clk_rate = values[0];
	cur_rate = clk_get_rate(mpp_clk->clk);
	if (clk_rate != cur_rate)
		clk_set_rate(mpp_clk->clk, clk_rate);

	return count;
}

static const struct proc_ops procfs_fops_clk = {
	.proc_open = fops_open_clk,
	.proc_read = seq_read,
	.proc_release = single_release,
	.proc_write = fops_write_clk,
};

struct proc_dir_entry *mpp_procfs_create_clk_rw(const char *name, umode_t mode,
						struct proc_dir_entry *parent,
						struct mpp_clk_info *mpp_clk)
{
	return proc_create_data(name, mode, parent, &procfs_fops_clk, (void*)mpp_clk);
}
#endif

struct mpp_service *g_srv;
static struct mpp_session *mpp_chnl_open(int client_type)
{
	int ret;
	struct mpp_dev *mpp;
	struct mpp_session *session = NULL;

	if (!g_srv)
		return NULL;
	session = mpp_session_init();
	if (!session)
		return NULL;
	session->srv = g_srv;
	session->k_space = 1;
	session->pp_session = false;
	session->process_task = mpp_process_task_default;
	session->wait_result = mpp_wait_result_default;
	session->deinit = mpp_session_deinit_default;
	if (session->srv) {
		mutex_lock(&g_srv->session_lock);
		list_add_tail(&session->service_link, &g_srv->session_list);
		mutex_unlock(&g_srv->session_lock);
	}
#ifdef RKVEPU500_PP_ENABLE
	if (client_type == MPP_DEVICE_RKVENC_PP) {
		session->pp_session = true;
		client_type = MPP_DEVICE_RKVENC;
	}
#endif
	client_type = array_index_nospec(client_type, MPP_DEVICE_BUTT);
	mpp = g_srv->sub_devices[client_type];
	if (!mpp)
		return NULL;
	session->device_type = (enum MPP_DEVICE_TYPE)client_type;
	session->mpp = mpp;
	if (mpp->dev_ops) {
		if (mpp->dev_ops->process_task)
			session->process_task =
				mpp->dev_ops->process_task;

		if (mpp->dev_ops->wait_result)
			session->wait_result =
				mpp->dev_ops->wait_result;

		if (mpp->dev_ops->deinit)
			session->deinit = mpp->dev_ops->deinit;
	}
	session->index = atomic_fetch_inc(&mpp->session_index);
	if (mpp->dev_ops && mpp->dev_ops->init_session) {
		ret = mpp->dev_ops->init_session(session);
		if (ret)
			return NULL;
	}
	mpp_session_attach_workqueue(session, mpp->queue);
	return session;
}

static int mpp_chnl_register(struct mpp_session *session, void *fun, u32 chn_id)
{
	session->callback = fun;
	session->chn_id = chn_id & 0xffff;
	session->online = chn_id >> 16;
	session->mpp->online_mode = session->online;
	if (session->online) {
		session->mpp->always_on = 1;
		mpp_power_on(session->mpp);
	}

	return 0;
}

static int mpp_chnl_release(struct mpp_session *session)
{
	mpp_debug_enter();
	if (!session) {
		mpp_err("session is null\n");
		return -EINVAL;
	}
	/* wait for task all done */
	atomic_inc(&session->release_request);

	if (session->mpp)
		mpp_session_detach_workqueue(session);
	else
		mpp_session_deinit(session);

	return 0;
}

static int mpp_chnl_add_req(struct mpp_session *session,  void *reqs)
{
	struct mpp_task_msgs task_msgs;
	struct mpp_request *req = NULL;
	void *msg = reqs;
	int ret = 0;

	memset(&task_msgs, 0, sizeof(task_msgs));
	do {
		req = &task_msgs.reqs[task_msgs.req_cnt];
		/* first, parse to fixed struct */
		{
			struct mpp_msg_v1 *msg_v1;

			msg_v1 = (struct mpp_msg_v1 *)msg;
			ret = mpp_parse_msg_v1(msg_v1, req);
			if (ret)
				return -EFAULT;

			msg += sizeof(struct mpp_msg_v1);
		}
		task_msgs.req_cnt++;
		/* check loop times */
		if (task_msgs.req_cnt > MPP_MAX_MSG_NUM) {
			mpp_err("fail, message count %d more than %d.\n",
				task_msgs.req_cnt, MPP_MAX_MSG_NUM);
			return -EINVAL;
		}
		/* second, process request */
		ret = mpp_process_request(session, session->srv, req, &task_msgs);
		if (ret)
			return -EFAULT;
		/* last, process task message */
		if (mpp_msg_is_last(req)) {
			session->msg_flags = task_msgs.flags;
			if (task_msgs.set_cnt > 0) {
				struct mpp_dev *mpp = NULL;
				struct mpp_task *task = NULL;
				struct mpp_taskqueue *queue = NULL;
				ret = mpp_process_task(session, &task_msgs);
				if (ret)
					return ret;

				mpp = task_msgs.mpp;
				task = task_msgs.task;
				queue = task_msgs.queue;

				atomic_inc(&mpp->task_count);
				set_bit(TASK_STATE_PENDING, &task->state);

				/* online task trigger by user */
				if (!session->online) {
					mutex_lock(&queue->pending_lock);
					list_add_tail(&task->queue_link, &queue->pending_list);
					mutex_unlock(&queue->pending_lock);
					mpp_taskqueue_trigger_work(mpp);
				}
			}
			if (task_msgs.poll_cnt > 0) {
				ret = mpp_wait_result(session, &task_msgs);
				if (ret)
					return ret;
			}
		}
	} while (!mpp_msg_is_last(req));

	return 0;
}

static u32 mpp_chnl_get_iova_addr(struct mpp_session *session,  struct dma_buf *buf, u32 reg_idx)
{
	struct mpp_dev *mpp = NULL;

	if (!session) {
		mpp_err("session is null");
		return -1;
	}
	mpp = session->mpp;

	return mpp_dma_get_iova(buf, mpp->dev);
}

static struct device *mpp_chnl_get_dev(struct mpp_session *session)
{
	struct mpp_dev *mpp = session->mpp;

	return mpp->dev;
}

int mpp_show_session_info(struct seq_file *seq, u32 chan_id)
{
	struct mpp_session *session = NULL, *n;
	struct mpp_service *srv = g_srv;

	mutex_lock(&srv->session_lock);
	list_for_each_entry_safe(session, n,
				 &srv->session_list,
				 service_link) {
		struct  mpp_dev *mpp;

		if (session->chn_id != chan_id)
			continue;

		if (!session->priv)
			continue;

		if (!session->mpp)
			continue;
		mpp = session->mpp;

		if (mpp->dev_ops->dump_session)
			mpp->dev_ops->dump_session(session, seq);
	}
	mutex_unlock(&srv->session_lock);

	return 0;
}
EXPORT_SYMBOL(mpp_show_session_info);

u32 mpp_srv_get_phy(struct dma_buf *buf)
{
	return mpp_dma_get_iova(buf, g_srv->dev);
}

EXPORT_SYMBOL(mpp_srv_get_phy);

struct vcodec_mppdev_svr_fn {
	struct mpp_session *(*chnl_open)(int client_type);
	int (*chnl_register)(struct mpp_session *session, void *fun, unsigned int chn_id);
	int (*chnl_release)(struct mpp_session *session);
	int (*chnl_add_req)(struct mpp_session *session,  void *reqs);
	unsigned int (*chnl_get_iova_addr)(struct mpp_session *session,
					   struct dma_buf *buf, unsigned int reg_idx);
	void (*chnl_release_iova_addr)(struct mpp_session *session,  struct dma_buf *buf);
	struct device *(*mpp_chnl_get_dev)(struct mpp_session *session);
	int (*chnl_run_task)(struct mpp_session *session, void *param);
	int (*chnl_check_running)(struct mpp_session *session);
};

struct vcodec_mppdev_svr_fn g_mpp_svr_fn_ops    = {
	.chnl_open				= mpp_chnl_open,
	.chnl_register			= mpp_chnl_register,
	.chnl_release			= mpp_chnl_release,
	.chnl_add_req			= mpp_chnl_add_req,
	.chnl_get_iova_addr		= mpp_chnl_get_iova_addr,
	.chnl_release_iova_addr		= NULL,
	.mpp_chnl_get_dev		= mpp_chnl_get_dev,
	.chnl_run_task			= mpp_chnl_run_task,
	.chnl_check_running		= mpp_chnl_is_running,
};

struct vcodec_mppdev_svr_fn *get_mppdev_svr_ops(void)
{
	return &g_mpp_svr_fn_ops;
}

EXPORT_SYMBOL(get_mppdev_svr_ops);
