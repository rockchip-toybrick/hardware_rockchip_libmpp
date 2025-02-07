/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FRAME_IMPL_H__
#define __KMPP_FRAME_IMPL_H__

#include "kmpp_frame.h"

typedef struct KmppFrameImpl_t {
	rk_u32  width;
	rk_u32  height;
	rk_u32  hor_stride;
	rk_u32  ver_stride;
	rk_u32  hor_stride_pixel;
	rk_u32  offset_x;
	rk_u32  offset_y;
	rk_u32  fmt;
	rk_u32  fd;
	rk_u64  pts;
	rk_s32  jpeg_chan_id;
	rk_s32  mpi_buf_id;

	void    *osd_buf;
	void    *jpg_combo_osd_buf;
	rk_u32  is_gray;
	rk_u32  is_full;
	rk_u32  phy_addr;
	rk_u64  dts;
	void    *pp_info;
	/* num of pskip need to gen after normal frame */
	rk_u32	pskip_num;
	rk_u32  eos;
	rk_u32  pskip;
	rk_u32  idr_request;

	KmppShmPtr meta;

	/* hook callback element */
	KmppShmPtr buffer;
} KmppFrameImpl;

#endif /* __KMPP_FRAME_IMPL_H__ */
