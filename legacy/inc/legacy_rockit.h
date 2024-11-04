/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __LEGACY_ROCKIT_H__
#define __LEGACY_ROCKIT_H__

#include "rk_type.h"

typedef void* MppEnc;
typedef void* MppRoi;
typedef void* MppOsd;
typedef void* MpiBuf;
typedef void *HalBufs;
typedef void* MppPpInfo;

struct vcodec_attr {
	MppCtxType		type;
	MppCodingType	coding;
	RK_S32			chan_id;
	RK_S32			online;
	RK_U32			buf_size;
	RK_U32			max_strm_cnt;
	RK_U32			shared_buf_en;
	RK_U32      	smart_en;
	RK_U32      	max_width;
	RK_U32      	max_height;
	RK_U32      	max_lt_cnt;
	RK_U32      	qpmap_en;
	RK_U32      	chan_dup;
	RK_U32      	tmvp_enable;
	RK_U32      	only_smartp;
};

struct hal_shared_buf {
	HalBufs    dpb_bufs;
	MppBuffer  recn_ref_buf;
	MppBuffer  ext_line_buf;
	MppBuffer  stream_buf;
};

#endif /* __LEGACY_ROCKIT_H__ */
