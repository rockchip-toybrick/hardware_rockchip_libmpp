/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include "kmpp_cls.h"

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
        EntryType       data_type       : 4;
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

typedef rk_s32 (*KmppObjInit)(void *entry, osal_fs_dev *file, const rk_u8 *caller);
typedef rk_s32 (*KmppObjDeinit)(void *entry, const rk_u8 *caller);
typedef rk_s32 (*KmppObjDump)(void *entry);
typedef rk_s32 (*KmppObjHook)(void *entry, void *arg, const rk_u8 *caller);

rk_s32 kmpp_objdef_get(KmppObjDef *def, rk_s32 size, const rk_u8 *name);
rk_s32 kmpp_objdef_put(KmppObjDef def);
rk_u32 kmpp_objdef_find(KmppObjDef *def, const rk_u8 *name);

/*
 * object define info register function
 * if KmppLocTbl tbl is valid, the entry offset / size will be added to the trie
 * if KmppLocTbl tbl is NULL, the entry is a array object and name is just for index
 */
rk_s32 kmpp_objdef_add_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl *tbl);
/*
 * object hook handler register function
 * the hook will be registered to a defferent trie in objdef for fast indexing.
 */
rk_s32 kmpp_objdef_add_hook(KmppObjDef def, const rk_u8 *name, KmppObjHook hook);
/* object init function register default object is all zero */
rk_s32 kmpp_objdef_add_init(KmppObjDef def, KmppObjInit init);
/* object deinit function register */
rk_s32 kmpp_objdef_add_deinit(KmppObjDef def, KmppObjDeinit deinit);
/* object dump function register */
rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump);

/* NOTE: get entry / index / offset by name can only used for non __xxxx elements */
rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl **tbl);
rk_s32 kmpp_objdef_get_index(KmppObjDef def, const rk_u8 *name);
rk_s32 kmpp_objdef_get_offset(KmppObjDef def, const rk_u8 *name);
rk_s32 kmpp_objdef_get_hook(KmppObjDef def, const rk_u8 *name);

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

#define kmpp_obj_get_f(obj, def)                kmpp_obj_get(obj, def, __FUNCTION__)
#define kmpp_obj_assign_f(obj, def, buf, size)  kmpp_obj_assign(obj, def, buf, size, __FUNCTION__)
#define kmpp_obj_get_share_f(obj, def, file)    kmpp_obj_get_share(obj, def, file, __FUNCTION__)
#define kmpp_obj_put_f(obj)                     kmpp_obj_put(obj, __FUNCTION__)

/* normal kernel object allocator both object head and body */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def, const rk_u8 *caller);
/* use external buffer for both object head and body */
rk_s32 kmpp_obj_assign(KmppObj *obj, KmppObjDef def, void *buf, rk_s32 size, const rk_u8 *caller);
/* import external shared buffer for object body */
rk_s32 kmpp_obj_get_share(KmppObj *obj, KmppObjDef def, osal_fs_dev *file, const rk_u8 *caller);
rk_s32 kmpp_obj_put(KmppObj obj, const rk_u8 *caller);
rk_s32 kmpp_obj_check(KmppObj obj, const rk_u8 *caller);

/* object implement element entry access */
void *kmpp_obj_to_entry(KmppObj obj);
/* object implement element shm access */
KmppShm kmpp_obj_to_shm(KmppObj obj);
rk_s32 kmpp_obj_to_shmptr(KmppObj obj, KmppShmPtr *sptr);
rk_s32 kmpp_obj_from_shmptr(KmppObj *obj, KmppShmPtr *sptr);

/* value access function */
rk_s32 kmpp_obj_set_s32(KmppObj obj, const rk_u8 *name, rk_s32 val);
rk_s32 kmpp_obj_get_s32(KmppObj obj, const rk_u8 *name, rk_s32 *val);
rk_s32 kmpp_obj_set_u32(KmppObj obj, const rk_u8 *name, rk_u32 val);
rk_s32 kmpp_obj_get_u32(KmppObj obj, const rk_u8 *name, rk_u32 *val);
rk_s32 kmpp_obj_set_s64(KmppObj obj, const rk_u8 *name, rk_s64 val);
rk_s32 kmpp_obj_get_s64(KmppObj obj, const rk_u8 *name, rk_s64 *val);
rk_s32 kmpp_obj_set_u64(KmppObj obj, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_obj_get_u64(KmppObj obj, const rk_u8 *name, rk_u64 *val);
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
rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppLocTbl *tbl, void *val);

/* kernel access only function */
rk_s32 kmpp_obj_set_kobj(KmppObj obj, const rk_u8 *name, KmppObj val);
rk_s32 kmpp_obj_get_kobj(KmppObj obj, const rk_u8 *name, KmppObj *val);
rk_s32 kmpp_obj_set_kptr(KmppObj obj, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_kptr(KmppObj obj, const rk_u8 *name, void **val);
rk_s32 kmpp_obj_set_kfp(KmppObj obj, const rk_u8 *name, void *val);
rk_s32 kmpp_obj_get_kfp(KmppObj obj, const rk_u8 *name, void **val);
rk_s32 kmpp_obj_tbl_set_kobj(KmppObj obj, KmppLocTbl *tbl, KmppObj val);
rk_s32 kmpp_obj_tbl_get_kobj(KmppObj obj, KmppLocTbl *tbl, KmppObj *val);
rk_s32 kmpp_obj_tbl_set_kptr(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_kptr(KmppObj obj, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_kfp(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_kfp(KmppObj obj, KmppLocTbl *tbl, void **val);

/* share access function */
rk_s32 kmpp_obj_set_shm(KmppObj obj, const rk_u8 *name, KmppShmPtr *val);
rk_s32 kmpp_obj_get_shm(KmppObj obj, const rk_u8 *name, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_set_shm(KmppObj obj, KmppLocTbl *tbl, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_get_shm(KmppObj obj, KmppLocTbl *tbl, KmppShmPtr *val);

/* run callback function */

/* run function pointer callback in object as element with name */
rk_s32 kmpp_obj_run(KmppObj obj, const rk_u8 *name, void *arg);
/* run function pointer callback in object as element with location table */
rk_s32 kmpp_obj_tbl_run(KmppObj obj, KmppLocTbl *tbl, void *arg);
/*
 * run function pointer callback in object define with function register index
 * which is kmpp_objdef_get_hook return value
 */
rk_s32 kmpp_obj_idx_run(KmppObj obj, rk_s32 idx, void *arg, const rk_u8 *caller);

rk_s32 kmpp_obj_dump(KmppObj obj, const rk_u8 *caller);
#define kmpp_obj_dump_f(obj) kmpp_obj_dump(obj, __FUNCTION__)

const rk_u8 *strof_entry_type(EntryType type);

#endif /* __KMPP_OBJ_H__ */