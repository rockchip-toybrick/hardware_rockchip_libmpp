/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame"

#include <linux/kernel.h>

#include "kmpp_log.h"

#include "kmpp_obj.h"
#include "kmpp_shm.h"

#define KMPP_FRAME_KOBJ_IMPL
#include "kmpp_frame.h"

#define KMPP_OBJ_IMPL_TYPE          KmppFrameImpl
#define KMPP_OBJ_IMPL_DEF           kmpp_frame_def
#include "kmpp_obj_helper.h"

static KmppObjDef kmpp_frame_def = NULL;

#define KMPP_FRAME_ACCESSORS(ftype, type, field) \
    static KmppLocTbl *tbl_##field = NULL; \
    rk_s32 kmpp_frame_get_##field(MppFrame s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##field, v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_get_entry(s))->field; \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_frame_get_##field); \
    rk_s32 kmpp_frame_set_##field(MppFrame s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##field) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##field, v); \
        else \
            ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_get_entry(s))->field = v; \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_frame_set_##field);

ENTRY_TABLE_KMPP_FRAME(KMPP_FRAME_ACCESSORS)

#define ENTRY_QUERY(ftype, type, f1) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_IMPL_DEF, #f1, &tbl_##f1); \
    } while (0);

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

rk_s32 kmpp_frame_init(void)
{
    rk_s32 impl_size = sizeof(KMPP_OBJ_IMPL_TYPE);

    kmpp_objdef_init(&kmpp_frame_def, impl_size, "kmpp_frame");

    if (!kmpp_frame_def) {
        kmpp_loge_f("kmpp_frame_def init failed\n");
        return rk_nok;
    }

    ENTRY_TABLE_KMPP_FRAME(ENTRY_TO_TRIE1)
    kmpp_objdef_add_entry(kmpp_frame_def, NULL, NULL);
    kmpp_objdef_add_preset(kmpp_frame_def, kmpp_frame_impl_preset);
    kmpp_objdef_add_dump(kmpp_frame_def, kmpp_frame_impl_dump);

    ENTRY_TABLE_KMPP_FRAME(ENTRY_QUERY)

    return kmpp_objdef_add_shm_mgr(kmpp_frame_def);
}

rk_s32 kmpp_frame_deinit(void)
{
    return kmpp_objdef_deinit(kmpp_frame_def);
}

rk_s32 kmpp_frame_size(void)
{
    return kmpp_objdef_get_buf_size(kmpp_frame_def);
}
EXPORT_SYMBOL(kmpp_frame_size);

rk_s32 kmpp_frame_get(MppFrame *frame)
{
    return kmpp_obj_get((KmppObj *)frame, kmpp_frame_def);
}
EXPORT_SYMBOL(kmpp_frame_get);

rk_s32 kmpp_frame_get_share(MppFrame *frame, KmppShmGrp grp)
{
    return kmpp_obj_get_share((KmppObj *)frame, kmpp_frame_def, grp);
}
EXPORT_SYMBOL(kmpp_frame_get_share);

rk_s32 kmpp_frame_assign(MppFrame *frame, void *buf, rk_s32 size)
{
    return kmpp_obj_assign((KmppObj *)frame, kmpp_frame_def, buf, size);
}
EXPORT_SYMBOL(kmpp_frame_assign);

rk_s32 kmpp_frame_put(MppFrame frame)
{
    return kmpp_obj_put(frame);
}
EXPORT_SYMBOL(kmpp_frame_put);

void kmpp_frame_dump(MppFrame frame, const rk_u8 *caller)
{
    kmpp_obj_dump(frame, caller);
}
EXPORT_SYMBOL(kmpp_frame_dump);
