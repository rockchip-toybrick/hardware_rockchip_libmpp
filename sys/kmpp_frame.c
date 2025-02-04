/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame"

#include <linux/kernel.h>

#include "kmpp_log.h"

#include "kmpp_obj.h"
#include "kmpp_meta.h"

#define KMPP_FRAME_KOBJ_IMPL
#include "kmpp_frame.h"

static rk_u32 kmpp_frame_debug = 0;

static rk_s32 kmpp_frame_impl_init(void *entry, osal_fs_dev *file, const rk_u8 *caller)
{
    if (entry) {
        KmppFrameImpl *impl = (KmppFrameImpl*)entry;
        KmppMeta meta = NULL;

        impl->width = 1280;
        impl->height = 720;
        impl->hor_stride = 1280;
        impl->ver_stride = 720;
        impl->hor_stride_pixel = 1280;
        impl->offset_x = 0;
        impl->offset_y = 0;
        impl->fmt = 0;
        impl->fd = -1;

        if (file)
            kmpp_meta_get_share(&meta, file, caller);
        else
            kmpp_meta_get(&meta, caller);

        kmpp_obj_to_shmptr(meta, &impl->meta);

        if (kmpp_frame_debug)
            kmpp_logi_f("frame %px meta %px : sptr %#llx:%#llx\n",
                        impl, meta, impl->meta.uaddr, impl->meta.kaddr);
    }

    return rk_ok;
}

static rk_s32 kmpp_frame_impl_deinit(void *entry, const rk_u8 *caller)
{
    if (entry) {
        KmppFrameImpl *impl = (KmppFrameImpl*)entry;
        KmppMeta meta = NULL;

        kmpp_obj_from_shmptr(&meta, &impl->meta);

        if (kmpp_frame_debug)
            kmpp_logi_f("meta %px : sptr %#llx:%#llx\n",
                        meta, impl->meta.uaddr, impl->meta.kaddr);

        if (meta)
            kmpp_meta_put_f(meta);
    }

    return rk_ok;
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
    kmpp_logi("meta [u:k]       %#llx:%#llx\n", impl->meta.uaddr, impl->meta.kaddr);

    return rk_ok;
}

static rk_s32 kmpp_frame_impl_set_buffer(void *entry, void *arg, const rk_u8 *caller)
{
    kmpp_logi_f("entry %px, arg %px caller %s\n", entry, arg, caller);
    return rk_ok;
}

static rk_s32 kmpp_frame_impl_get_buffer(void *entry, void *arg, const rk_u8 *caller)
{
    kmpp_logi_f("entry %px, arg %px caller %s\n", entry, arg, caller);
    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_frame
#define KMPP_OBJ_INTF_TYPE          KmppFrame
#define KMPP_OBJ_IMPL_TYPE          KmppFrameImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_FRAME_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE       KMPP_FRAME_STRUCT_TABLE
#define KMPP_OBJ_HOOK_TABLE         KMPP_FRAME_HOOK_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_frame_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_frame_impl_deinit
#define KMPP_OBJ_FUNC_DUMP          kmpp_frame_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"
