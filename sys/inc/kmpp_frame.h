/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FRAME_H__
#define __KMPP_FRAME_H__

#include "kmpi_defs.h"

void kmpp_frame_dump(KmppFrame frame, const rk_u8 *caller);
#define kmpp_frame_dump_f(frame) kmpp_frame_dump(frame, __func__)

#define ENTRY_TABLE_KMPP_FRAME(ENTRY, prefix) \
    ENTRY(prefix, u32, rk_u32, width) \
    ENTRY(prefix, u32, rk_u32, height) \
    ENTRY(prefix, u32, rk_u32, hor_stride) \
    ENTRY(prefix, u32, rk_u32, ver_stride) \
    ENTRY(prefix, u32, rk_u32, hor_stride_pixel) \
    ENTRY(prefix, u32, rk_u32, offset_x) \
    ENTRY(prefix, u32, rk_u32, offset_y) \
    ENTRY(prefix, u32, rk_u32, fmt) \
    ENTRY(prefix, s32, rk_s32, fd)

#define KMPP_OBJ_NAME           kmpp_frame
#define KMPP_OBJ_INTF_TYPE      KmppFrame
#define KMPP_OBJ_ENTRY_TABLE    ENTRY_TABLE_KMPP_FRAME
#include "kmpp_obj_func.h"

#include "kmpp_frame_impl.h"

#endif /*__KMPP_FRAME_H__*/
