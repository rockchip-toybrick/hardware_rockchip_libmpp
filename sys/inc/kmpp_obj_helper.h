/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_HERLPER_H__
#define __KMPP_OBJ_HERLPER_H__

#if !defined(KMPP_OBJ_NAME) || \
    !defined(KMPP_OBJ_INTF_TYPE) || \
    !defined(KMPP_OBJ_IMPL_TYPE) || \
    !defined(KMPP_OBJ_ENTRY_TABLE)

#warning "When using kmpp_obj_helper.h The following macro must be defined:"
#warning "KMPP_OBJ_NAME                 - object name"
#warning "KMPP_OBJ_INTF_TYPE            - object interface type"
#warning "KMPP_OBJ_IMPL_TYPE            - object implement type"
#warning "option macro:"
#warning "KMPP_OBJ_ENTRY_TABLE          - object element value / pointer entry table"
#warning "KMPP_OBJ_STRUCT_TABLE         - object element structure / array / share memory table"
#warning "KMPP_OBJ_ENTRY_HOOK           - object element value hook function table"
#warning "KMPP_OBJ_STRUCT_HOOK          - object element structure hook function table"
#warning "KMPP_OBJ_FUNC_EXPORT_ENABLE   - enable function EXPORT_SYMBOL"
#warning "KMPP_OBJ_FUNC_STUB_ENABLE     - enable function stub mode by define empty function"
#warning "KMPP_OBJ_SHARE_ENABLE         - use /dev/kmpp_objs to share object to userspace"
#warning "KMPP_OBJ_FUNC_INIT            - add object init function"
#warning "KMPP_OBJ_FUNC_DEINIT          - add object deinit function"
#warning "KMPP_OBJ_FUNC_DUMP            - add object dump function"

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME not defined"
#endif
#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE not defined"
#endif
#ifndef KMPP_OBJ_IMPL_TYPE
#error "KMPP_OBJ_IMPL_TYPE not defined"
#endif

#else /* all input macro defined */

#define KMPP_OBJ_TO_STR(x)      #x
#define KMPP_OBJ_DEF(x)         x##_def
#define KMPP_OBJ_DEF_NAME(x)    KMPP_OBJ_TO_STR(x)

#ifndef KMPP_OBJ_ENTRY_TABLE
#define KMPP_OBJ_ENTRY_TABLE(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_STRUCT_TABLE
#define KMPP_OBJ_STRUCT_TABLE(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_ENTRY_HOOK
#define KMPP_OBJ_ENTRY_HOOK(ENTRY, prefix)
#endif
#ifndef KMPP_OBJ_STRUCT_HOOK
#define KMPP_OBJ_STRUCT_HOOK(ENTRY, prefix)
#endif

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
        .flag_offset = offsetof(KMPP_OBJ_IMPL_TYPE, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD_TO_LOCTBL_ACCESS1(f1, ftype) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

#define ENTRY_TO_TRIE1(prefix, ftype, type, f1) \
    do { \
        KmppLocTbl tbl = FIELD_TO_LOCTBL_ACCESS1(f1, ftype); \
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), #f1, &tbl); \
    } while (0);

#define HOOK_TO_TRIE1(prefix, ftype, type, f1) \
    do { \
        /* NOTE: also add entry table for userspace access and dump */ \
        KmppLocTbl tbl = FIELD_TO_LOCTBL_ACCESS1(f1, ftype); \
        kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), #f1, &tbl); \
        kmpp_objdef_add_hook(KMPP_OBJ_DEF(prefix), "s_"#f1, KMPP_OBJ_FUNC3(prefix, impl_set, f1)); \
        kmpp_objdef_add_hook(KMPP_OBJ_DEF(prefix), "g_"#f1, KMPP_OBJ_FUNC3(prefix, impl_get, f1)); \
    } while (0);

#define ENTRY_QUERY(prefix, ftype, type, f1) \
    do { \
        kmpp_objdef_get_entry(KMPP_OBJ_DEF(prefix), #f1, &tbl_##prefix##_##f1); \
    } while (0);

#define HOOK_QUERY(prefix, ftype, type, f1) \
    do { \
        KMPP_OBJ_HOOK3(prefix, set, f1) = \
        kmpp_objdef_get_hook(KMPP_OBJ_DEF(prefix), "s_"#f1); \
        KMPP_OBJ_HOOK3(prefix, get, f1) = \
        kmpp_objdef_get_hook(KMPP_OBJ_DEF(prefix), "g_"#f1); \
    } while (0);

#define FIELD_TO_LOCTBL_FLAG2(f1, f2, ftype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1.f2), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1.f2), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_offset = offsetof(KMPP_OBJ_IMPL_TYPE, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD_TO_LOCTBL_ACCESS2(f1, f2, ftype) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1.f2), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1.f2), \
        .data_type = ENTRY_TYPE_##ftype, \
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
#define KMPP_OBJ_HOOK3(x, y, z) hook_##x##_##y##_##z##_idx

#if defined(KMPP_OBJ_SHARE_ENABLE)
#define KMPP_OBJ_SHARE_FUNC(x)  kmpp_objdef_share(x)
#else
#define KMPP_OBJ_SHARE_FUNC(x)
#endif

#if defined(KMPP_OBJ_FUNC_INIT)
#define KMPP_OBJ_ADD_INIT(x)    kmpp_objdef_add_init(KMPP_OBJ_DEF(x), KMPP_OBJ_FUNC_INIT)
#else
#define KMPP_OBJ_ADD_INIT(x)
#endif

#if defined(KMPP_OBJ_FUNC_DEINIT)
#define KMPP_OBJ_ADD_DEINIT(x)  kmpp_objdef_add_deinit(KMPP_OBJ_DEF(x), KMPP_OBJ_FUNC_DEINIT)
#else
#define KMPP_OBJ_ADD_DEINIT(x)
#endif

#if defined(KMPP_OBJ_FUNC_DUMP)
#define KMPP_OBJ_ADD_DUMP(x)    kmpp_objdef_add_dump(KMPP_OBJ_DEF(x), KMPP_OBJ_FUNC_DUMP)
#else
#define KMPP_OBJ_ADD_DUMP(x)
#endif

#if !defined(KMPP_OBJ_FUNC_STUB_ENABLE)
#include "kmpp_string.h"

#define KMPP_OBJ_ENTRY_FUNC(prefix, ftype, type, field) \
    static KmppLocTbl *tbl_##prefix##_##field = NULL; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            *v = ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field; \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            ((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field = v; \
        return ret; \
    }

#define KMPP_OBJ_STRUCT_FUNC(prefix, ftype, type, field) \
    static KmppLocTbl *tbl_##prefix##_##field = NULL; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_get_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            osal_memcpy(v, &((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field, \
                        sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->field)); \
        return ret; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (tbl_##prefix##_##field) \
            ret = kmpp_obj_tbl_set_##ftype(s, tbl_##prefix##_##field, v); \
        else \
            osal_memcpy(&((KMPP_OBJ_IMPL_TYPE*)kmpp_obj_to_entry(s))->field, v, \
                        sizeof(((KMPP_OBJ_IMPL_TYPE*)0)->field)); \
        return ret; \
    }

#define KMPP_OBJ_ENTRY_HOOK_FUNC(prefix, ftype, type, field) \
    static rk_s32 KMPP_OBJ_HOOK3(prefix, get, field) = -1; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (KMPP_OBJ_HOOK3(prefix, get, field) >= 0) \
            ret = kmpp_obj_idx_run(s, KMPP_OBJ_HOOK3(prefix, get, field), (void *)v, __FUNCTION__); \
        return ret; \
    } \
    static rk_s32 KMPP_OBJ_HOOK3(prefix, set, field) = -1; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (KMPP_OBJ_HOOK3(prefix, set, field) >= 0) \
            ret = kmpp_obj_idx_run(s, KMPP_OBJ_HOOK3(prefix, set, field), (void *)&v, __FUNCTION__); \
        return ret; \
    }

#define KMPP_OBJ_STRUCT_HOOK_FUNC(prefix, ftype, type, field) \
    static rk_s32 KMPP_OBJ_HOOK3(prefix, get, field) = -1; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (KMPP_OBJ_HOOK3(prefix, get, field) >= 0) \
            ret = kmpp_obj_idx_run(s, KMPP_OBJ_HOOK3(prefix, get, field), (void *)v, __FUNCTION__); \
        return ret; \
    } \
    static rk_s32 KMPP_OBJ_HOOK3(prefix, set, field) = -1; \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        rk_s32 ret = kmpp_obj_check(s, __FUNCTION__); \
        if (ret) return ret; \
        if (KMPP_OBJ_HOOK3(prefix, set, field) >= 0) \
            ret = kmpp_obj_idx_run(s, KMPP_OBJ_HOOK3(prefix, set, field), (void *)v, __FUNCTION__); \
        return ret; \
    }

#define KMPP_OBJS_USAGE_SET(prefix) \
static KmppObjDef KMPP_OBJ_DEF(prefix) = NULL; \
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_ENTRY_FUNC, prefix) \
KMPP_OBJ_STRUCT_TABLE(KMPP_OBJ_STRUCT_FUNC, prefix) \
KMPP_OBJ_ENTRY_HOOK(KMPP_OBJ_ENTRY_HOOK_FUNC, prefix) \
KMPP_OBJ_STRUCT_HOOK(KMPP_OBJ_STRUCT_HOOK_FUNC, prefix) \
rk_s32 KMPP_OBJ_FUNC2(prefix, init)(void) \
{ \
    rk_s32 impl_size = sizeof(KMPP_OBJ_IMPL_TYPE); \
    kmpp_objdef_get(&KMPP_OBJ_DEF(prefix), impl_size, KMPP_OBJ_DEF_NAME(KMPP_OBJ_INTF_TYPE)); \
    if (!KMPP_OBJ_DEF(prefix)) { \
        kmpp_loge_f(#prefix " init failed\n"); \
        return rk_nok; \
    } \
    KMPP_OBJ_ENTRY_TABLE(ENTRY_TO_TRIE1, prefix) \
    KMPP_OBJ_STRUCT_TABLE(ENTRY_TO_TRIE1, prefix) \
    KMPP_OBJ_ENTRY_HOOK(HOOK_TO_TRIE1, prefix) \
    KMPP_OBJ_STRUCT_HOOK(HOOK_TO_TRIE1, prefix) \
    kmpp_objdef_add_entry(KMPP_OBJ_DEF(prefix), NULL, NULL); \
    kmpp_objdef_add_hook(KMPP_OBJ_DEF(prefix), NULL, NULL); \
    KMPP_OBJ_ADD_INIT(prefix); \
    KMPP_OBJ_ADD_DEINIT(prefix); \
    KMPP_OBJ_ADD_DUMP(prefix); \
    KMPP_OBJ_ENTRY_TABLE(ENTRY_QUERY, prefix) \
    KMPP_OBJ_STRUCT_TABLE(ENTRY_QUERY, prefix) \
    KMPP_OBJ_ENTRY_HOOK(HOOK_QUERY, prefix) \
    KMPP_OBJ_STRUCT_HOOK(HOOK_QUERY, prefix) \
    KMPP_OBJ_SHARE_FUNC(KMPP_OBJ_DEF(prefix)); \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, deinit)(void) \
{ \
    return kmpp_objdef_put(KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, size)(void) \
{ \
    return kmpp_objdef_get_buf_size(KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get)(KMPP_OBJ_INTF_TYPE *obj) \
{ \
    return kmpp_obj_get_f((KmppObj *)obj, KMPP_OBJ_DEF(prefix)); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get_share)(KMPP_OBJ_INTF_TYPE *obj, KmppShm shm) \
{ \
    return kmpp_obj_get_share_f((KmppObj *)obj, KMPP_OBJ_DEF(prefix), shm); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, assign)(KMPP_OBJ_INTF_TYPE *obj, void *buf, rk_s32 size) \
{ \
    return kmpp_obj_assign_f((KmppObj *)obj, KMPP_OBJ_DEF(prefix), buf, size); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, put)(KMPP_OBJ_INTF_TYPE obj) \
{ \
    return kmpp_obj_put_f(obj); \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, dump)(KMPP_OBJ_INTF_TYPE obj, const rk_u8 *caller) \
{ \
    return kmpp_obj_dump(obj, caller); \
}

#else
#define KMPP_OBJ_ENTRY_FUNC(prefix, ftype, type, field) \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    }

#define KMPP_OBJ_STRUCT_FUNC(prefix, ftype, type, field) \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    }

#define KMPP_OBJ_ENTRY_HOOK_FUNC(prefix, ftype, type, field) \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    }

#define KMPP_OBJ_STRUCT_HOOK_FUNC(prefix, ftype, type, field) \
    rk_s32 KMPP_OBJ_FUNC3(prefix, get, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    } \
    rk_s32 KMPP_OBJ_FUNC3(prefix, set, field)(KMPP_OBJ_INTF_TYPE s, type *v) \
    { \
        (void) s; \
        (void) v; \
        return rk_ok; \
    }

#define KMPP_OBJS_USAGE_SET(prefix) \
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_ENTRY_FUNC, prefix) \
KMPP_OBJ_STRUCT_TABLE(KMPP_OBJ_STRUCT_FUNC, prefix) \
KMPP_OBJ_ENTRY_HOOK(KMPP_OBJ_ENTRY_HOOK_FUNC, prefix) \
KMPP_OBJ_STRUCT_HOOK(KMPP_OBJ_STRUCT_HOOK_FUNC, prefix) \
rk_s32 KMPP_OBJ_FUNC2(prefix, init)(void) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, deinit)(void) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, size)(void) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get)(KMPP_OBJ_INTF_TYPE *obj) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, get_share)(KMPP_OBJ_INTF_TYPE *obj, KmppShmGrp grp) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, assign)(KMPP_OBJ_INTF_TYPE *obj, void *buf, rk_s32 size) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, put)(KMPP_OBJ_INTF_TYPE obj) \
{ \
    return rk_ok; \
} \
rk_s32 KMPP_OBJ_FUNC2(prefix, dump)(KMPP_OBJ_INTF_TYPE obj, const rk_u8 *caller) \
{ \
    return rk_ok; \
}
#endif


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
KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_EXPORT, prefix) \
KMPP_OBJ_STRUCT_TABLE(KMPP_OBJ_EXPORT, prefix) \
KMPP_OBJ_ENTRY_HOOK(KMPP_OBJ_EXPORT, prefix) \
KMPP_OBJ_STRUCT_HOOK(KMPP_OBJ_EXPORT, prefix)

#ifdef KMPP_OBJ_FUNC_EXPORT_ENABLE
KMPP_OBJS_USAGE_EXPORT(KMPP_OBJ_NAME)
#endif

#undef KMPP_OBJ_TO_STR
#undef KMPP_OBJ_DEF

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_IMPL_TYPE
#undef KMPP_OBJ_ENTRY_TABLE
#undef KMPP_OBJ_STRUCT_TABLE
#undef KMPP_OBJ_ENTRY_HOOK
#undef KMPP_OBJ_STRUCT_HOOK
#undef KMPP_OBJ_FUNC_EXPORT_ENABLE
#undef KMPP_OBJ_FUNC_INIT
#undef KMPP_OBJ_FUNC_DEINIT
#undef KMPP_OBJ_FUNC_DUMP
#undef KMPP_OBJ_SHARE_ENABLE

/* undef tmp macro */
#undef FIELD_TO_LOCTBL_FLAG1
#undef FIELD_TO_LOCTBL_ACCESS1
#undef ENTRY_TO_TRIE1
#undef FIELD_TO_LOCTBL_FLAG2
#undef FIELD_TO_LOCTBL_ACCESS2
#undef ENTRY_TO_TRIE2

#undef KMPP_OBJ_FUNC2
#undef KMPP_OBJ_FUNC3
#undef KMPP_OBJ_ENTRY_FUNC
#undef KMPP_OBJ_EXPORT
#undef KMPP_OBJ_SHARE_FUNC
#undef KMPP_OBJ_ADD_INIT
#undef KMPP_OBJ_ADD_DEINIT
#undef KMPP_OBJ_ADD_DUMP

#undef __KMPP_OBJ_HERLPER_H__

#endif

#endif /* __KMPP_OBJ_HERLPER_H__ */
