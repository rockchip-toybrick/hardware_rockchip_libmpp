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
	VPU_CLIENT_RKVENC = 1,	/* 0x00010000 */
	VPU_CLIENT_BUTT,
} MppClientType;

/* RK standalone encoder */
#define HAVE_RKVENC         (1 << VPU_CLIENT_RKVENC)	/* 0x00010000 */

#endif /*__MPP_DEV_DEFS_H__*/
