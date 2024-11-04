/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_CFG_H__
#define __KMPP_CFG_H__

#include "rk_type.h"

typedef enum CfgType_e {
    CFG_TYPE_s32,
    CFG_TYPE_u32,
    CFG_TYPE_s64,
    CFG_TYPE_u64,
    CFG_TYPE_ptr,
    CFG_TYPE_st,
    CFG_TYPE_BUTT,
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

#define FIELD1_TO_LOCTBL_FLAG(type, field1, cfgtype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(type, field1), \
        .data_size = sizeof(((type *)0)->field1), \
        .data_type = CFG_TYPE_##cfgtype, \
        .flag_type = flag_value ? 1 : 0, \
        .flag_offset = offsetof(type, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD1_TO_LOCTBL_ACCESS(type, field1, cfgtype) \
    { \
        .data_offset = offsetof(type, field1), \
        .data_size = sizeof(((type *)0)->field1), \
        .data_type = CFG_TYPE_##cfgtype, \
        .flag_type = 0, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

#define FIELD2_TO_LOCTBL_FLAG(type, field1, field2, cfgtype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(type, field1.field2), \
        .data_size = sizeof(((type *)0)->field1.field2), \
        .data_type = CFG_TYPE_##cfgtype, \
        .flag_type = flag_value ? 1 : 0, \
        .flag_offset = offsetof(type, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD2_TO_LOCTBL_ACCESS(type, field1, field2, cfgtype) \
    { \
        .data_offset = offsetof(type, field1.field2), \
        .data_size = sizeof(((type *)0)->field1.field2), \
        .data_type = CFG_TYPE_##cfgtype, \
        .flag_type = 0, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

typedef void* KmppCfgDef;
typedef void* KmppCfg;
typedef void (*KmppCfgPreset)(void *cfg);
typedef rk_s32 (*KmppModuleFunc)(void *args);

rk_s32 kmpp_cfg_init(KmppCfgDef *def, rk_u32 size, const rk_u8 *name);
rk_s32 kmpp_cfg_add_entry(KmppCfgDef def, const rk_u8 *name, KmppLocTbl *tbl);
rk_s32 kmpp_cfg_add_preset(KmppCfgDef def, KmppCfgPreset preset);
rk_s32 kmpp_cfg_add_func(KmppCfgDef def, KmppModuleFunc func);
rk_s32 kmpp_cfg_deinit(KmppCfgDef def);
rk_u32 kmpp_cfg_size(KmppCfgDef def);

rk_u32 kmpp_cfg_lookup(KmppCfgDef *def, const rk_u8 *name);
rk_u32 kmpp_cfg_unref(KmppCfgDef *def);

rk_s32 kmpp_cfg_get(KmppCfg *cfg, KmppCfgDef def);
rk_s32 kmpp_cfg_assign(KmppCfg *cfg, KmppCfgDef def, void *buf, rk_u32 size);
rk_s32 kmpp_cfg_put(KmppCfg cfg);
void *kmpp_cfg_to_impl(KmppCfg cfg);

rk_s32 kmpp_cfg_set_s32(KmppCfg cfg, const rk_u8 *name, rk_s32 val);
rk_s32 kmpp_cfg_get_s32(KmppCfg cfg, const rk_u8 *name, rk_s32 *val);
rk_s32 kmpp_cfg_set_u32(KmppCfg cfg, const rk_u8 *name, rk_u32 val);
rk_s32 kmpp_cfg_get_u32(KmppCfg cfg, const rk_u8 *name, rk_u32 *val);
rk_s32 kmpp_cfg_set_s64(KmppCfg cfg, const rk_u8 *name, rk_s64 val);
rk_s32 kmpp_cfg_get_s64(KmppCfg cfg, const rk_u8 *name, rk_s64 *val);
rk_s32 kmpp_cfg_set_u64(KmppCfg cfg, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_cfg_get_u64(KmppCfg cfg, const rk_u8 *name, rk_u64 *val);
rk_s32 kmpp_cfg_set_ptr(KmppCfg cfg, const rk_u8 *name, void *val);
rk_s32 kmpp_cfg_get_ptr(KmppCfg cfg, const rk_u8 *name, void **val);
rk_s32 kmpp_cfg_set_st(KmppCfg cfg, const rk_u8 *name, void *val);
rk_s32 kmpp_cfg_get_st(KmppCfg cfg, const rk_u8 *name, void *val);

/* run callback function */
rk_s32 kmpp_cfg_run(KmppCfg cfg, const rk_u8 *name);

#endif /* __KMPP_CFG_H__ */