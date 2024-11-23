/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include "rk_type.h"

typedef enum CfgType_e {
    ENTRY_TYPE_s32,
    ENTRY_TYPE_u32,
    ENTRY_TYPE_s64,
    ENTRY_TYPE_u64,
    ENTRY_TYPE_ptr,
    ENTRY_TYPE_fp,      /* function poineter */
    ENTRY_TYPE_st,
    ENTRY_TYPE_BUTT,
} CfgType;

/* location table */
typedef union KmppLocTbl_u {
    rk_u64              val;
    struct {
        rk_u16          data_offset;
        rk_u16          data_size       : 12;
        CfgType         data_type       : 3;
        rk_u16          flag_type       : 1;
        rk_u16          flag_offset;
        rk_u16          flag_value;
    };
} KmppLocTbl;

typedef void* KmppObjDef;
typedef void* KmppObj;
typedef void (*KmppObjPreset)(void *cfg);

rk_s32 kmpp_obj_init(KmppObjDef *def, rk_u32 size, const rk_u8 *name);
rk_s32 kmpp_obj_add_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl *tbl);
rk_s32 kmpp_obj_get_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl **tbl);
rk_s32 kmpp_obj_add_preset(KmppObjDef def, KmppObjPreset preset);
rk_s32 kmpp_obj_deinit(KmppObjDef def);
rk_u32 kmpp_obj_size(KmppObjDef def);

rk_u32 kmpp_obj_lookup(KmppObjDef *def, const rk_u8 *name);
rk_u32 kmpp_obj_unref(KmppObjDef *def);

rk_s32 kmpp_obj_get(KmppObj *cfg, KmppObjDef def);
rk_s32 kmpp_obj_assign(KmppObj *cfg, KmppObjDef def, void *buf, rk_u32 size);
rk_s32 kmpp_obj_put(KmppObj cfg);
rk_s32 kmpp_obj_check(KmppObj cfg, const rk_u8 *caller);

void *kmpp_obj_to_impl(KmppObj cfg);

rk_s32 kmpp_obj_set_s32(KmppObj cfg, const rk_u8 *name, rk_s32 val);
rk_s32 kmpp_obj_get_s32(KmppObj cfg, const rk_u8 *name, rk_s32 *val);
rk_s32 kmpp_obj_set_u32(KmppObj cfg, const rk_u8 *name, rk_u32 val);
rk_s32 kmpp_obj_get_u32(KmppObj cfg, const rk_u8 *name, rk_u32 *val);
rk_s32 kmpp_obj_set_s64(KmppObj cfg, const rk_u8 *name, rk_s64 val);
rk_s32 kmpp_obj_get_s64(KmppObj cfg, const rk_u8 *name, rk_s64 *val);
rk_s32 kmpp_obj_set_u64(KmppObj cfg, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_obj_get_u64(KmppObj cfg, const rk_u8 *name, rk_u64 *val);
rk_s32 kmpp_obj_set_ptr(KmppObj cfg, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_ptr(KmppObj cfg, const rk_u8 *name, void **val);
rk_s32 kmpp_obj_set_fp(KmppObj cfg, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_fp(KmppObj cfg, const rk_u8 *name, void **val);
rk_s32 kmpp_obj_set_st(KmppObj cfg, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_st(KmppObj cfg, const rk_u8 *name, void *val);

rk_s32 kmpp_obj_tbl_set_s32(KmppObj cfg, KmppLocTbl *tbl, rk_s32 val);
rk_s32 kmpp_obj_tbl_get_s32(KmppObj cfg, KmppLocTbl *tbl, rk_s32 *val);
rk_s32 kmpp_obj_tbl_set_u32(KmppObj cfg, KmppLocTbl *tbl, rk_u32 val);
rk_s32 kmpp_obj_tbl_get_u32(KmppObj cfg, KmppLocTbl *tbl, rk_u32 *val);
rk_s32 kmpp_obj_tbl_set_s64(KmppObj cfg, KmppLocTbl *tbl, rk_s64 val);
rk_s32 kmpp_obj_tbl_get_s64(KmppObj cfg, KmppLocTbl *tbl, rk_s64 *val);
rk_s32 kmpp_obj_tbl_set_u64(KmppObj cfg, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_u64(KmppObj cfg, KmppLocTbl *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_ptr(KmppObj cfg, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_ptr(KmppObj cfg, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_fp(KmppObj cfg, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_fp(KmppObj cfg, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_st(KmppObj cfg, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj cfg, KmppLocTbl *tbl, void *val);

/* run a callback function */
rk_s32 kmpp_obj_run(KmppObj cfg, const rk_u8 *name);

#endif /* __KMPP_OBJ_H__ */