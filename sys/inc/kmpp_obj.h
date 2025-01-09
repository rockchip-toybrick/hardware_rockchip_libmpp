/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include "kmpp_sys_defs.h"
#include "kmpp_trie.h"

/*
 * kernel object definition and kernel object
 *
 * The kernel object definition is used to define the object structure.
 * It uses trie to record the object name and the location tables to
 * which keep each element offset and size info in the object structure.
 *
 * The kernel object is the actual object used by kernel and userspace.
 *
 * Normal kernel object can be shared between different kernel modules.
 * When KMPP_OBJ_SHARE_ENABLE is defined before kmpp_obj_helper.h the kernel object
 * can be shared between kernel and userspace.
 *
 * Different kernel object creation methods:
 * kmpp_obj_get
 * - create an object by malloc and used only in kernel
 * kmpp_obj_assign
 * - create an object by external buffer and used only in kernel
 * kmpp_obj_get_share
 * - create an object by share memory and used both in kernel and userspace
 */

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

typedef struct KmppObjDefSet_t {
    KmppTrie            trie;
    rk_s32              count;
    rk_s32              buf_size;
    KmppObjDef          defs[0];
} KmppObjDefSet;

typedef void (*KmppObjPreset)(void *obj);
typedef rk_s32 (*KmppObjDump)(void *entry);

rk_s32 kmpp_objdef_get(KmppObjDef *def, rk_s32 size, const rk_u8 *name);
rk_s32 kmpp_objdef_put(KmppObjDef def);
rk_u32 kmpp_objdef_find(KmppObjDef *def, const rk_u8 *name);

rk_s32 kmpp_objdef_add_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl *tbl);
rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl **tbl);
rk_s32 kmpp_objdef_add_preset(KmppObjDef def, KmppObjPreset preset);
rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump);

rk_s32 kmpp_objdef_dump(KmppObjDef def);
void kmpp_objdef_dump_all(void);

/* kmpp objcet internal element access function */
const rk_u8 *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_buf_size(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
KmppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* Allow objdef to be shared with userspace */
rk_s32 kmpp_objdef_share(KmppObjDef def);
/* create an objdef set for userspace sharing */
rk_s32 kmpp_objdef_get_shared(KmppObjDefSet **defs);
/* destroy an objdef set */
rk_s32 kmpp_objdef_put_shared(KmppObjDefSet *defs);

/* normal kernel object allocator both object head and body */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def);
/* use external buffer for both object head and body */
rk_s32 kmpp_obj_assign(KmppObj *obj, KmppObjDef def, void *buf, rk_s32 size);
/* import external shared buffer for object body */
rk_s32 kmpp_obj_get_share(KmppObj *obj, KmppObjDef def, KmppShm shm);
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

const rk_u8 *strof_entry_type(EntryType type);

#endif /* __KMPP_OBJ_H__ */