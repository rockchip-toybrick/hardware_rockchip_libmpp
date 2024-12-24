/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VENC_OBJS_H__
#define __KMPP_VENC_OBJS_H__

#include "kmpi_venc.h"

#define ENTRY_TABLE_KMPP_VENC_INIT_CFG(ENTRY, prefix) \
    ENTRY(prefix, u32, MppCodingType, coding) \
    ENTRY(prefix, s32, rk_s32, chan_id) \
    ENTRY(prefix, s32, rk_s32, online) \
    ENTRY(prefix, u32, rk_u32, buf_size) \
    ENTRY(prefix, u32, rk_u32, max_strm_cnt) \
    ENTRY(prefix, u32, rk_u32, shared_buf_en) \
    ENTRY(prefix, u32, rk_u32, smart_en) \
    ENTRY(prefix, u32, rk_u32, max_width) \
    ENTRY(prefix, u32, rk_u32, max_height) \
    ENTRY(prefix, u32, rk_u32, max_lt_cnt) \
    ENTRY(prefix, u32, rk_u32, qpmap_en) \
    ENTRY(prefix, u32, rk_u32, chan_dup) \
    ENTRY(prefix, u32, rk_u32, tmvp_enable) \
    ENTRY(prefix, u32, rk_u32, only_smartp)

#if 0
rk_s32 kmpp_venc_init_cfg_init(void);
rk_s32 kmpp_venc_init_cfg_deinit(void);

rk_s32 kmpp_venc_init_cfg_get(KmppVencInitCfg *cfg);
rk_s32 kmpp_venc_init_cfg_get_share(KmppVencInitCfg *cfg, KmppShmGrp grp);
rk_s32 kmpp_venc_init_cfg_assign(KmppVencInitCfg *cfg, void *buf, rk_s32 size);
rk_s32 kmpp_venc_init_cfg_put(KmppVencInitCfg cfg);

rk_s32 kmpp_venc_init_cfg_get_coding(KmppVencInitCfg cfg, MppCodingType *coding);
rk_s32 kmpp_venc_init_cfg_set_width(KmppVencInitCfg cfg, MppCodingType coding);
rk_s32 kmpp_venc_init_cfg_get_chan_id(KmppVencInitCfg cfg, rk_s32 *chan_id);
rk_s32 kmpp_venc_init_cfg_set_chan_id(KmppVencInitCfg cfg, rk_s32 chan_id);
rk_s32 kmpp_venc_init_cfg_get_online(KmppVencInitCfg cfg, rk_s32 *online);
rk_s32 kmpp_venc_init_cfg_set_online(KmppVencInitCfg cfg, rk_s32 online);
rk_s32 kmpp_venc_init_cfg_get_buf_size(KmppVencInitCfg cfg, rk_u32 *buf_size);
rk_s32 kmpp_venc_init_cfg_set_buf_size(KmppVencInitCfg cfg, rk_u32 buf_size);
rk_s32 kmpp_venc_init_cfg_get_max_strm_cnt(KmppVencInitCfg cfg, rk_u32 *max_strm_cnt);
rk_s32 kmpp_venc_init_cfg_set_max_strm_cnt(KmppVencInitCfg cfg, rk_u32 max_strm_cnt);
rk_s32 kmpp_venc_init_cfg_get_shared_buf_en(KmppVencInitCfg cfg, rk_u32 *shared_buf_en);
rk_s32 kmpp_venc_init_cfg_set_shared_buf_en(KmppVencInitCfg cfg, rk_u32 shared_buf_en);
rk_s32 kmpp_venc_init_cfg_get_smart_en(KmppVencInitCfg cfg, rk_u32 *smart_en);
rk_s32 kmpp_venc_init_cfg_set_smart_en(KmppVencInitCfg cfg, rk_u32 smart_en);
rk_s32 kmpp_venc_init_cfg_get_max_width(KmppVencInitCfg cfg, rk_u32 *max_width);
rk_s32 kmpp_venc_init_cfg_set_max_width(KmppVencInitCfg cfg, rk_u32 max_width);
rk_s32 kmpp_venc_init_cfg_get_max_height(KmppVencInitCfg cfg, rk_s32 *max_height);
rk_s32 kmpp_venc_init_cfg_set_max_height(KmppVencInitCfg cfg, rk_s32 max_height);
rk_s32 kmpp_venc_init_cfg_get_max_lt_cnt(KmppVencInitCfg cfg, rk_u32 *max_lt_cnt);
rk_s32 kmpp_venc_init_cfg_set_max_lt_cnt(KmppVencInitCfg cfg, rk_u32 max_lt_cnt);
rk_s32 kmpp_venc_init_cfg_get_qpmap_en(KmppVencInitCfg cfg, rk_u32 *qpmap_en);
rk_s32 kmpp_venc_init_cfg_set_qpmap_en(KmppVencInitCfg cfg, rk_u32 qpmap_en);
rk_s32 kmpp_venc_init_cfg_get_chan_dup(KmppVencInitCfg cfg, rk_u32 *chan_dup);
rk_s32 kmpp_venc_init_cfg_set_chan_dup(KmppVencInitCfg cfg, rk_u32 chan_dup);
rk_s32 kmpp_venc_init_cfg_get_tmvp_enable(KmppVencInitCfg cfg, rk_u32 *tmvp_enable);
rk_s32 kmpp_venc_init_cfg_set_tmvp_enable(KmppVencInitCfg cfg, rk_u32 tmvp_enable);
rk_s32 kmpp_venc_init_cfg_get_only_smartp(KmppVencInitCfg cfg, rk_u32 *only_smartp);
rk_s32 kmpp_venc_init_cfg_set_only_smartp(KmppVencInitCfg cfg, rk_u32 only_smartp);

#else

#define KMPP_OBJ_NAME           kmpp_venc_init_cfg
#define KMPP_OBJ_INTF_TYPE      KmppVencInitCfg
#define KMPP_OBJ_ENTRY_TABLE    ENTRY_TABLE_KMPP_VENC_INIT_CFG

#include "kmpp_obj_func.h"

#endif

void kmpp_venc_init_cfg_dump(KmppVencInitCfg cfg, const rk_u8 *caller);
#define kmpp_venc_init_cfg_dump_f(cfg) kmpp_venc_init_cfg_dump(cfg, __func__)

#include "kmpp_venc_objs_impl.h"

#endif /*__KMPP_VENC_OBJS_H__*/
