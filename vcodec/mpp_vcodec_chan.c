// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#define MODULE_TAG "mpp_vcodec_chan"

#include <linux/dma-buf.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/delay.h>
#include "mpp_vcodec_chan.h"
#include "mpp_vcodec_base.h"
#include "mpp_vcodec_flow.h"
#include "mpp_log.h"
#include "mpp_enc.h"
#include "mpp_vcodec_thread.h"
#include "rk_venc_cfg.h"
#include "rk_export_func.h"
#include "mpp_packet_impl.h"
#include "mpp_time.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_mem.h"
#include "mpp_vcodec_rockit.h"

int mpp_vcodec_schedule(void)
{
	return 0;
}

int mpp_vcodec_chan_create(struct vcodec_attr *attr)
{
	int chan_id = attr->chan_id;
	MppCtxType type = attr->type;
	MppCodingType coding = attr->coding;
	RK_S32	online = attr->online;
	RK_U32  buf_size = attr->buf_size;
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);
	MPP_RET ret = MPP_NOK;
	RK_U32 init_done = 1;
	RK_U32 num_chan = mpp_vcodec_get_chan_num(type);

	mpp_log("num_chan = %d", num_chan);
	mpp_assert(chan_entry->chan_id == chan_id);

	if (chan_entry->handle != NULL) {
		if (attr->chan_dup)
			return 0;

		chan_id = mpp_vcodec_get_free_chan(type);
		if (chan_id < 0)
			return -1;

		mpp_log("current chan %d already created get new chan_id %d \n",
			attr->chan_id, chan_id);
		attr->chan_id = chan_id;
		chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);
	} else {
		if (attr->chan_dup)
			return -1;
	}
	switch (type) {
	case MPP_CTX_DEC: {

	} break;
	case MPP_CTX_ENC: {
		MppEnc enc = NULL;
		MppEncInitCfg cfg = {
			.coding         = coding,
			.online         = online,
			.buf_size       = buf_size,
			.max_strm_cnt   = attr->max_strm_cnt,
			.ref_buf_shared = attr->shared_buf_en,
			.smart_en       = attr->smart_en,
			.shared_buf     = &chan_entry->shared_buf,
			.qpmap_en       = attr->smart_en ? 1 : attr->qpmap_en,
			.chan_id        = chan_id,
			.only_smartp    = attr->only_smartp,
			.tmvp_enable    = attr->tmvp_enable
		};

		ret = mpp_enc_init(&enc, &cfg);
		if (ret)
			break;
		mpp_enc_register_chl(enc, (void *)mpp_vcodec_enc_int_handle, chan_id);
		mpp_vcodec_chan_entry_init(chan_entry, type, coding, (void *)enc);
#ifdef CHAN_BUF_SHARED
		if (mpp_vcodec_chan_setup_hal_bufs(chan_entry, attr)) {
			mpp_enc_deinit(chan_entry->handle);
			mpp_vcodec_chan_entry_deinit(chan_entry);
			return -1;
		}
#endif
		mpp_vcodec_inc_chan_num(type);
		chan_entry->cfg.online = online;
		chan_entry->last_yuv_time = mpp_time();
		chan_entry->last_jeg_combo_start = mpp_time();
		chan_entry->last_jeg_combo_end = mpp_time();
		init_done = 1;

		mpp_log("create channel %d handle %p online %d\n",
			chan_id, chan_entry->handle, online);
	} break;
	default: {
		mpp_err("create chan error type %d\n", type);
	} break;
	}

	if (!init_done)
		mpp_err("create chan %d fail \n", chan_id);

	return 0;
}

int mpp_vcodec_chan_unbind(struct mpp_chan* chan)
{
	RK_S32 chan_id = chan->binder_chan_id != -1 ? chan->binder_chan_id : chan->master_chan_id;
	struct mpp_chan *bind_chan;

	if (chan_id == -1)
		return 0;

	bind_chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	mpp_assert(bind_chan);
	if (chan->binder_chan_id != -1) {
		/* bind chan is slave */
		atomic_set(&chan->cfg.comb_runing, 0);
		atomic_set(&bind_chan->runing, 0);
		mpp_enc_deinit_frame(bind_chan->handle);
		chan->binder_chan_id = -1;
		bind_chan->master_chan_id = -1;
	} else {
		/* bind_chan is master */
		atomic_set(&bind_chan->cfg.comb_runing, 0);
		atomic_set(&chan->runing, 0);
		chan->master_chan_id = -1;
		bind_chan->binder_chan_id = -1;
	}

	return 0;
}

#define VCODEC_WAIT_TIMEOUT_DELAY		(2200)

int mpp_vcodec_chan_destory(int chan_id, MppCtxType type)
{
	int ret;
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);

	if (!chan_entry->handle)
		return 0;
	mpp_assert(chan_entry->handle != NULL);
	switch (type) {
	case MPP_CTX_DEC: {
	} break;
	case MPP_CTX_ENC: {
		bool wait = true;

		mpp_log("destroy chan %d hnd %p online %d combo %d mst %d\n",
			chan_id, chan_entry->handle, chan_entry->cfg.online,
			chan_entry->binder_chan_id, chan_entry->master_chan_id);

		ret = mpp_vcodec_chan_stop(chan_id, type);
		if (chan_entry->cfg.online) {
			struct mpp_chan *chan_tmp = chan_entry;

			if (chan_entry->master_chan_id != -1) {
				chan_tmp = mpp_vcodec_get_chan_entry(chan_entry->master_chan_id, type);
				mpp_enc_unbind_jpeg_task(chan_tmp->handle);
			}
			if (!mpp_enc_check_hw_running(chan_tmp->handle) &&
			    !mpp_enc_check_is_int_process(chan_tmp->handle))
				wait = false;
		}

		if (wait)
			ret = wait_event_timeout(chan_entry->stop_wait,
						 !atomic_read(&chan_entry->runing),
						 msecs_to_jiffies(VCODEC_WAIT_TIMEOUT_DELAY));

		if (chan_entry->cfg.online)
			mpp_vcodec_chan_unbind(chan_entry);
		mutex_lock(&chan_entry->chan_debug_lock);
		mpp_enc_deinit(chan_entry->handle);
		mpp_vcodec_stream_clear(chan_entry);
		mpp_vcodec_dec_chan_num(type);
		mpp_vcodec_chan_entry_deinit(chan_entry);
		mutex_unlock(&chan_entry->chan_debug_lock);
		mpp_log("destroy chan %d done\n", chan_id);
	} break;
	default: {
		mpp_err("create chan error type %d\n", type);
	} break;
	}

	return 0;
}

int mpp_vcodec_chan_start(int chan_id, MppCtxType type)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);

	mpp_assert(chan_entry->handle != NULL);
	if (!chan_entry->handle)
		return MPP_NOK;

	switch (type) {
	case MPP_CTX_DEC: {

	} break;
	case MPP_CTX_ENC: {
		unsigned long lock_flag;

		mpp_enc_start(chan_entry->handle);
		spin_lock_irqsave(&chan_entry->chan_lock, lock_flag);
		chan_entry->state = CHAN_STATE_RUN;
		spin_unlock_irqrestore(&chan_entry->chan_lock,
				       lock_flag);
		enc_chan_update_chan_prior_tab();
	} break;
	default: {
		mpp_err("create chan error type %d\n", type);
	} break;
	}

	return 0;
}

int mpp_vcodec_chan_stop(int chan_id, MppCtxType type)
{
	int ret = 0;
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);

	mpp_assert(chan_entry->handle != NULL);
	switch (type) {
	case MPP_CTX_DEC: {

	} break;
	case MPP_CTX_ENC: {
		unsigned long lock_flag;

		ret = mpp_enc_stop(chan_entry->handle);
		spin_lock_irqsave(&chan_entry->chan_lock, lock_flag);
		if (chan_entry->state != CHAN_STATE_RUN) {
			spin_unlock_irqrestore(&chan_entry->chan_lock,
					       lock_flag);
			return 0;
		}
		chan_entry->state = CHAN_STATE_SUSPEND;
		spin_unlock_irqrestore(&chan_entry->chan_lock,
				       lock_flag);
		wake_up(&chan_entry->wait);
		enc_chan_update_chan_prior_tab();
	} break;
	default: {
		mpp_err("create chan error type %d\n", type);
	} break;
	}

	return ret;
}

int mpp_vcodec_chan_pause(int chan_id, MppCtxType type)
{
	return 0;
}

int mpp_vcodec_chan_resume(int chan_id, MppCtxType type)
{
	return 0;
}

int mpp_vcodec_chan_get_stream(int chan_id, MppCtxType type,
			       struct venc_packet *enc_packet)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);
	RK_U32 count = atomic_read(&chan_entry->stream_count);
	MppPacketImpl *packet = NULL;
	unsigned long flags;

	if (!count) {
		mpp_err("no stream count found in list \n");
		memset(enc_packet, 0, sizeof(struct venc_packet));

		return MPP_NOK;
	}

	spin_lock_irqsave(&chan_entry->stream_list_lock, flags);
	packet = list_first_entry_or_null(&chan_entry->stream_done, MppPacketImpl, list);
	list_move_tail(&packet->list, &chan_entry->stream_remove);
	spin_unlock_irqrestore(&chan_entry->stream_list_lock, flags);

	/* flush cache before get packet*/
	if (packet->buf.buf)
		mpp_ring_buf_flush(&packet->buf, 1);
	atomic_dec(&chan_entry->stream_count);

	enc_packet->flag = mpp_packet_get_flag(packet);
	enc_packet->len = mpp_packet_get_length(packet);
	enc_packet->temporal_id = mpp_packet_get_temporal_id(packet);
	enc_packet->u64pts = mpp_packet_get_pts(packet);
	enc_packet->u64dts = mpp_packet_get_dts(packet);
	enc_packet->data_num = 1;
	enc_packet->u64priv_data = packet->buf.mpi_buf_id;
	if (1 || packet->buf.mpi_buf_id == -1) {
		struct dma_buf *dmabuf = mpp_buffer_get_dma(packet->buf.buf);
		get_dma_buf(dmabuf);
		enc_packet->u64priv_data = dma_buf_fd(dmabuf, 0);
		enc_packet->u64priv_data |= (RK_U64)((RK_U64)1 << 32);
	}
	enc_packet->offset = packet->buf.start_offset;
	enc_packet->u64packet_addr = (uintptr_t )packet;
	enc_packet->buf_size = mpp_buffer_get_size(packet->buf.buf);

	chan_entry->seq_user_get = mpp_packet_get_dts(packet);
	atomic_inc(&chan_entry->str_out_cnt);
	atomic_inc(&chan_entry->pkt_user_get);

	return MPP_OK;
}

int mpp_vcodec_chan_put_stream(int chan_id, MppCtxType type,
			       struct venc_packet *enc_packet)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);
	MppPacketImpl *packet = NULL, *n;
	struct venc_module *venc =  NULL;
	RK_U32 found = 0;
	unsigned long flags;

	spin_lock_irqsave(&chan_entry->stream_list_lock, flags);
	list_for_each_entry_safe(packet, n, &chan_entry->stream_remove, list) {
		if ((uintptr_t)packet == enc_packet->u64packet_addr) {
			list_del_init(&packet->list);
			kref_put(&packet->ref, stream_packet_free);
			atomic_dec(&chan_entry->str_out_cnt);
			atomic_inc(&chan_entry->pkt_user_put);
			chan_entry->seq_user_put = mpp_packet_get_dts(packet);
			venc = mpp_vcodec_get_enc_module_entry();
			vcodec_thread_trigger(venc->thd);
			found = 1;
			break;
		}
	}

	if (!found) {
		mpp_err("release packet fail %llx \n", enc_packet->u64packet_addr);
		list_for_each_entry_safe(packet, n, &chan_entry->stream_remove, list) {
			RK_U64 p_address = (uintptr_t )packet;

			mpp_err("dump packet out list %llx \n", p_address);
		}
		mpp_assert(found);
	}
	spin_unlock_irqrestore(&chan_entry->stream_list_lock, flags);

	return 0;
}

int mpp_vcodec_chan_push_frm(int chan_id, void *param)
{
	struct mpp_frame_infos *info = param;
	struct venc_module *venc = NULL;
	struct vcodec_threads *thd;
	struct mpp_chan *chan_entry = NULL;
	MppBufferInfo buf_info;
	MppBuffer buffer = info->mpp_buffer;
	MppFrame frame = NULL;

	chan_entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	venc = mpp_vcodec_get_enc_module_entry();
	thd = venc->thd;

	memset(&buf_info, 0, sizeof(buf_info));
	if (info->fd) {
		buf_info.fd = info->fd;
		mpp_buffer_import(&buffer, &buf_info);
	}
	mpp_frame_init_with_frameinfo(&frame, info);
	if (info->jpeg_chan_id > 0) {
		MppFrame comb_frame = NULL;

		mpp_frame_init(&comb_frame);
		mpp_frame_copy(comb_frame, frame);
		if (info->jpg_combo_osd_buf)
			frame_add_osd(comb_frame, (MppEncOSDData3 *)info->jpg_combo_osd_buf);
		mpp_frame_set_buffer(comb_frame, buffer);
		mpp_frame_set_chan_id(comb_frame, info->jpeg_chan_id);
		mpp_frame_set_combo_frame(frame, comb_frame);
	}
	mpp_frame_set_buffer(frame, buffer);
	mpp_buffer_put(buffer);
	chan_entry->frame = frame;

	vcodec_thread_trigger(thd);

	return 0;
}

static int mpp_vcodec_chan_change_coding_type(int chan_id, void *arg)
{
	struct vcodec_attr *attr = (struct vcodec_attr*)arg;
	struct mpp_chan *entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	int ret;
	MppEncCfgImpl *cfg = mpp_malloc(MppEncCfgImpl, 1);
	bool wait = true;

	if (!cfg) {
		mpp_err("change_coding_type malloc fail");
		return MPP_NOK;
	}

	mpp_enc_control(entry->handle, MPP_ENC_GET_CFG, cfg);
	mpp_assert(entry->handle != NULL);
	mpp_assert(chan_id == attr->chan_id);

	ret = mpp_vcodec_chan_stop(chan_id, MPP_CTX_ENC);
	if (entry->cfg.online) {
		struct mpp_chan *chan_tmp = entry;

		if (entry->master_chan_id != -1) {
			chan_tmp = mpp_vcodec_get_chan_entry(entry->master_chan_id, MPP_CTX_ENC);
			mpp_enc_unbind_jpeg_task(chan_tmp->handle);
		}
		if (!mpp_enc_check_hw_running(chan_tmp->handle) &&
		    !mpp_enc_check_is_int_process(chan_tmp->handle))
			wait = false;
	}

	if (wait)
		ret = wait_event_timeout(entry->stop_wait,
					 !atomic_read(&entry->runing),
					 msecs_to_jiffies(VCODEC_WAIT_TIMEOUT_DELAY));

	if (entry->cfg.online)
		mpp_vcodec_chan_unbind(entry);
	mutex_lock(&entry->chan_debug_lock);
	mpp_enc_deinit(entry->handle);
	mpp_vcodec_stream_clear(entry);
	mpp_vcodec_dec_chan_num(MPP_CTX_ENC);
	entry->handle = NULL;
	entry->state = CHAN_STATE_NULL;
	entry->reenc = 0;
	entry->binder_chan_id = -1;
	entry->master_chan_id = -1;
	mutex_unlock(&entry->chan_debug_lock);
	mpp_vcodec_chan_create(attr);
	entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);

	mpp_enc_control(entry->handle, MPP_ENC_SET_PREP_CFG, &cfg->cfg.prep);

	cfg->cfg.rc.change = MPP_ENC_RC_CFG_CHANGE_ALL;
	mpp_enc_control(entry->handle, MPP_ENC_SET_RC_CFG, &cfg->cfg.rc);

	mpp_enc_control(entry->handle, MPP_ENC_SET_REF_CFG, &cfg->cfg.ref_param);

	mpp_enc_control(entry->handle, MPP_ENC_GET_CFG, cfg);
	cfg->cfg.prep.change = MPP_ENC_PREP_CFG_CHANGE_ALL;
	mpp_enc_control(entry->handle, MPP_ENC_SET_CFG, cfg);

	mpp_vcodec_chan_start(chan_id, MPP_CTX_ENC);
	mpp_free(cfg);

	return 0;
}

int mpp_vcodec_chan_control(int chan_id, MppCtxType type, int cmd, void *arg)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, type);
	mpp_assert(chan_entry->handle != NULL);

	switch (type) {
	case MPP_CTX_DEC: {
		;
	} break;
	case MPP_CTX_ENC: {
		if (cmd == MPP_ENC_SET_CHANGE_STREAM_TYPE)
			mpp_vcodec_chan_change_coding_type(chan_id, arg);
		else {
			MppEncCfgImpl *cfg = (MppEncCfgImpl *)arg;
			bool prep_change = cfg ? cfg->cfg.prep.change : false;

			mpp_enc_control(chan_entry->handle, cmd, arg);
			/*
			* In the case of wrapping, when switching resolutions,
			* need to ensure that the wrapping frame being encoded finish.
			*/
			if (chan_entry->cfg.online && prep_change) {
				if (mpp_enc_check_hw_running(chan_entry->handle)) {
					wait_event_timeout(chan_entry->stop_wait,
							   !atomic_read(&chan_entry->runing),
							   msecs_to_jiffies(200));
				}
			}
		}
	} break;
	default: {
		mpp_err("control type %d error\n", type);
	} break;
	}

	return 0;
}

EXPORT_SYMBOL(mpp_vcodec_chan_create);
EXPORT_SYMBOL(mpp_vcodec_chan_destory);
EXPORT_SYMBOL(mpp_vcodec_chan_start);
EXPORT_SYMBOL(mpp_vcodec_chan_stop);
EXPORT_SYMBOL(mpp_vcodec_chan_pause);
EXPORT_SYMBOL(mpp_vcodec_chan_resume);
EXPORT_SYMBOL(mpp_vcodec_chan_control);
EXPORT_SYMBOL(mpp_vcodec_chan_push_frm);
EXPORT_SYMBOL(mpp_vcodec_chan_get_stream);
EXPORT_SYMBOL(mpp_vcodec_chan_put_stream);
EXPORT_SYMBOL(mpp_vcodec_schedule);
