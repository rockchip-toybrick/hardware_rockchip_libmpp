/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VENC_OBJS_IMPL_H__
#define __KMPP_VENC_OBJS_IMPL_H__

#include "kmpp_sym.h"
#include "kmpp_venc_objs.h"

typedef struct KmppVencInitCfgImpl_t {
    MppCtxType      type;
    MppCodingType   coding;
    rk_s32          chan_id;
    rk_s32          online;
    rk_u32          buf_size;
    rk_u32          max_strm_cnt;
    rk_u32          shared_buf_en;
    rk_u32          smart_en;
    rk_u32          max_width;
    rk_u32          max_height;
    rk_u32          max_lt_cnt;
    rk_u32          qpmap_en;
    rk_u32          chan_dup;
    rk_u32          tmvp_enable;
    rk_u32          only_smartp;
} KmppVencInitCfgImpl;

typedef struct KmppVencNotifyImpl_t {
    KmppSyms            venc_syms;
    KmppSym             ntfy_sym;
    rk_s32              chan_id;
    KmppNotifyCmd       cmd;
    KmppVencDropFrmType drop_type;
    rk_u32              pipe_id;
    rk_u32              frame_id;
    KmppFrame           frame;
    KmppPacket          packet;
    rk_u32              is_intra;
    /* md/od infos */
    rk_u32              luma_pix_sum_od;
    rk_u32              md_index;
    rk_u32              od_index;
} KmppVencNtfyImpl;

rk_s32 kmpp_venc_notify(KmppVencNtfy ntfy);

#endif /* __KMPP_VENC_OBJS_IMPL_H__ */
