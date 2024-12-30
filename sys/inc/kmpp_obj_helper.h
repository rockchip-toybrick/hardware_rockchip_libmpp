/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_HERLPER_H__
#define __KMPP_OBJ_HERLPER_H__

#include "kmpp_shm.h"

#if !defined(KMPP_OBJ_NAME) || \
    !defined(KMPP_OBJ_INTF_TYPE) || \
    !defined(KMPP_OBJ_IMPL_TYPE) || \
    !defined(KMPP_OBJ_ENTRY_TABLE) || \
    (!defined(KMPP_OBJ_SHM_DISABLE) && \
     !defined(KMPP_OBJ_SHM_STAND_ALONE) && \
     !defined(KMPP_OBJ_SHM_IN_OBJS))

#warning "When using kmpp_obj_helper.h The following macro must be defined:"
#warning "KMPP_OBJ_NAME                  - object name"
#warning "KMPP_OBJ_INTF_TYPE             - object interface type"
#warning "KMPP_OBJ_IMPL_TYPE             - object implement type"
#warning "KMPP_OBJ_ENTRY_TABLE           - object element entry table"
#warning "one of three types of object share memory config must be defined:"
#warning "KMPP_OBJ_SHM_DISABLE           - do not share object to userspace"
#warning "KMPP_OBJ_SHM_STAND_ALONE       - use stand-alone path to share object to userspace"
#warning "KMPP_OBJ_SHM_IN_OBJS           - use /dev/kmpp_objs to share object to userspace"
#warning "option macro:"
#warning "KMPP_OBJ_FUNC_EXPORT_ENABLE    - enable function EXPORT_SYMBOL"
#warning "KMPP_OBJ_FUNC_PRESET           - add object preset function"
#warning "KMPP_OBJ_FUNC_DUMP             - add object dump function"

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME not defined"
#endif
#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE not defined"
#endif
#ifndef KMPP_OBJ_IMPL_TYPE
#error "KMPP_OBJ_IMPL_TYPE not defined"
#endif
#ifndef KMPP_OBJ_ENTRY_TABLE
#error "KMPP_OBJ_ENTRY_TABLE not defined"
#endif
#if !defined(KMPP_OBJ_SHM_DISABLE) && !defined(KMPP_OBJ_SHM_STAND_ALONE) && !defined(KMPP_OBJ_SHM_IN_OBJS)
#error "KMPP_OBJ_SHM_DISABLE, KMPP_OBJ_SHM_STAND_ALONE or KMPP_OBJ_SHM_IN_OBJS must be defined one of them"
#endif

#else /* all input macro defined */

#define KMPP_OBJ_TO_STR(x)      #x
#define KMPP_OBJ_DEF(x)         x##_def

/*
 * macro for register structure fiedl to trie
 * type     -> struct base type
 * f1       -> struct:field1        name segment 1 the field1 part
 * f2       -> struct:field1:field2 name segment 2 the field2 part
 * ftype    -> field type as EntryType
 */
#define FIELD_TO_LOCTBL_FLAG1(f1, ftype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_type = flag_value ? 1 : 0, \
        .flag_offset = offsetof(KMPP_OBJ_IMPL_TYPE, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD_TO_LOCTBL_ACCESS1(f1, ftype) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_type = 0, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

#define ENTRY_TO_TRIE1(prefix, ftype, type, f1) \
    do { \
        KmppLocTbl tbl = FIELD_TO_LOCTBL_ACCESS1(f1, ftype); \
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), #f1, &tbl); \
    } while (0);

#define ENTRY_QUERY(prefix, ftype, type, f1) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_DEF(prefix), #f1, &tbl_##f1); \
    } while (0);

#define FIELD_TO_LOCTBL_FLAG2(f1, f2, ftype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1.f2), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1.f2), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_type = flag_value ? 1 : 0, \
        .flag_offset = offsetof(KMPP_OBJ_IMPL_TYPE, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD_TO_LOCTBL_ACCESS2(f1, f2, ftype) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1.f2), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1.f2), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_type = 0, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

#define ENTRY_TO_TRIE2(prefix, ftype, type, f1, f2) \
    do { \
        KmppLocTbl tbl = FIELD_TO_LOCTBL_ACCESS2(KMPP_OBJ_IMPL_TYPE, f1, f2, ftype); \
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), #f1":"#f2, &tbl); \
    } while (0);

#define KMPP_OBJ_FUNC2(x, y)    x##_##y
#define KMPP_OBJ_FUNC3(x, y, z) x##_##y##_##z

#define KMPP_OBJ_ACCESSORS(prefix, ftype, type, field) \
    static KmppLocTbl *tbl_##field = NULL; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##field, v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_get_entry(s))->field; \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##field) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##field, v); \
        else \
            ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_get_entry(s))->field = v; \
        return ret; \
    }

#if defined(KMPP_OBJ_SHM_DISABLE)
#define KMPP_OBJ_DEF_NAME(x, y) KMPP_OBJ_TO_STR(x)
#define KMPP_OBJ_SHM_FUNC(x)    rk_ok
#elif defined(KMPP_OBJ_SHM_STAND_ALONE)
#define KMPP_OBJ_DEF_NAME(x, y) KMPP_OBJ_TO_STR(x)
#define KMPP_OBJ_SHM_FUNC(x)    kmpp_objdef_add_shm_mgr(x)
#elif defined(KMPP_OBJ_SHM_IN_OBJS)
#define KMPP_OBJ_DEF_NAME(x, y) KMPP_OBJ_TO_STR(y)
#define KMPP_OBJ_SHM_FUNC(x)    kmpp_objdef_bind_shm_mgr(x)
#else
#define KMPP_OBJ_DEF_NAME(x, y) KMPP_OBJ_TO_STR(x)
#define KMPP_OBJ_SHM_FUNC(x)    rk_ok
#endif

#if defined(KMPP_OBJ_FUNC_PRESET)
#define KMPP_OBJ_ADD_PRESET(x)  kmpp_objdef_add_preset(KMPP_OBJ_DEF(x), KMPP_OBJ_FUNC_PRESET)
#else
#define KMPP_OBJ_ADD_PRESET(x)
#endif

#if defined(KMPP_OBJ_FUNC_DUMP)
#define KMPP_OBJ_ADD_DUMP(x)    kmpp_objdef_add_dump(KMPP_OBJ_DEF(x), KMPP_OBJ_FUNC_DUMP)
#else
#define KMPP_OBJ_ADD_DUMP(x)
#endif

#define KMPP_OBJS_USAGE_SET(prefix) \
static KmppObjDef KMPP_OBJ_DEF(prefix) = NULL; \
static void KMPP_OBJ_FUNC3(prefix, impl, preset)(void *obj); \
static rk_s32 KMPP_OBJ_FUNC3(prefix, impl, dump)(void *entry); \
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_ACCESSORS, prefix) \
rk_s32 KMPP_OBJ_FUNC2(prefix, init)(void) \
{ \
    rk_s32 impl_size = sizeof(KMPP_OBJ_IMPL_TYPE); \
    kmpp_objdef_init(&KMPP_OBJ_DEF(prefix), impl_size, KMPP_OBJ_DEF_NAME(KMPP_OBJ_NAME, KMPP_OBJ_INTF_TYPE)); \
    if (!KMPP_OBJ_DEF(prefix)) { \
        kmpp_loge_f(#prefix " init failed\n"); \
        return rk_nok; \
    } \
    KMPP_OBJ_ENTRY_TABLE(ENTRY_TO_TRIE1, prefix) \
    kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), NULL, NULL); \
    KMPP_OBJ_ADD_PRESET(prefix); \
    KMPP_OBJ_ADD_DUMP(prefix); \
    KMPP_OBJ_ENTRY_TABLE(ENTRY_QUERY, prefix) \
    return KMPP_OBJ_SHM_FUNC(KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, deinit)(void) \
{ \
    return kmpp_objdef_deinit(KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, size)(void) \
{ \
    return kmpp_objdef_get_buf_size(KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get)(KMPP_OBJ_INTF_TYPE *obj) \
{ \
    return kmpp_obj_get((KmppObj *)obj, KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get_share)(KMPP_OBJ_INTF_TYPE *obj, KmppShmGrp grp) \
{ \
    return kmpp_obj_get_share((KmppObj *)obj, KMPP_OBJ_DEF(prefix), grp); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, assign)(KMPP_OBJ_INTF_TYPE *obj, void *buf, rk_s32 size) \
{ \
    return kmpp_obj_assign((KmppObj *)obj, KMPP_OBJ_DEF(prefix), buf, size); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, put)(KMPP_OBJ_INTF_TYPE obj) \
{ \
    return kmpp_obj_put(obj); \
} \
void KMPP_OBJ_FUNC2(prefix, dump)(KMPP_OBJ_INTF_TYPE obj, const rk_u8 *caller) \
{ \
    kmpp_obj_dump(obj, caller); \
}

KMPP_OBJS_USAGE_SET(KMPP_OBJ_NAME);

#define KMPP_OBJ_EXPORT(prefix, ftype, type, field) \
    EXPORT_SYMBOL(KMPP_OBJ_FUNC3(prefix, get, field)); \
    EXPORT_SYMBOL(KMPP_OBJ_FUNC3(prefix, set, field));

#define KMPP_OBJS_USAGE_EXPORT(prefix) \
EXPORT_SYMBOL(KMPP_OBJ_FUNC2(prefix, size)); \
EXPORT_SYMBOL(KMPP_OBJ_FUNC2(prefix, get)); \
EXPORT_SYMBOL(KMPP_OBJ_FUNC2(prefix, get_share)); \
EXPORT_SYMBOL(KMPP_OBJ_FUNC2(prefix, assign)); \
EXPORT_SYMBOL(KMPP_OBJ_FUNC2(prefix, put)); \
EXPORT_SYMBOL(KMPP_OBJ_FUNC2(prefix, dump)); \
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_EXPORT, prefix)

#ifdef KMPP_OBJ_FUNC_EXPORT_ENABLE
KMPP_OBJS_USAGE_EXPORT(KMPP_OBJ_NAME)
#endif

#undef KMPP_OBJ_TO_STR
#undef KMPP_OBJ_DEF

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_IMPL_TYPE
#undef KMPP_OBJ_ENTRY_TABLE
#undef KMPP_OBJ_FUNC_EXPORT_ENABLE
#undef KMPP_OBJ_FUNC_PRESET
#undef KMPP_OBJ_FUNC_DUMP
#undef KMPP_OBJ_SHM_DISABLE
#undef KMPP_OBJ_SHM_STAND_ALONE
#undef KMPP_OBJ_SHM_IN_OBJS

/* undef tmp macro */
#undef FIELD_TO_LOCTBL_FLAG1
#undef FIELD_TO_LOCTBL_ACCESS1
#undef ENTRY_TO_TRIE1
#undef FIELD_TO_LOCTBL_FLAG2
#undef FIELD_TO_LOCTBL_ACCESS2
#undef ENTRY_TO_TRIE2

#undef KMPP_OBJ_FUNC2
#undef KMPP_OBJ_FUNC3
#undef KMPP_OBJ_ACCESSORS
#undef KMPP_OBJ_EXPORT
#undef KMPP_OBJ_DEF_NAME
#undef KMPP_OBJ_SHM_FUNC
#undef KMPP_OBJ_FUNC_EXPORT
#undef KMPP_OBJ_ADD_PRESET
#undef KMPP_OBJ_ADD_DUMP

#undef __KMPP_OBJ_HERLPER_H__

#endif

#endif /* __KMPP_OBJ_HERLPER_H__ */