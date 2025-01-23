/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_venc_objs"

#include <linux/kernel.h>

#include "kmpp_log.h"

#include "kmpp_obj.h"
#include "kmpp_venc_objs_impl.h"

static rk_s32 kmpp_venc_init_cfg_impl_init(void *entry, osal_fs_dev *file, const rk_u8 *caller)
{
    if (entry) {
        KmppVencInitCfgImpl *impl = (KmppVencInitCfgImpl*)entry;

        impl->type = MPP_CTX_BUTT;
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

    return rk_ok;
}

static rk_s32 kmpp_venc_init_cfg_impl_dump(void *entry)
{
    KmppVencInitCfgImpl *impl = (KmppVencInitCfgImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("type             %d\n", impl->type);
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
#define KMPP_OBJ_ENTRY_TABLE        KMPP_VENC_INIT_CFG_ENTRY_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_venc_init_cfg_impl_init
#define KMPP_OBJ_FUNC_DUMP          kmpp_venc_init_cfg_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"

/* ------------------------ kmpp notify infos ------------------------- */
static rk_s32 kmpp_venc_notify_impl_init(void *entry, osal_fs_dev *file, const rk_u8 *caller)
{
    if (entry) {
        KmppVencNtfyImpl *impl = (KmppVencNtfyImpl*)entry;

        impl->chan_id = -1;

        if (!impl->venc_syms)
            kmpp_syms_get(&impl->venc_syms, "venc");

        if (impl->venc_syms && !impl->ntfy_sym)
            kmpp_sym_get(&impl->ntfy_sym, impl->venc_syms, "venc_ntfy");
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_notify_impl_deinit(void *entry, const rk_u8 *caller)
{
    if (entry) {
        KmppVencNtfyImpl *impl = (KmppVencNtfyImpl*)entry;

        impl->chan_id = -1;

        if (impl->ntfy_sym) {
            kmpp_sym_put(impl->ntfy_sym);
            impl->ntfy_sym = NULL;
        }

        if (impl->venc_syms) {
            kmpp_syms_put(impl->venc_syms);
            impl->venc_syms = NULL;
        }
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_notify_impl_dump(void *entry)
{
    KmppVencNtfyImpl *impl = (KmppVencNtfyImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("chan_id      %d\n",      impl->chan_id);
    kmpp_logi("cmd          %d\n",      impl->cmd);
    kmpp_logi("type         %d\n",      impl->drop_type);
    kmpp_logi("pipe_id      %d\n",      impl->pipe_id);
    kmpp_logi("frame_id     %d\n",      impl->frame_id);
    kmpp_logi("frame        %p\n",      impl->frame);
    kmpp_logi("is_intra     %d\n",      impl->is_intra);
    kmpp_logi("luma_pix_sum_od %d\n",   impl->luma_pix_sum_od);
    kmpp_logi("md_index     %d\n",      impl->md_index);
    kmpp_logi("od_index     %d\n",      impl->od_index);

    return rk_ok;
}

rk_s32 kmpp_venc_notify(KmppVencNtfy ntfy)
{
    rk_s32 ret = rk_ok;
    KmppVencNtfyImpl *impl = kmpp_obj_to_entry(ntfy);

    if (!impl->ntfy_sym) {
        kmpp_err_f("the kmpp venc notify symbol is NULL\n");
        return rk_nok;
    }

    if (kmpp_sym_run(impl->ntfy_sym, ntfy, &ret))
        return rk_nok;

    return ret;
}

#define KMPP_OBJ_NAME               kmpp_venc_ntfy
#define KMPP_OBJ_INTF_TYPE          KmppVencNtfy
#define KMPP_OBJ_IMPL_TYPE          KmppVencNtfyImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_NOTIFY_CFG_ENTRY_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_venc_notify_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_venc_notify_impl_deinit
#define KMPP_OBJ_FUNC_DUMP          kmpp_venc_notify_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"
