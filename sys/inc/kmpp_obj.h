/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include "kmpp_sys_defs.h"
#include "kmpp_trie.h"

typedef enum EntryType_e {
    ENTRY_TYPE_s32,
    ENTRY_TYPE_u32,
    ENTRY_TYPE_s64,
    ENTRY_TYPE_u64,
    ENTRY_TYPE_ptr,
    ENTRY_TYPE_fp,      /* function poineter */
    ENTRY_TYPE_st,
    ENTRY_TYPE_BUTT,
} EntryType;

/* location table */
typedef union KmppLocTbl_u {
    rk_u64              val;
    struct {
        rk_u16          data_offset;
        rk_u16          data_size       : 12;
        EntryType       data_type       : 3;
        rk_u16          flag_type       : 1;
        rk_u16          flag_offset;
        rk_u16          flag_value;
    };
} KmppLocTbl;

typedef void (*KmppObjPreset)(void *obj);
typedef rk_s32 (*KmppObjDump)(void *entry);

rk_s32 kmpp_objdef_init(KmppObjDef *def, rk_s32 size, const rk_u8 *name);
rk_s32 kmpp_objdef_add_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl *tbl);
rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl **tbl);
rk_s32 kmpp_objdef_add_preset(KmppObjDef def, KmppObjPreset preset);
rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump);
rk_s32 kmpp_objdef_add_shm_mgr(KmppObjDef def);
rk_s32 kmpp_objdef_deinit(KmppObjDef def);
rk_u32 kmpp_objdef_lookup(KmppObjDef *def, const rk_u8 *name);

/* kmpp objcet internal element set / get function */
const rk_u8 *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_buf_size(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
KmppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* normal kernel object allocator both object head and body */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def);
/* use external buffer for both object head and body */
rk_s32 kmpp_obj_assign(KmppObj *obj, KmppObjDef def, void *buf, rk_s32 size);
/* import external shared buffer for object body */
rk_s32 kmpp_obj_get_share(KmppObj *obj, KmppObjDef def, KmppShmGrp grp);
rk_s32 kmpp_obj_put(KmppObj obj);
rk_s32 kmpp_obj_check(KmppObj obj, const rk_u8 *caller);

void *kmpp_obj_get_entry(KmppObj obj);
KmppShm kmpp_obj_get_shm(KmppObj obj);

rk_s32 kmpp_obj_set_s32(KmppObj obj, const rk_u8 *name, rk_s32 val);
rk_s32 kmpp_obj_get_s32(KmppObj obj, const rk_u8 *name, rk_s32 *val);
rk_s32 kmpp_obj_set_u32(KmppObj obj, const rk_u8 *name, rk_u32 val);
rk_s32 kmpp_obj_get_u32(KmppObj obj, const rk_u8 *name, rk_u32 *val);
rk_s32 kmpp_obj_set_s64(KmppObj obj, const rk_u8 *name, rk_s64 val);
rk_s32 kmpp_obj_get_s64(KmppObj obj, const rk_u8 *name, rk_s64 *val);
rk_s32 kmpp_obj_set_u64(KmppObj obj, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_obj_get_u64(KmppObj obj, const rk_u8 *name, rk_u64 *val);
rk_s32 kmpp_obj_set_ptr(KmppObj obj, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_ptr(KmppObj obj, const rk_u8 *name, void **val);
rk_s32 kmpp_obj_set_fp(KmppObj obj, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_fp(KmppObj obj, const rk_u8 *name, void **val);
rk_s32 kmpp_obj_set_st(KmppObj obj, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_st(KmppObj obj, const rk_u8 *name, void *val);

rk_s32 kmpp_obj_tbl_set_s32(KmppObj obj, KmppLocTbl *tbl, rk_s32 val);
rk_s32 kmpp_obj_tbl_get_s32(KmppObj obj, KmppLocTbl *tbl, rk_s32 *val);
rk_s32 kmpp_obj_tbl_set_u32(KmppObj obj, KmppLocTbl *tbl, rk_u32 val);
rk_s32 kmpp_obj_tbl_get_u32(KmppObj obj, KmppLocTbl *tbl, rk_u32 *val);
rk_s32 kmpp_obj_tbl_set_s64(KmppObj obj, KmppLocTbl *tbl, rk_s64 val);
rk_s32 kmpp_obj_tbl_get_s64(KmppObj obj, KmppLocTbl *tbl, rk_s64 *val);
rk_s32 kmpp_obj_tbl_set_u64(KmppObj obj, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_u64(KmppObj obj, KmppLocTbl *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_ptr(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_ptr(KmppObj obj, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_fp(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_fp(KmppObj obj, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppLocTbl *tbl, void *val);

/* run a callback function */
rk_s32 kmpp_obj_run(KmppObj obj, const rk_u8 *name);
rk_s32 kmpp_obj_dump(KmppObj obj, const rk_u8 *caller);

#define kmpp_obj_dump_f(obj) kmpp_obj_dump(obj, __FUNCTION__)

#endif /* __KMPP_OBJ_H__ */