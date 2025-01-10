/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPI_DEFS_H__
#define __KMPI_DEFS_H__

#include "rk_type.h"

/* context pointer and integer id */
typedef rk_s32 KmppChanId;
typedef void* KmppCtx;

/* data flow structure */
typedef void* KmppPacket;
typedef void* KmppFrame;
typedef void* KmppBuffer;
typedef void* KmppPacketBulk;
typedef void* KmppFrameBulk;

#endif /*__KMPI_DEFS_H__*/
