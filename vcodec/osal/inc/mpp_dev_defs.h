// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#ifndef __MPP_DEV_DEFS_H__
#define __MPP_DEV_DEFS_H__

#include "rk_type.h"

/*
 * Platform video codec hardware feature
 */
typedef enum MppClientType_e {
	VPU_CLIENT_VDPU1        = 0,    /* 0x00000001 */
	VPU_CLIENT_VDPU2        = 1,    /* 0x00000002 */
	VPU_CLIENT_VDPU1_PP     = 2,    /* 0x00000004 */
	VPU_CLIENT_VDPU2_PP     = 3,    /* 0x00000008 */
	VPU_CLIENT_AV1DEC       = 4,    /* 0x00000010 */

	VPU_CLIENT_HEVC_DEC     = 8,    /* 0x00000100 */
	VPU_CLIENT_RKVDEC       = 9,    /* 0x00000200 */
	VPU_CLIENT_AVSPLUS_DEC  = 12,   /* 0x00001000 */
	VPU_CLIENT_JPEG_DEC     = 13,   /* 0x00002000 */

	VPU_CLIENT_RKVENC       = 16,   /* 0x00010000 */
	VPU_CLIENT_VEPU1        = 17,   /* 0x00020000 */
	VPU_CLIENT_VEPU2        = 18,   /* 0x00040000 */
	VPU_CLIENT_VEPU2_JPEG   = 19,   /* 0x00080000 */
	VPU_CLIENT_JPEG_ENC     = 20,   /* 0x00100000 */

	VPU_CLIENT_VEPU22       = 24,   /* 0x01000000 */

	IEP_CLIENT_TYPE         = 28,   /* 0x10000000 */
	VDPP_CLIENT_TYPE        = 29,   /* 0x20000000 */

	VPU_CLIENT_BUTT,
} MppClientType;

/* RK standalone encoder */
#define HAVE_RKVENC         (1 << VPU_CLIENT_RKVENC)	/* 0x00010000 */

#endif /*__MPP_DEV_DEFS_H__*/
