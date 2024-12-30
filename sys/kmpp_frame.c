/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame"

#include <linux/kernel.h>

#include "kmpp_log.h"

#include "kmpp_obj.h"

#define KMPP_FRAME_KOBJ_IMPL
#include "kmpp_frame.h"

static void kmpp_frame_impl_preset(void *entry)
{
    if (entry) {
        KmppFrameImpl *impl = (KmppFrameImpl*)entry;

        impl->width = 1280;
        impl->height = 720;
        impl->hor_stride = 1280;
        impl->ver_stride = 720;
        impl->hor_stride_pixel = 1280;
        impl->offset_x = 0;
        impl->offset_y = 0;
        impl->fmt = 0;
        impl->fd = -1;
    }
}

static rk_s32 kmpp_frame_impl_dump(void *entry)
{
    KmppFrameImpl *impl = (KmppFrameImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("width            %d\n", impl->width);
    kmpp_logi("height           %d\n", impl->height);
    kmpp_logi("hor_stride       %d\n", impl->hor_stride);
    kmpp_logi("ver_stride       %d\n", impl->ver_stride);
    kmpp_logi("hor_stride_pixel %d\n", impl->hor_stride_pixel);
    kmpp_logi("offset_x         %d\n", impl->offset_x);
    kmpp_logi("offset_y         %d\n", impl->offset_y);
    kmpp_logi("fmt              %d\n", impl->fmt);
    kmpp_logi("fd               %d\n", impl->fd);

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_frame
#define KMPP_OBJ_INTF_TYPE          KmppFrame
#define KMPP_OBJ_IMPL_TYPE          KmppFrameImpl
#define KMPP_OBJ_ENTRY_TABLE        ENTRY_TABLE_KMPP_FRAME
#define KMPP_OBJ_FUNC_PRESET        kmpp_frame_impl_preset
#define KMPP_OBJ_FUNC_DUMP          kmpp_frame_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHM_STAND_ALONE
#include "kmpp_obj_helper.h"
