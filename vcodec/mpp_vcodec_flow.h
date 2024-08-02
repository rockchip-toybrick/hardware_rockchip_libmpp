/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 */
#ifndef __ROCKCHIP_MPP_VCODEC_FLOW_H__
#define __ROCKCHIP_MPP_VCODEC_FLOW_H__

enum mpp_vcodec_event_type {
	MPP_VCODEC_EVENT_FRAME,
	MPP_VCODEC_EVENT_SLICE,
	MPP_VCODEC_EVENT_BUTT,
};

int mpp_vcodec_enc_routine(void *param);
void *mpp_vcodec_dec_routine(void *param);
void mpp_vcodec_enc_int_handle(int chan_id, int event, void *param);
int mpp_vcodec_enc_run_task(RK_U32 chan_id, RK_S64 pts, RK_S64 dts);
int mpp_vcodec_enc_set_online_mode(RK_U32 chan_id, RK_U32 mode);

#endif
