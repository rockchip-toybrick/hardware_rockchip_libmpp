/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VENC_OBJS_H__
#define __KMPP_VENC_OBJS_H__

#include "rk_defs.h"
#include "kmpi_venc.h"

#define KMPP_VENC_INIT_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, u32, MppCtxType,      type,           ELEM_FLAG_NONE, type) \
    ENTRY(prefix, u32, MppCodingType,   coding,         ELEM_FLAG_NONE, coding) \
    ENTRY(prefix, s32, rk_s32,          chan_id,        ELEM_FLAG_NONE, chan_id) \
    ENTRY(prefix, s32, rk_s32,          online,         ELEM_FLAG_NONE, online) \
    ENTRY(prefix, u32, rk_u32,          buf_size,       ELEM_FLAG_NONE, buf_size) \
    ENTRY(prefix, u32, rk_u32,          max_strm_cnt,   ELEM_FLAG_NONE, max_strm_cnt) \
    ENTRY(prefix, u32, rk_u32,          shared_buf_en,  ELEM_FLAG_NONE, shared_buf_en) \
    ENTRY(prefix, u32, rk_u32,          smart_en,       ELEM_FLAG_NONE, smart_en) \
    ENTRY(prefix, u32, rk_u32,          max_width,      ELEM_FLAG_NONE, max_width) \
    ENTRY(prefix, u32, rk_u32,          max_height,     ELEM_FLAG_NONE, max_height) \
    ENTRY(prefix, u32, rk_u32,          max_lt_cnt,     ELEM_FLAG_NONE, max_lt_cnt) \
    ENTRY(prefix, u32, rk_u32,          qpmap_en,       ELEM_FLAG_NONE, qpmap_en) \
    ENTRY(prefix, u32, rk_u32,          chan_dup,       ELEM_FLAG_NONE, chan_dup) \
    ENTRY(prefix, u32, rk_u32,          tmvp_enable,    ELEM_FLAG_NONE, tmvp_enable) \
    ENTRY(prefix, u32, rk_u32,          only_smartp,    ELEM_FLAG_NONE, only_smartp) \
    ENTRY(prefix, u32, rk_u32,          ntfy_mode,      ELEM_FLAG_NONE, ntfy_mode)

#define KMPP_OBJ_NAME           kmpp_venc_init_cfg
#define KMPP_OBJ_INTF_TYPE      KmppVencInitCfg
#define KMPP_OBJ_ENTRY_TABLE    KMPP_VENC_INIT_CFG_ENTRY_TABLE

#include "kmpp_obj_func.h"

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

#define KMPP_NOTIFY_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, s32,  rk_s32,                 chan_id,            ELEM_FLAG_NONE, chan_id) \
    ENTRY(prefix, u32,  KmppNotifyCmd,          cmd,                ELEM_FLAG_NONE, cmd) \
    ENTRY(prefix, u32,  KmppVencDropFrmType,    drop_type,          ELEM_FLAG_NONE, drop_type) \
    ENTRY(prefix, u32,  rk_u32,                 pipe_id,            ELEM_FLAG_NONE, pipe_id) \
    ENTRY(prefix, u32,  rk_u32,                 frame_id,           ELEM_FLAG_NONE, frame_id) \
    ENTRY(prefix, kobj, KmppFrame,              frame,              ELEM_FLAG_NONE, frame) \
    ENTRY(prefix, kobj, KmppPacket,             packet,             ELEM_FLAG_NONE, packet) \
    ENTRY(prefix, u32,  rk_u32,                 luma_pix_sum_od,    ELEM_FLAG_NONE, luma_pix_sum_od) \
    ENTRY(prefix, u32,  rk_u32,                 md_index,           ELEM_FLAG_NONE, md_index) \
    ENTRY(prefix, u32,  rk_u32,                 od_index,           ELEM_FLAG_NONE, od_index) \
    ENTRY(prefix, u32,  rk_u32,                 is_intra,           ELEM_FLAG_NONE, is_intra)

#define KMPP_OBJ_NAME           kmpp_venc_ntfy
#define KMPP_OBJ_INTF_TYPE      KmppVencNtfy
#define KMPP_OBJ_ENTRY_TABLE    KMPP_NOTIFY_CFG_ENTRY_TABLE

#include "kmpp_obj_func.h"

/* ------------------------ kmpp static config ------------------------- */
#define KMPP_OBJ_NAME           kmpp_venc_st_cfg
#define KMPP_OBJ_INTF_TYPE      KmppVencStCfg

#include "kmpp_obj_func.h"

#endif /*__KMPP_VENC_OBJS_H__*/
