/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VENC_OBJS_IMPL_H__
#define __KMPP_VENC_OBJS_IMPL_H__

#include "rk_type.h"

#ifdef KMPP_VENC_OBJS_KOBJ_IMPL

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

#endif /* KMPP_VENC_OBJS_KOBJ_IMPL */

#endif /* __KMPP_VENC_OBJS_IMPL_H__ */
