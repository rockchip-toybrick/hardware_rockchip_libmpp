/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_venc_objs"

#include <linux/kernel.h>

#include "kmpp_log.h"

#include "kmpp_obj.h"
#include "kmpp_shm.h"

#define KMPP_VENC_OBJS_KOBJ_IMPL
#include "kmpp_venc_objs.h"

static void kmpp_venc_init_cfg_impl_preset(void *entry)
{
    if (entry) {
        KmppVencInitCfgImpl *impl = (KmppVencInitCfgImpl*)entry;

        impl->coding = MPP_VIDEO_CodingUnused;
        impl->chan_id = -1;
        impl->online = 0;
        impl->buf_size = 0;
        impl->max_strm_cnt = 0;
        impl->shared_buf_en = 0;
        impl->smart_en = 0;
        impl->max_width = 0;
        impl->max_height = 0;
        impl->max_lt_cnt = 0;
        impl->qpmap_en = 0;
        impl->chan_dup = 0;
        impl->tmvp_enable = 0;
        impl->only_smartp = 0;
    }
}

static rk_s32 kmpp_venc_init_cfg_impl_dump(void *entry)
{
    KmppVencInitCfgImpl *impl = (KmppVencInitCfgImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("coding           %d\n", impl->coding);
    kmpp_logi("chan_id          %d\n", impl->chan_id);
    kmpp_logi("online           %d\n", impl->online);
    kmpp_logi("buf_size         %d\n", impl->buf_size);
    kmpp_logi("max_strm_cnt     %d\n", impl->max_strm_cnt);
    kmpp_logi("shared_buf_en    %d\n", impl->shared_buf_en);
    kmpp_logi("smart_en         %d\n", impl->smart_en);
    kmpp_logi("max_width        %d\n", impl->max_width);
    kmpp_logi("max_height       %d\n", impl->max_height);
    kmpp_logi("max_lt_cnt       %d\n", impl->max_lt_cnt);
    kmpp_logi("qpmap_en         %d\n", impl->qpmap_en);
    kmpp_logi("chan_dup         %d\n", impl->chan_dup);
    kmpp_logi("tmvp_enable      %d\n", impl->tmvp_enable);
    kmpp_logi("only_smartp      %d\n", impl->only_smartp);

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_venc_init_cfg
#define KMPP_OBJ_INTF_TYPE          KmppVencInitCfg
#define KMPP_OBJ_IMPL_TYPE          KmppVencInitCfgImpl
#define KMPP_OBJ_ENTRY_TABLE        ENTRY_TABLE_KMPP_VENC_INIT_CFG
#define KMPP_OBJ_FUNC_PRESET        kmpp_venc_init_cfg_impl_preset
#define KMPP_OBJ_FUNC_DUMP          kmpp_venc_init_cfg_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHM_IN_OBJS
#include "kmpp_obj_helper.h"
