/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VENC_OBJS_H__
#define __KMPP_VENC_OBJS_H__

#include "rk_defs.h"
#include "kmpi_venc.h"

#define KMPP_VENC_INIT_CFG_ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, u32, MppCtxType, type) \
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
#define KMPP_OBJ_ENTRY_TABLE    KMPP_VENC_INIT_CFG_ENTRY_TABLE

#include "kmpp_obj_func.h"

#endif

#define kmpp_venc_init_cfg_dump_f(cfg) kmpp_venc_init_cfg_dump(cfg, __FUNCTION__)

/* ------------------------ kmpp notify infos ------------------------- */
/* notify function config */
typedef void* KmppVencNtfy;

typedef enum KmppNotifyCmd_e {
    KMPP_NOTIFY_NONE = 0,
    /* notify frame info when enc done */
    KMPP_NOTIFY_VENC_TASK_DONE,
    /* notify drop info when enc failed */
    KMPP_NOTIFY_VENC_TASK_DROP,

    KMPP_NOTIFY_VENC_WRAP = 0x100,
    /* notiy to get pipe id / frame id for wrap task */
    KMPP_NOTIFY_VENC_GET_WRAP_TASK_ID,
    /* notify reg stask ready wrap task */
    KMPP_NOTIFY_VENC_WRAP_TASK_READY,
    /* notify source id mismatch for wrap task */
    KMPP_NOTIFY_VENC_SOURCE_ID_MISMATCH,

    KMPP_NOTIFY_IVS = 0x1000,
    /* notify md/od task done */
    KMPP_NOTIFY_IVS_TASK_DONE,
} KmppNotifyCmd;

typedef enum KmppVencDropFrmType_e {
    /* drop frame cause by cfg failed */
    KMPP_VENC_DROP_CFG_FAILED,
    /* drop frame cause by enc failed */
    KMPP_VENC_DROP_ENC_FAILED,
} KmppVencDropFrmType;

#define KMPP_NOTIFY_CFG_ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, s32,  rk_s32,                 chan_id) \
    ENTRY(prefix, u32,  KmppNotifyCmd,          cmd) \
    ENTRY(prefix, u32,  KmppVencDropFrmType,    drop_type) \
    ENTRY(prefix, u32,  rk_u32,                 pipe_id) \
    ENTRY(prefix, u32,  rk_u32,                 frame_id) \
    ENTRY(prefix, kobj, KmppFrame,              frame) \
    ENTRY(prefix, kobj, KmppPacket,             packet) \
    ENTRY(prefix, u32,  rk_u32,                 luma_pix_sum_od) \
    ENTRY(prefix, u32,  rk_u32,                 md_index) \
    ENTRY(prefix, u32,  rk_u32,                 od_index) \
    ENTRY(prefix, u32,  rk_u32,                 is_intra)

#define KMPP_OBJ_NAME           kmpp_venc_ntfy
#define KMPP_OBJ_INTF_TYPE      KmppVencNtfy
#define KMPP_OBJ_ENTRY_TABLE    KMPP_NOTIFY_CFG_ENTRY_TABLE

#include "kmpp_obj_func.h"

#endif /*__KMPP_VENC_OBJS_H__*/
