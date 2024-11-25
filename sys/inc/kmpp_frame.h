/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FRAME_H__
#define __KMPP_FRAME_H__

#include "rk_type.h"
#include "kmpp_shm.h"

rk_s32 kmpp_frame_init(void);
rk_s32 kmpp_frame_deinit(void);

rk_s32 kmpp_frame_get(MppFrame *frame);
rk_s32 kmpp_frame_get_share(MppFrame *frame, KmppShmGrp grp);
rk_s32 kmpp_frame_assign(MppFrame *frame, void *buf, rk_s32 size);
rk_s32 kmpp_frame_put(MppFrame frame);
void kmpp_frame_dump(MppFrame frame, const rk_u8 *caller);
#define kmpp_frame_dump_f(frame) kmpp_frame_dump(frame, __func__)

#define ENTRY_TABLE_KMPP_FRAME(ENTRY) \
    ENTRY(u32, rk_u32, width) \
    ENTRY(u32, rk_u32, height) \
    ENTRY(u32, rk_u32, hor_stride) \
    ENTRY(u32, rk_u32, ver_stride) \
    ENTRY(u32, rk_u32, hor_stride_pixel) \
    ENTRY(u32, rk_u32, offset_x) \
    ENTRY(u32, rk_u32, offset_y) \
    ENTRY(u32, rk_u32, fmt) \
    ENTRY(s32, rk_s32, fd)

#if 0

rk_s32 kmpp_frame_get_width(MppFrame frame, rk_u32 *width);
rk_s32 kmpp_frame_set_width(MppFrame frame, rk_u32 width);
rk_s32 kmpp_frame_get_height(MppFrame frame, rk_u32 *height);
rk_s32 kmpp_frame_set_height(MppFrame frame, rk_u32 height);
rk_s32 kmpp_frame_get_hor_stride(MppFrame frame, rk_u32 *hor_stride);
rk_s32 kmpp_frame_set_hor_stride(MppFrame frame, rk_u32 hor_stride);
rk_s32 kmpp_frame_get_ver_stride(MppFrame frame, rk_u32 *ver_stride);
rk_s32 kmpp_frame_set_ver_stride(MppFrame frame, rk_u32 ver_stride);
rk_s32 kmpp_frame_get_hor_stride_pixel(MppFrame frame, rk_u32 *hor_stride_pixel);
rk_s32 kmpp_frame_set_hor_stride_pixel(MppFrame frame, rk_u32 hor_stride_pixel);
rk_s32 kmpp_frame_get_offset_x(MppFrame frame, rk_u32 *offset_x);
rk_s32 kmpp_frame_set_offset_x(MppFrame frame, rk_u32 offset_x);
rk_s32 kmpp_frame_get_offset_y(MppFrame frame, rk_u32 *offset_y);
rk_s32 kmpp_frame_set_offset_y(MppFrame frame, rk_u32 offset_y);

#else

#define KMPP_OBJ_FUNC_PREFIX    kmpp_frame
#define KMPP_OBJ_INTF_TYPE      MppFrame
#define KMPP_OBJ_ENTRY_TABLE    ENTRY_TABLE_KMPP_FRAME

#include "kmpp_obj_func.h"

#endif

#include "kmpp_frame_impl.h"

#endif /*__KMPP_FRAME_H__*/
