/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

#include "kmpp_atomic.h"
#include "kmpp_mem.h"
#include "kmpp_wait.h"

struct osal_wait_t {
    rk_u32 need_free;
    void *wait;
};

rk_s32 osal_wait_init(osal_wait **wait)
{
    osal_wait *p = NULL;
    wait_queue_head_t *wq = NULL;

    if (!wait)
        return -1;

    p = osal_kmalloc(sizeof(osal_wait) + sizeof(wait_queue_head_t), osal_gfp_normal);
    if (!p)
        return -1;

    wq = (wait_queue_head_t *)(p + 1);

    init_waitqueue_head(wq);
    p->wait = (void *)wq;
    p->need_free = 1;

    *wait = p;
    return 0;
}
EXPORT_SYMBOL(osal_wait_init);

void osal_wait_deinit(osal_wait **wait)
{
    if (wait) {
        osal_wait *p = KMPP_FETCH_AND(wait, NULL);

        if (p && p->need_free)
            osal_kfree(p);
    }
}
EXPORT_SYMBOL(osal_wait_deinit);

rk_s32 osal_wait_size(void)
{
    return sizeof(wait_queue_head_t);
}
EXPORT_SYMBOL(osal_wait_size);

rk_s32 osal_wait_assign(osal_wait **wait, void *buf, rk_s32 size)
{
    if (wait && buf && size >= sizeof(wait_queue_head_t)) {
        osal_wait *p = (osal_wait *)buf;
        wait_queue_head_t *wq = (wait_queue_head_t *)(buf + sizeof(osal_wait));

        init_waitqueue_head(wq);
        p->wait = (void *)wq;
        p->need_free = 0;

        *wait = p;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_wait_assign);

rk_s32 osal_wait_interruptible(osal_wait *wait, osal_wait_cond_func func, void *param)
{
    if (wait && wait->wait) {
        DEFINE_WAIT(__wait);
        rk_s32 cond = 0;
        rk_s32 ret = 0;

        wait_queue_head_t *wq = (wait_queue_head_t *)(wait->wait);

        prepare_to_wait(wq, &__wait, TASK_INTERRUPTIBLE);
        if (func)
            cond = func(param);

        if (!cond) {
            if (!signal_pending(current)) {
                schedule();
            }
            if (signal_pending(current)) {
                ret = -ERESTARTSYS;
            }
        }

        finish_wait(wq, &__wait);
        return ret;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_wait_interruptible);

rk_s32 osal_wait_uninterruptible(osal_wait *wait, osal_wait_cond_func func, void *param)
{
    if (wait && wait->wait) {
        DEFINE_WAIT(__wait);
        rk_s32 cond = 0;
        rk_s32 ret = 0;

        wait_queue_head_t *wq = (wait_queue_head_t *)(wait->wait);

        prepare_to_wait(wq, &__wait, TASK_UNINTERRUPTIBLE);
        if (func)
            cond = func(param);

        if (!cond)
            schedule();

        finish_wait(wq, &__wait);
        return ret;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_wait_uninterruptible);

rk_s32 osal_wait_timeout_interruptible(osal_wait *wait, osal_wait_cond_func func, void *param, rk_u32 ms)
{
    if (wait && wait->wait) {
        DEFINE_WAIT(__wait);
        rk_s32 cond = 0;
        rk_s32 ret = ms;

        wait_queue_head_t *wq = (wait_queue_head_t *)(wait->wait);

        prepare_to_wait(wq, &__wait, TASK_INTERRUPTIBLE);
        if (func)
            cond = func(param);

        if (!cond) {
            if (!signal_pending(current)) {
                ret = schedule_timeout(msecs_to_jiffies(ret));
                ret = jiffies_to_msecs(ret);
            }
            if (signal_pending(current)) {
                ret = -ERESTARTSYS;
            }
        }

        finish_wait(wq, &__wait);
        return ret;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_wait_timeout_interruptible);

rk_s32 osal_wait_timeout_uninterruptible(osal_wait *wait, osal_wait_cond_func func, void *param, rk_u32 ms)
{
    if (wait && wait->wait) {
        DEFINE_WAIT(__wait);
        rk_s32 cond = 0;
        rk_s32 ret = ms;

        wait_queue_head_t *wq = (wait_queue_head_t *)(wait->wait);

        prepare_to_wait(wq, &__wait, TASK_UNINTERRUPTIBLE);
        if (func)
            cond = func(param);

        if (!cond) {
            ret = schedule_timeout(msecs_to_jiffies(ret));
            ret = jiffies_to_msecs(ret);
        }

        finish_wait(wq, &__wait);
        return ret;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_wait_timeout_uninterruptible);

void osal_wait_wakeup(osal_wait *wait)
{
    if (wait && wait->wait)
        wake_up_all((wait_queue_head_t *)(wait->wait));
}
EXPORT_SYMBOL(osal_wait_wakeup);