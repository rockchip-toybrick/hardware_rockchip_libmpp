/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FRAME_IMPL_H__
#define __KMPP_FRAME_IMPL_H__

#ifdef KMPP_FRAME_KOBJ_IMPL

#include "rk_type.h"

typedef struct KmppFrameImpl_t {
	RK_U32  width;
	RK_U32  height;
	RK_U32  hor_stride;
	RK_U32  ver_stride;
	RK_U32  hor_stride_pixel;
	RK_U32  offset_x;
	RK_U32  offset_y;
	RK_U32  fmt;
	RK_U32  fd;
	RK_U64  pts;
	RK_S32  jpeg_chan_id;
	RK_S32  mpi_buf_id;

	void    *osd_buf;
	void    *jpg_combo_osd_buf;
	RK_U32  is_gray;
	RK_U32  is_full;
	RK_U32  phy_addr;
	RK_U64  dts;
	void    *pp_info;
	/* num of pskip need to gen after normal frame */
	RK_U32	pskip_num;
	RK_U32  eos;
	RK_U32  pskip;
	RK_U32  idr_request;
} KmppFrameImpl;

#endif /* KMPP_FRAME_KOBJ_IMPL */

#endif /* __KMPP_FRAME_IMPL_H__ */
