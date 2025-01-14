// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#define MODULE_TAG "mpp_vcodec"

#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/module.h>

#include "mpp_vcodec_flow.h"
#include "mpp_vcodec_base.h"
#include "mpp_vcodec_thread.h"
#include "mpp_enc.h"
#include "mpp_log.h"
#include "mpp_buffer_impl.h"
#include "mpp_packet.h"
#include "mpp_vcodec_debug.h"
#include "mpp_packet_impl.h"
#include "mpp_time.h"
#include "kmpp_obj.h"
#include "kmpp_venc_objs_impl.h"

void mpp_vcodec_enc_add_packet_list(struct mpp_chan *chan_entry, MppPacket packet);

MPP_RET mpp_frame_init_with_frameinfo(KmppFrame *frame, struct mpp_frame_infos *info)
{
	KmppFrame p = NULL;

	if (NULL == frame) {
		mpp_err_f("invalid NULL pointer input\n");
		return MPP_ERR_NULL_PTR;
	}

	kmpp_frame_get(&p);
	kmpp_frame_set_width(p, info->width);
	kmpp_frame_set_height(p, info->height);
	kmpp_frame_set_hor_stride(p, info->hor_stride);
	kmpp_frame_set_ver_stride(p, info->ver_stride);
	kmpp_frame_set_pts(p, info->pts);
	kmpp_frame_set_dts(p, info->dts);
	kmpp_frame_set_fmt(p, info->fmt);
	kmpp_frame_set_offset_x(p, info->offset_x);
	kmpp_frame_set_offset_y(p, info->offset_y);
	kmpp_frame_set_is_gray(p, info->is_gray);
	kmpp_frame_set_is_full(p, info->is_full);
	kmpp_frame_set_phy_addr(p, info->phy_addr);
	kmpp_frame_set_idr_request(p, info->idr_request);
	kmpp_frame_set_eos(p, info->eos);
	kmpp_frame_set_pskip_request(p, info->pskip);
	kmpp_frame_set_pskip_num(p, info->pskip_num);

	if (info->pp_info)
		kmpp_frame_set_pp_info(p, (MppPpInfo*)info->pp_info);
	*frame = p;

	return MPP_OK;
}

static MPP_RET enc_chan_process_single_chan(RK_U32 chan_id)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	struct mpp_chan *comb_chan = NULL;
	KmppFrame frame = NULL;
	KmppFrame comb_frame = NULL;
	unsigned long lock_flag;
	RK_U64 cfg_start = 0, cfg_end = 0;
	RK_U64 dts = 0;

	if (!chan_entry) {
		mpp_err_f("chan_entry is NULL\n");
		return MPP_NOK;
	}

	spin_lock_irqsave(&chan_entry->chan_lock, lock_flag);
	if (chan_entry->state != CHAN_STATE_RUN) {
		mpp_err("cur chnl %d state is no runing\n", chan_id);
		spin_unlock_irqrestore(&chan_entry->chan_lock, lock_flag);
		return MPP_OK;
	}
	spin_unlock_irqrestore(&chan_entry->chan_lock, lock_flag);

	if (atomic_read(&chan_entry->runing) > 0) {
		mpp_vcodec_detail("cur chnl %d state is wating irq\n", chan_id);
		return MPP_OK;
	}

	mpp_vcodec_detail("enc_chan_process_single_chan id %d\n", chan_id);
	if (!chan_entry->reenc) {
		frame = chan_entry->frame;
		if (!frame)
			return MPP_OK;
		chan_entry->frame = NULL;
		chan_entry->gap_time = (RK_S32)(mpp_time() - chan_entry->last_yuv_time);
		chan_entry->last_yuv_time = mpp_time();
		kmpp_frame_get_combo_frame(frame, &comb_frame);
		if (comb_frame) {
			RK_U32 jpeg_chan_id = 0;

			kmpp_frame_get_chan_id(comb_frame, &jpeg_chan_id);

			mpp_vcodec_jpegcomb("attach jpeg id %d\n", jpeg_chan_id);
			comb_chan = mpp_vcodec_get_chan_entry(jpeg_chan_id, MPP_CTX_ENC);
			if (comb_chan->state != CHAN_STATE_RUN)
				comb_chan = NULL;

			if (comb_chan && atomic_read(&comb_chan->runing) > 0) {
				mpp_err_f("chan %d combo chan %d state is wating irq\n",
					  chan_id, jpeg_chan_id);
				comb_chan = NULL;
			}

			if (comb_chan && comb_chan->handle) {
				chan_entry->combo_gap_time = (RK_S32)(mpp_time() - chan_entry->last_jeg_combo_start);
				chan_entry->last_jeg_combo_start = mpp_time();
				chan_entry->binder_chan_id = jpeg_chan_id;
			}
		}
	}

	if (frame != NULL || chan_entry->reenc) {
		MPP_RET ret = MPP_OK;
		RK_U32 pskip = 0;

		kmpp_frame_get_pskip_request(frame, &pskip);
		cfg_start = mpp_time();
		atomic_inc(&chan_entry->runing);
		if (frame && pskip) {
			MppPacket packet = NULL;
			struct venc_module *venc = NULL;

			venc = mpp_vcodec_get_enc_module_entry();
			ret = mpp_enc_force_pskip(chan_entry->handle, frame, &packet);
			if (packet)
				mpp_vcodec_enc_add_packet_list(chan_entry, packet);
			kmpp_frame_put(frame);
			frame = NULL;
			atomic_dec(&chan_entry->runing);
			wake_up(&chan_entry->stop_wait);
			vcodec_thread_trigger(venc->thd);
			goto __RETURN;
		} else {
			RK_U32 pskip_num = 0;

			kmpp_frame_get_pskip_num(frame, &pskip_num);
			ret = mpp_enc_cfg_reg((MppEnc)chan_entry->handle, frame);

			if (frame && pskip_num > 0) {
				kmpp_frame_get(&chan_entry->pskip_frame);
				kmpp_frame_copy(chan_entry->pskip_frame, frame);
			}
		}

		kmpp_frame_get_dts(frame, &dts);
		chan_entry->seq_encoding = dts;
		if (MPP_OK == ret) {
			if (comb_chan && comb_chan->handle) {
				atomic_inc(&comb_chan->runing);
				atomic_inc(&chan_entry->cfg.comb_runing);
				ret = mpp_enc_cfg_reg((MppEnc)comb_chan->handle, comb_frame);
				if (MPP_OK == ret) {
					ret = mpp_enc_hw_start((MppEnc)chan_entry->handle,
							       (MppEnc)comb_chan->handle);
					if (MPP_OK != ret) {
						mpp_err("combo start fail \n");
						atomic_dec(&chan_entry->cfg.comb_runing);
						atomic_dec(&comb_chan->runing);
						wake_up(&comb_chan->stop_wait);
					} else
						comb_chan->master_chan_id = chan_entry->chan_id;

				} else {
					KmppVencNtfy ntfy = mpp_enc_get_notify(comb_chan->handle);
					KmppVencNtfyImpl* ntfy_impl = (KmppVencNtfyImpl*)kmpp_obj_to_entry(ntfy);

					mpp_err("combo cfg fail \n");
					atomic_dec(&comb_chan->runing);
					atomic_dec(&chan_entry->cfg.comb_runing);
					wake_up(&comb_chan->stop_wait);

					ntfy_impl->cmd = KMPP_NOTIFY_VENC_TASK_DROP;
					ntfy_impl->drop_type = KMPP_VENC_DROP_CFG_FAILED;
					ntfy_impl->chan_id = comb_chan->chan_id;
					ntfy_impl->frame = comb_frame;
					kmpp_venc_notify(ntfy);
					if (comb_frame) {
						kmpp_frame_put(comb_frame);
						comb_frame = NULL;
					}
					ret = mpp_enc_hw_start( (MppEnc)chan_entry->handle, NULL);
				}
			} else
				ret = mpp_enc_hw_start((MppEnc)chan_entry->handle, NULL);
		}

		if (MPP_OK != ret) {
			struct venc_module *venc = NULL;

			venc = mpp_vcodec_get_enc_module_entry();
			atomic_dec(&chan_entry->runing);
			wake_up(&chan_entry->stop_wait);
			if (frame) {
				KmppVencNtfy ntfy = mpp_enc_get_notify(chan_entry->handle);
				KmppVencNtfyImpl* ntfy_impl = (KmppVencNtfyImpl*)kmpp_obj_to_entry(ntfy);

				ntfy_impl->cmd = KMPP_NOTIFY_VENC_TASK_DROP;
				ntfy_impl->drop_type = KMPP_VENC_DROP_CFG_FAILED;
				ntfy_impl->chan_id = chan_entry->chan_id;
				ntfy_impl->frame = frame;
				kmpp_venc_notify(ntfy);
				kmpp_frame_put(frame);
				frame = NULL;
			}
			if (comb_frame) {
				KmppVencNtfy ntfy = mpp_enc_get_notify(comb_chan->handle);
				KmppVencNtfyImpl* ntfy_impl = (KmppVencNtfyImpl*)kmpp_obj_to_entry(ntfy);

				ntfy_impl->cmd = KMPP_NOTIFY_VENC_TASK_DROP;
				ntfy_impl->drop_type = KMPP_VENC_DROP_CFG_FAILED;
				ntfy_impl->chan_id = comb_chan->chan_id;
				ntfy_impl->frame = comb_frame;
				kmpp_venc_notify(ntfy);
				kmpp_frame_put(comb_frame);
				comb_frame = NULL;
			}
			if (chan_entry->cfg.online)
				mpp_enc_online_task_failed(chan_entry->handle);
			vcodec_thread_trigger(venc->thd);
		}

		cfg_end = mpp_time();
		chan_entry->last_cfg_time = cfg_end - cfg_start;
	}

__RETURN:
	enc_chan_update_tab_after_enc(chan_id);

	return MPP_OK;
}

void mpp_vcodec_enc_add_packet_list(struct mpp_chan *chan_entry,
				    MppPacket packet)
{
	MppPacketImpl *p = (MppPacketImpl *) packet;
	unsigned long flags;

	spin_lock_irqsave(&chan_entry->stream_list_lock, flags);
	list_add_tail(&p->list, &chan_entry->stream_done);
	atomic_inc(&chan_entry->stream_count);
	atomic_inc(&chan_entry->pkt_total_num);
	chan_entry->seq_encoded = mpp_packet_get_dts(packet);
	spin_unlock_irqrestore(&chan_entry->stream_list_lock, flags);
	wake_up(&chan_entry->wait);

	chan_entry->reenc = 0;
}

static void mpp_vcodec_event_frame(int chan_id)
{
	MppPacket packet = NULL;
	MppPacket jpeg_packet = NULL;
	MPP_RET ret = MPP_OK;
	struct venc_module *venc = NULL;
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	struct mpp_chan *comb_entry = NULL;

	chan_entry->reenc = 1;
	venc = mpp_vcodec_get_enc_module_entry();
	if (!venc) {
		mpp_err_f("get_enc_module_entry fail");
		return;
	}
	if (atomic_read(&chan_entry->cfg.comb_runing)) {
		comb_entry = mpp_vcodec_get_chan_entry(
				     chan_entry->binder_chan_id, MPP_CTX_ENC);
		if (comb_entry && comb_entry->handle) {
			chan_entry->last_jeg_combo_end = mpp_time();
			ret = mpp_enc_int_process((MppEnc)chan_entry->handle,
						  comb_entry->handle, &packet,
						  &jpeg_packet);
			if (jpeg_packet)
				mpp_vcodec_enc_add_packet_list(comb_entry, jpeg_packet);
		}
		atomic_dec(&chan_entry->cfg.comb_runing);
		atomic_dec(&comb_entry->runing);
		wake_up(&comb_entry->stop_wait);
		chan_entry->binder_chan_id = -1;
		comb_entry->master_chan_id = -1;
	} else {
		ret = mpp_enc_int_process((MppEnc)chan_entry->handle, NULL,
					  &packet, &jpeg_packet);
	}

	if (packet)
		mpp_vcodec_enc_add_packet_list(chan_entry, packet);

	if (chan_entry->pskip_frame) {
		RK_U32 i;
		MPP_RET ret;
		RK_U32 pskip_num;
		RK_S64 pts;
		RK_S64 pts_diff = 1000000 / mpp_enc_get_fps_out(chan_entry->handle);

		kmpp_frame_get_pts(chan_entry->pskip_frame, &pts);
		kmpp_frame_get_pskip_num(chan_entry->pskip_frame, &pskip_num);
		for (i = 0; i < pskip_num; i++) {
			packet = NULL;
			kmpp_frame_set_pts(chan_entry->pskip_frame, pts + i * pts_diff);
			ret = mpp_enc_force_pskip(chan_entry->handle, chan_entry->pskip_frame, &packet);
			if (packet)
				mpp_vcodec_enc_add_packet_list(chan_entry, packet);
		}

		kmpp_frame_put(chan_entry->pskip_frame);
		chan_entry->pskip_frame = NULL;
	}

	if (ret)
		chan_entry->reenc = 0;

	atomic_dec(&chan_entry->runing);
	wake_up(&chan_entry->stop_wait);
	vcodec_thread_trigger(venc->thd);

	return;
}

static void mpp_vcodec_event_slice(int chan_id, void *param)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	MppPacket packet = NULL;

	if (atomic_read(&chan_entry->cfg.comb_runing)) {
		mpp_err("combo running fail because slice fifo.");
	}

	mpp_enc_slice_info((MppEnc)chan_entry->handle, param, &packet);
	if (packet)
		mpp_vcodec_enc_add_packet_list(chan_entry, packet);

	return;
}

void mpp_vcodec_enc_int_handle(int chan_id, int event, void *param)
{
	switch (event) {
	case MPP_VCODEC_EVENT_FRAME: {
		mpp_vcodec_event_frame(chan_id);
	} break;
	case MPP_VCODEC_EVENT_SLICE: {
		mpp_vcodec_event_slice(chan_id, param);
	} break;
	default: {
		mpp_err("mpp vcodec event %d is invalid.\n", event);
	} break;
	}

	return;
}

int mpp_vcodec_enc_run_task(RK_U32 chan_id, RK_S64 pts, RK_S64 dts)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	unsigned long lock_flag;

	if (!chan_entry)
		return -EINVAL;

	spin_lock_irqsave(&chan_entry->chan_lock, lock_flag);
	if (chan_entry->state != CHAN_STATE_RUN) {
		spin_unlock_irqrestore(&chan_entry->chan_lock, lock_flag);
		return MPP_NOK;
	}
	spin_unlock_irqrestore(&chan_entry->chan_lock, lock_flag);

	return mpp_enc_run_task(chan_entry->handle, pts, dts);
}

int mpp_vcodec_enc_set_online_mode(RK_U32 chan_id, RK_U32 mode)
{
	struct mpp_chan *chan_entry = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
	unsigned long lock_flag;

	if (!chan_entry)
		return -EINVAL;

	spin_lock_irqsave(&chan_entry->chan_lock, lock_flag);
	if (chan_entry->state != CHAN_STATE_RUN) {
		spin_unlock_irqrestore(&chan_entry->chan_lock, lock_flag);
		return MPP_NOK;
	}
	spin_unlock_irqrestore(&chan_entry->chan_lock, lock_flag);

	if (mode >= MPP_ENC_ONLINE_MODE_BUT) {
		mpp_err_f("online mode invalid %d\n", mode);
		return MPP_NOK;
	}

	chan_entry->cfg.online = mode;

	return mpp_enc_set_online_mode(chan_entry->handle, mode);
}

int mpp_vcodec_enc_routine(void *param)
{
	RK_U32 started_chan_num = 0;
	RK_U32 next_chan;
	unsigned long lock_flag;
	struct venc_module *venc = mpp_vcodec_get_enc_module_entry();

	if (!venc) {
		mpp_err_f("get_enc_module_entry fail\n");
		return MPP_ERR_VALUE;
	}
	spin_lock_irqsave(&venc->enc_lock, lock_flag);
	started_chan_num = venc->started_chan_num;
	venc->chan_pri_tab_index = 0;
	spin_unlock_irqrestore(&venc->enc_lock, lock_flag);
	mpp_vcodec_detail("mpp_vcodec_enc_routine started_chan_num %d\n",
			  started_chan_num);
	while (started_chan_num--) {
		/* get high prior chan id */
		enc_chan_get_high_prior_chan();
		next_chan = venc->curr_high_prior_chan;
		if (next_chan >= MAX_ENC_NUM)
			continue;
		if (enc_chan_process_single_chan(next_chan) != MPP_OK)
			break;
	}
	mpp_vcodec_detail("mpp_vcodec_enc_routine end\n");

	return MPP_OK;
}

void *mpp_vcodec_dec_routine(void *param)
{
	return NULL;
}
