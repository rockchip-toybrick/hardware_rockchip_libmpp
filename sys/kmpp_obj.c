/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_obj"

#include <linux/kernel.h>
#include "rk-mpp-kobj.h"

#include "rk_list.h"
#include "kmpp_atomic.h"
#include "kmpp_bitops.h"
#include "kmpp_macro.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_string.h"
#include "kmpp_trie.h"

#include "kmpp_obj_impl.h"
#include "kmpp_mem_pool.h"
#include "kmpp_shm.h"
#include "kmpp_ioctl_impl.h"

#define OBJ_DBG_FLOW                    (0x00000001)
#define OBJ_DBG_SHARE                   (0x00000002)
#define OBJ_DBG_ENTRY                   (0x00000004)
#define OBJ_DBG_HOOK                    (0x00000008)
#define OBJ_DBG_IOCTL                   (0x00000010)
#define OBJ_DBG_SET                     (0x00000040)
#define OBJ_DBG_GET                     (0x00000080)

#define obj_dbg(flag, fmt, ...)         kmpp_dbg(kmpp_obj_debug, flag, fmt, ## __VA_ARGS__)

#define obj_dbg_flow(fmt, ...)          obj_dbg(OBJ_DBG_FLOW, fmt, ## __VA_ARGS__)
#define obj_dbg_share(fmt, ...)         obj_dbg(OBJ_DBG_SHARE, fmt, ## __VA_ARGS__)
#define obj_dbg_entry(fmt, ...)         obj_dbg(OBJ_DBG_ENTRY, fmt, ## __VA_ARGS__)
#define obj_dbg_hook(fmt, ...)          obj_dbg(OBJ_DBG_HOOK, fmt, ## __VA_ARGS__)
#define obj_dbg_ioctl(fmt, ...)         obj_dbg(OBJ_DBG_IOCTL, fmt, ## __VA_ARGS__)
#define obj_dbg_set(fmt, ...)           obj_dbg(OBJ_DBG_SET, fmt, ## __VA_ARGS__)
#define obj_dbg_get(fmt, ...)           obj_dbg(OBJ_DBG_GET, fmt, ## __VA_ARGS__)

#define ENTRY_TO_PTR(tbl, entry)        ((rk_u8 *)entry + tbl->tbl.elem_offset)
#define ENTRY_TO_s32_PTR(tbl, entry)    ((rk_s32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u32_PTR(tbl, entry)    ((rk_u32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_s64_PTR(tbl, entry)    ((rk_s64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u64_PTR(tbl, entry)    ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_st_PTR(tbl, entry)     ((void *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_shm_PTR(tbl, entry)    ((void *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_kobj_PTR(tbl, entry)   ((KmppObj *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_kptr_PTR(tbl, entry)   ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_kfp_PTR(tbl, entry)    ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_uobj_PTR(tbl, entry)   ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_uptr_PTR(tbl, entry)   ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_ufp_PTR(tbl, entry)    ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))

/* 32bit unsigned long pointer */
#define ELEM_FLAG_U32_POS(offset)       (((offset) & (~31)) / 8)
#define ELEM_FLAG_BIT_POS(offset)       ((offset) & 31)
#define ENTRY_TO_FLAG_PTR(e, entry)     ((rk_ul *)((rk_u8 *)entry + ELEM_FLAG_U32_POS(e->tbl.flag_offset)))

#define ENTRY_SET_FLAG(e, entry) \
    osal_set_bit(ELEM_FLAG_BIT_POS(e->tbl.flag_offset), ENTRY_TO_FLAG_PTR(e, entry))

#define ENTRY_CLR_FLAG(e, entry) \
    osal_clear_bit(ELEM_FLAG_BIT_POS(e->tbl.flag_offset), ENTRY_TO_FLAG_PTR(e, entry))

#define ENTRY_TEST_FLAG(e, entry) \
    osal_test_bit(ELEM_FLAG_BIT_POS(e->tbl.flag_offset), ENTRY_TO_FLAG_PTR(e, entry))

#define get_obj_srv(caller) \
    ({ \
        KmppObjSrv *__tmp; \
        if (srv_obj) { \
            __tmp = srv_obj; \
        } else { \
            kmpp_err_f("kmpp obj srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

#define get_obj_srv_f() get_obj_srv(__FUNCTION__)

typedef struct KmppObjDefInfo_t {
    KmppObjDef def;
    const rk_u8 *name;
    void *root;
    rk_s32 buf_size;
    rk_s32 offset;
} KmppObjDefInfo;

typedef struct KmppObjDefImpl_t {
    osal_list_head list;
    rk_s32 ref_cnt;
    rk_s32 index;
    rk_s32 entry_size;
    rk_s32 flag_max_pos;
    rk_s32 flag_offset;
    rk_s32 buf_size;
    KmppMemPool pool;
    KmppTrie trie;
    KmppTrie hook;
    rk_s32 shared;
    rk_s32 hook_cnt;
    KmppObjHook *hooks;
    KmppObjInit init;
    KmppObjDeinit deinit;
    KmppObjIoctls *ioctls;
    KmppObjDump dump;
    const rk_u8 *name_check;            /* for object name address check */
    rk_u8 *name;
} KmppObjDefImpl;

typedef struct KmppObjImpl_t {
    const rk_u8 *name_check;
    /* class infomation link */
    KmppObjDefImpl *def;
    /* trie for fast access */
    KmppTrie trie;
    /* record used memory pool */
    void *pool;
    /* implment body here */
    KmppShm shm;
    void *entry;
} KmppObjImpl;

typedef struct KmppObjSrv_t {
    osal_list_head  list;
    KmppMemPool     head_pool;
    rk_s32          count;
    rk_s32          index;
} KmppObjSrv;

static rk_u32 kmpp_obj_debug = 0;
static KmppObjSrv *srv_obj = NULL;
/* NOTE: objdef get / put MUST on insmod / rmmod in spinlock */

const rk_u8 *strof_ELEM_TYPE(ElemType type)
{
    static const rk_u8 *ELEM_TYPE_names[] = {
        [ELEM_TYPE_s32]    = "s32",
        [ELEM_TYPE_u32]    = "u32",
        [ELEM_TYPE_s64]    = "s64",
        [ELEM_TYPE_u64]    = "u64",
        [ELEM_TYPE_st]     = "struct",
        [ELEM_TYPE_shm]    = "shm_ptr",
        [ELEM_TYPE_kobj]   = "kobj",
        [ELEM_TYPE_kptr]   = "kptr",
        [ELEM_TYPE_kfp]    = "kfunc_ptr",
        [ELEM_TYPE_uobj]   = "uobj",
        [ELEM_TYPE_uptr]   = "uptr",
        [ELEM_TYPE_ufp]    = "ufunc_ptr",
    };
    static const rk_u8 *invalid_type_str = "invalid";

    if (type & (~ELEM_TYPE_BUTT))
        return invalid_type_str;

    return ELEM_TYPE_names[type] ? ELEM_TYPE_names[type] : invalid_type_str;
}

#define KMPP_OBJ_ACCESS_IMPL(type, base_type, log_str) \
    rk_s32 kmpp_obj_impl_set_##type(KmppEntry *tbl, void *entry, base_type val) \
    { \
        base_type *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        base_type old = dst[0]; \
        dst[0] = val; \
        if (!tbl->tbl.flag_offset) { \
            obj_dbg_set("%px + %x set " #type " change " #log_str " -> " #log_str "\n", entry, tbl->tbl.elem_offset, old, val); \
        } else { \
            if (old != val) { \
                obj_dbg_set("%px + %x set " #type " update " #log_str " -> " #log_str " flag %#x\n", \
                            entry, tbl->tbl.elem_offset, old, val, tbl->tbl.flag_offset); \
                ENTRY_SET_FLAG(tbl, entry); \
            } else { \
                obj_dbg_set("%px + %x set " #type " keep   " #log_str "\n", entry, tbl->tbl.elem_offset, old); \
            } \
        } \
        return rk_ok; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppEntry *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->tbl.elem_size) { \
            base_type *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            obj_dbg_get("%px + %x get " #type " value  " #log_str "\n", entry, tbl->tbl.elem_offset, src[0]); \
            val[0] = src[0]; \
            return rk_ok; \
        } \
        return rk_nok; \
    }

KMPP_OBJ_ACCESS_IMPL(s32, rk_s32, %d)
KMPP_OBJ_ACCESS_IMPL(u32, rk_u32, %u)
KMPP_OBJ_ACCESS_IMPL(s64, rk_s64, %llx)
KMPP_OBJ_ACCESS_IMPL(u64, rk_u64, %llx)
KMPP_OBJ_ACCESS_IMPL(kobj, KmppObj, %px)
KMPP_OBJ_ACCESS_IMPL(kptr, void *, %px)
KMPP_OBJ_ACCESS_IMPL(kfp, void *, %px)
KMPP_OBJ_ACCESS_IMPL(uobj, rk_u64, %llx)
KMPP_OBJ_ACCESS_IMPL(uptr, rk_u64, %llx)
KMPP_OBJ_ACCESS_IMPL(ufp, rk_u64, %llx)

#define KMPP_OBJ_ACCESS_IMPL_ST(type, base_type) \
    rk_s32 kmpp_obj_impl_set_##type(KmppEntry *tbl, void *entry, base_type *val) \
    { \
        rk_u8 *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        if (!tbl->tbl.flag_offset) { \
            /* simple copy */ \
            obj_dbg_set("%px + %x set " #type " change %px -> %px\n", \
                        entry, tbl->tbl.elem_offset, dst, val); \
            osal_memcpy(dst, val, tbl->tbl.elem_size); \
            return rk_ok; \
        } else { \
            /* copy with flag check and updata */ \
            if (osal_memcmp(dst, val, tbl->tbl.elem_size)) { \
                obj_dbg_set("%px + %x set " #type " update %px -> %px flag %#x\n", \
                            entry, tbl->tbl.elem_offset, dst, val, tbl->tbl.flag_offset); \
                osal_memcpy(dst, val, tbl->tbl.elem_size); \
                ENTRY_SET_FLAG(tbl, entry); \
            } else { \
                obj_dbg_set("%px + %x set " #type " keep   %px\n", \
                            entry, tbl->tbl.elem_offset, dst); \
            } \
        } \
        return rk_ok; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppEntry *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->tbl.elem_size) { \
            void *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            obj_dbg_get("%px + %d get " #type " from %px\n", entry, tbl->tbl.elem_offset, src); \
            osal_memcpy(val, src, tbl->tbl.elem_size); \
            return rk_ok; \
        } \
        return rk_nok; \
    }

KMPP_OBJ_ACCESS_IMPL_ST(st, void)
KMPP_OBJ_ACCESS_IMPL_ST(shm, KmppShmPtr)

void kmpp_obj_init(void)
{
    KmppObjSrv *srv = srv_obj;

    if (srv)
        return;

    srv = (KmppObjSrv *)kmpp_calloc_atomic(sizeof(KmppObjSrv));
    if (!srv) {
        kmpp_loge_f("kmpp obj srv malloc failed\n");
        return;
    }

    OSAL_INIT_LIST_HEAD(&srv->list);
    srv->head_pool = kmpp_mem_get_pool_f("kmpp_obj_head", sizeof(KmppObjImpl), 0, 0);

    srv_obj = srv;
}

void kmpp_obj_deinit(void)
{
    KmppObjSrv *srv = srv_obj;

    if (!srv)
        return;

    kmpp_mem_put_pool_f(srv->head_pool);
    kmpp_free(srv);
    srv_obj = NULL;
}

rk_s32 kmpp_objdef_get(KmppObjDef *def, rk_s32 size, const rk_u8 *name)
{
    KmppObjSrv *srv = get_obj_srv_f();
    KmppObjDefImpl *impl = NULL;
    rk_s32 len;

    if (!srv || !def || !size || !name) {
        kmpp_loge_f("invalid srv %px param def %p size %d name %p\n",
                    srv, def, size, name);
        return rk_nok;
    }

    *def = NULL;
    len = KMPP_ALIGN(osal_strlen(name) + 1, sizeof(rk_s32));
    impl = (KmppObjDefImpl *)kmpp_calloc(sizeof(KmppObjDefImpl) + len);
    if (!impl) {
        kmpp_loge_f("malloc contex %d failed\n", sizeof(KmppObjDefImpl) + len);
        return rk_nok;
    }

    impl->name_check = name;
    impl->name = (rk_u8 *)(impl + 1);
    impl->entry_size = size;
    impl->buf_size = size + sizeof(KmppObjImpl);

    osal_strncpy(impl->name, name, len);

    OSAL_INIT_LIST_HEAD(&impl->list);
    osal_list_add_tail(&impl->list, &srv->list);
    impl->index = srv->index++;
    srv->count++;
    impl->ref_cnt++;

    obj_dbg_flow("kmpp_objdef_get %-16s - %px %d\n", name, impl, impl->ref_cnt);

    *def = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_objdef_get);

rk_s32 kmpp_objdef_put(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl) {
        rk_s32 old = KMPP_FETCH_SUB(&impl->ref_cnt, 1);

        obj_dbg_flow("kmpp_objdef_put %-16s - %px %d -> %d\n",
                     impl->name, impl, old, impl->ref_cnt);

        if (!impl->ref_cnt) {
            KmppObjSrv *srv = get_obj_srv_f();

            if (impl->trie)
                ret = kmpp_trie_deinit(impl->trie);

            if (impl->hook)
                ret = kmpp_trie_deinit(impl->hook);

            osal_list_del_init(&impl->list);
            srv->count--;

            if (impl->pool) {
                kmpp_mem_put_pool_f(impl->pool);
                impl->pool = NULL;
            }
            kmpp_free(impl->hooks);
            kmpp_free(impl);
        }

        ret = rk_ok;
    }

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_put);

static KmppObjDefImpl *find_objdef(const rk_u8 *name)
{
    KmppObjSrv *srv = get_obj_srv_f();
    KmppObjDefImpl *impl = NULL;
    KmppObjDefImpl *n = NULL;

    osal_list_for_each_entry_safe(impl, n, &srv->list, KmppObjDefImpl, list) {
        if (!osal_strcmp(impl->name, name))
            return impl;
    }

    return NULL;
}

rk_s32 kmpp_objdef_find(KmppObjDef *def, const rk_u8 *name)
{
    KmppObjDefImpl *impl = find_objdef(name);

    if (impl) {
        rk_s32 old = KMPP_FETCH_ADD(&impl->ref_cnt, 1);
        obj_dbg_flow("kmpp_objdef_find %s - %px %d->%d\n",
                     impl->name, impl, old, impl->ref_cnt);
        *def = impl;
        return rk_ok;
    }

    *def = NULL;
    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_find);

static rk_s32 create_objdef_mem_pool(KmppObjDefImpl *impl)
{
    rk_s32 old_size = impl->buf_size;

    /* When last entry finish update and create memory pool */
    if (impl->flag_max_pos) {
        rk_s32 flag_size = KMPP_ALIGN(impl->flag_max_pos, 8) / 8;

        impl->flag_offset = impl->buf_size;
        flag_size -= impl->entry_size;
        flag_size = KMPP_ALIGN(flag_size, 4);
        impl->buf_size += flag_size;
    }

    obj_dbg_entry("objdef %-16s entry size %4d buf size %4d -> %4d\n", impl->name,
                    impl->entry_size, old_size, impl->buf_size);

    impl->pool = kmpp_mem_get_pool_f(impl->name, impl->buf_size, 0, 0);
    if (!impl->pool)
        kmpp_loge_f("get mem pool size %d failed\n", impl->buf_size);

    return impl->pool ? rk_ok : rk_nok;
}

rk_s32 kmpp_objdef_add_entry(KmppObjDef def, const rk_u8 *name, KmppEntry *tbl)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (!impl->trie) {
        if (!name) {
            /* NOTE: no entry objdef still need to create mempool */
            return create_objdef_mem_pool(impl);
        }

        kmpp_trie_init(&impl->trie, impl->name);
    }

    if (impl->trie) {
        KmppTrie trie = impl->trie;

        if (name) {
            ret = kmpp_trie_add_info(trie, name, tbl, tbl ? sizeof(*tbl) : 0);

            if (tbl->tbl.flag_offset > impl->flag_max_pos)
                impl->flag_max_pos = tbl->tbl.flag_offset;

            obj_dbg_entry("objdef %-16s add entry %-16s flag offset %4d\n",
                          impl->name, name, tbl->tbl.flag_offset);
        } else {
            /* record object impl size */
            ret = kmpp_trie_add_info(trie, "__index", &impl->index, sizeof(rk_s32));
            ret = kmpp_trie_add_info(trie, "__size", &impl->entry_size, sizeof(rk_s32));
            ret |= kmpp_trie_add_info(trie, NULL, NULL, 0);
            ret |= create_objdef_mem_pool(impl);
        }
    }

    if (ret)
        kmpp_loge_f("class %s add entry %s failed ret %d\n",
                    impl ? impl->name : NULL, name, ret);

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_add_entry);

rk_s32 kmpp_objdef_add_hook(KmppObjDef def, const rk_u8 *name, KmppObjHook hook)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    obj_dbg_hook("name %s hook %px to trie %px\n", name, hook, impl->hook);

    if (!impl->hook) {
        if (!name)
            return rk_ok;

        kmpp_trie_init(&impl->hook, impl->name);
    }

    if (impl->hook) {
        KmppTrie trie = impl->hook;

        if (name) {
            ret = kmpp_trie_add_info(trie, name, &hook, sizeof(hook));
        } else {
            impl->hook_cnt = kmpp_trie_get_info_count(trie);
            ret = kmpp_trie_add_info(trie, NULL, NULL, 0);
            impl->hooks = kmpp_malloc(impl->hook_cnt * sizeof(KmppObjHook));
            if (impl->hooks) {
                KmppTrieInfo *info = kmpp_trie_get_info_first(trie);
                rk_s32 i = 0;

                while (info && i < impl->hook_cnt) {
                    impl->hooks[i] = *((KmppObjHook *)kmpp_trie_info_ctx(info));
                    obj_dbg_hook("add hook %2d - %px\n", i, impl->hooks[i]);
                    info = kmpp_trie_get_info_next(trie, info);
                    i++;
                }
            }
        }
    }

    if (ret)
        kmpp_loge_f("class %s add hook %s failed ret %d\n",
                    impl ? impl->name : NULL, name, ret);

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_add_hook);

rk_s32 kmpp_objdef_add_init(KmppObjDef def, KmppObjInit init)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

        impl->init = init;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_add_init);

rk_s32 kmpp_objdef_add_deinit(KmppObjDef def, KmppObjDeinit deinit)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

        impl->deinit = deinit;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_add_deinit);

rk_s32 kmpp_objdef_add_ioctl(KmppObjDef def, KmppObjIoctls *ioctls)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

        impl->ioctls = ioctls;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_add_ioctl);

rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

        impl->dump = dump;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_add_dump);

rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const rk_u8 *name, KmppEntry **tbl)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        if (info) {
            *tbl = (KmppEntry *)kmpp_trie_info_ctx(info);
            ret = rk_ok;
        }
    }

    if (ret)
        kmpp_loge_f("class %s get entry %s failed ret %d\n",
                    impl ? impl->name : NULL, name, ret);

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_get_entry);

rk_s32 kmpp_objdef_get_index(KmppObjDef def, const rk_u8 *name)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 index = -1;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        index = info ? info->index : -1;
    }

    return index;
}
EXPORT_SYMBOL(kmpp_objdef_get_index);

rk_s32 kmpp_objdef_get_offset(KmppObjDef def, const rk_u8 *name)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 offset = -1;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info);

            if (tbl)
                offset = tbl->tbl.elem_offset;
        }
    }

    return offset;
}
EXPORT_SYMBOL(kmpp_objdef_get_offset);

rk_s32 kmpp_objdef_get_hook(KmppObjDef def, const rk_u8 *name)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 index = -1;

    if (impl->hook && name) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->hook, name);

        index = info ? info->index : -1;
    }

    obj_dbg_hook("get hook %s from trie %px ret %d\n", name, impl->hook, index);

    return index;
}
EXPORT_SYMBOL(kmpp_objdef_get_hook);

rk_s32 kmpp_objdef_ioctl(KmppObjDef def, osal_fs_dev *file, KmppIoc ioc)
{
    KmppObjDefImpl *impl_def = (KmppObjDefImpl *)def;
    KmppIocImpl *impl_ioc = (KmppIocImpl *)ioc;
    KmppObjIoctls *ioctls = impl_def->ioctls;
    rk_s32 i;

    if (!ioctls) {
        kmpp_loge_f("objdef %s not support ioctl\n", impl_def->name);
        return rk_nok;
    }

    obj_dbg_ioctl("objdef %s ioctl cnt %d try proc cmd %d\n",
                  impl_def->name, ioctls->count, impl_ioc->cmd);

    for (i = 0; i < ioctls->count; i++) {
        KmppObjIoctl *func = &ioctls->funcs[i];

        if (func->cmd == impl_ioc->cmd)
            return func->func(file, &impl_ioc->in, &impl_ioc->out);
    }

    kmpp_loge_f("objdef %s not support ioctl cmd %d\n", impl_def->name, impl_ioc->cmd);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_ioctl);

rk_s32 kmpp_objdef_dump(KmppObjDef def, rk_u32 flag)
{
    KmppObjDefImpl *impl;
    KmppTrieInfo *info;
    const rk_u8 *name;
    rk_s32 i;

    if (!def || !flag)
        return rk_nok;

    impl = (KmppObjDefImpl *)def;
    info = NULL;
    name = impl->name;
    i = 0;

    kmpp_logi("--------------------- objdef %-16s ---------------------\n", name);

    if (flag & OBJDEF_DUMP_INFO)
        kmpp_logi("entry_size %d element count %d hook count %d\n",
                    impl->entry_size, kmpp_trie_get_info_count(impl->trie),
                    kmpp_trie_get_info_count(impl->hook));

    if (impl->trie) {
        info = kmpp_trie_get_info_first(impl->trie);
        while (info) {
            name = kmpp_trie_info_name(info);
            if (!kmpp_trie_info_name_is_self(name)) {
                rk_s32 idx = i++;

                if (flag & OBJDEF_DUMP_ENTRY) {
                    if (info->ctx_len) {
                        KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info);

                        kmpp_logi("elem %-2d - %-16s offset %4d size %d\n", idx,
                                    name, tbl->tbl.elem_offset, tbl->tbl.elem_size);
                    } else {
                        /* meta key */
                        if (idx != info->index)
                            kmpp_logi("elem %-2d - key - %c%c%c%c - index %d mismatch\n",
                                        idx, name[3], name[2], name[1], name[0], info->index);
                        else
                            kmpp_logi("elem %-2d - key - %c%c%c%c\n",
                                        idx, name[3], name[2], name[1], name[0]);
                    }
                }
            }
            info = kmpp_trie_get_info_next(impl->trie, info);
        }

        info = kmpp_trie_get_info_first(impl->trie);
        while (info) {
            name = kmpp_trie_info_name(info);
            if (kmpp_trie_info_name_is_self(name)) {
                void *p = kmpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                if (flag & OBJDEF_DUMP_SELF) {
                    if (info->ctx_len == sizeof(rk_u32)) {
                        kmpp_logi("trie %-2d - %-16s val %d\n", idx, name, *(rk_u32 *)p);
                    } else {
                        kmpp_logi("trie %-2d - %-16s str %s\n", idx, name, p);
                    }
                }
            }
            info = kmpp_trie_get_info_next(impl->trie, info);
        }
    }

    /* dump hook info */
    if (impl->hook) {
        i = 0;
        info = kmpp_trie_get_info_first(impl->hook);
        while (info) {
            name = kmpp_trie_info_name(info);
            if (!kmpp_trie_info_name_is_self(name)) {
                rk_s32 idx = i++;

                if (flag & OBJDEF_DUMP_HOOK) {
                    if (info->ctx_len) {
                        KmppObjHook *hook = (KmppObjHook *)kmpp_trie_info_ctx(info);

                        kmpp_logi("hook %-2d - %-16s hook %px\n", idx, name, *hook);
                    } else {
                        /* meta key */
                        if (idx != info->index)
                            kmpp_logi("hook %-2d - key - %c%c%c%c - index %d mismatch\n",
                                        idx, name[3], name[2], name[1], name[0], info->index);
                        else
                            kmpp_logi("hook %-2d - key - %c%c%c%c\n",
                                        idx, name[3], name[2], name[1], name[0]);
                    }
                }
            }
            info = kmpp_trie_get_info_next(impl->hook, info);
        }

        info = kmpp_trie_get_info_first(impl->hook);
        while (info) {
            name = kmpp_trie_info_name(info);
            if (kmpp_trie_info_name_is_self(name)) {
                void *p = kmpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                if (flag & OBJDEF_DUMP_SELF) {
                    if (info->ctx_len == sizeof(rk_u32)) {
                        kmpp_logi("trie %-2d - %-16s val %d\n", idx, name, *(rk_u32 *)p);
                    } else {
                        kmpp_logi("trie %-2d - %-16s str %s\n", idx, name, p);
                    }
                }
            }
            info = kmpp_trie_get_info_next(impl->hook, info);
        }
    }

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_objdef_dump);

void kmpp_objdef_dump_all(void)
{
    KmppObjSrv *srv = get_obj_srv_f();
    KmppObjDefImpl *impl = NULL;
    KmppObjDefImpl *n = NULL;

    osal_list_for_each_entry_safe(impl, n, &srv->list, KmppObjDefImpl, list) {
        kmpp_objdef_dump(impl, OBJDEF_DUMP_ALL);
    }
}
EXPORT_SYMBOL(kmpp_objdef_dump_all);

const rk_u8 *kmpp_objdef_get_name(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->name : NULL;
}
EXPORT_SYMBOL(kmpp_objdef_get_name);

rk_s32 kmpp_objdef_get_buf_size(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->buf_size : 0;
}
EXPORT_SYMBOL(kmpp_objdef_get_buf_size);

rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->entry_size : 0;
}
EXPORT_SYMBOL(kmpp_objdef_get_entry_size);

KmppTrie kmpp_objdef_get_trie(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    return impl ? impl->trie : NULL;
}
EXPORT_SYMBOL(kmpp_objdef_get_trie);

/* Allow objdef to be shared with userspace */
rk_s32 kmpp_objdef_share(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    if (impl)
        impl->shared = 1;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_objdef_share);

rk_s32 kmpp_objdefset_get(KmppObjDefSet **defs)
{
    KmppObjSrv *srv = get_obj_srv_f();
    KmppObjDefInfo *info = NULL;
    KmppObjDefSet *impl = NULL;
    KmppObjDefImpl *def, *n;
    KmppTrie trie = NULL;
    rk_s32 share_count;
    rk_s32 buf_offset;
    rk_s32 buf_size;
    rk_s32 size;
    rk_s32 ret = rk_nok;
    rk_s32 i = 0;

    if (!defs) {
        kmpp_loge_f("invalid param defs %p\n", defs);
        return ret;
    }

    share_count = 0;
    osal_list_for_each_entry_safe(def, n, &srv->list, KmppObjDefImpl, list) {
        if (!def->shared || !def->trie)
            continue;

        share_count++;
    }

    if (!share_count) {
        *defs = NULL;
        return rk_ok;
    }

    info = kmpp_calloc(sizeof(KmppObjDefInfo) * share_count);
    if (!info) {
        kmpp_loge_f("malloc %d objdef info size %d failed\n",
                    share_count, sizeof(KmppObjDefInfo) * share_count);
        goto done;
    }

    ret = kmpp_trie_init(&trie, "kmpp_objs");
    if (ret) {
        kmpp_loge_f("trie init failed\n");
        goto done;
    }

    buf_offset = 0;

    osal_list_for_each_entry_safe(def, n, &srv->list, KmppObjDefImpl, list) {
        if (!def->shared || !def->trie)
            continue;

        info[i].def = def;
        info[i].name = def->name;
        info[i].root = kmpp_trie_get_node_root(def->trie);
        info[i].buf_size = kmpp_trie_get_buf_size(def->trie);
        info[i].offset = buf_offset;

        kmpp_trie_add_info(trie, def->name, &buf_offset, sizeof(buf_offset));

        buf_offset += info[i].buf_size;

        obj_dbg_share("add %d:%d objdef %-16s root %px size %d offset %d\n",
                      share_count, i, info[i].name, info[i].root,
                      info[i].buf_size, buf_offset);
        i++;
    }
    buf_size = buf_offset;

    /* Add objdef count */
    kmpp_trie_add_info(trie, "__count", &share_count, sizeof(share_count));
    /* Add KmppShm private info and shm header size info to trie */
    kmpp_shm_add_trie_info(trie);
    /* End trie define and compress trie */
    kmpp_trie_add_info(trie, NULL, NULL, 0);

    /* prepare final objdef set */
    size = sizeof(KmppObjDefSet) + share_count * sizeof(KmppObjDef);
    impl = (KmppObjDefSet *)kmpp_calloc(size);
    if (!impl) {
        kmpp_loge_f("malloc %d objdef size %d failed\n", share_count, size);
        goto done;
    }

    /* compress done and create a new trie for share */
    {
        KmppTrie new_trie = NULL;
        void *root = kmpp_trie_get_node_root(trie);
        rk_s32 trie_size = kmpp_trie_get_buf_size(trie);
        void *kbase;

        obj_dbg_share("trie size %d buf size %d\n", trie_size, buf_size);

        size = trie_size + buf_size;
        kbase = kmpp_malloc_share(size);
        if (!kbase) {
            kmpp_loge_f("new trie and share buffer create failed\n");
            goto done;
        }

        osal_memcpy(kbase, root, trie_size);

        ret = kmpp_trie_init_by_root(&new_trie, kbase);
        if (ret || !new_trie) {
            kmpp_loge_f("new trie init by root failed\n");
            kmpp_free_share(kbase);
            goto done;
        }

        /* copy all objdef trie data and update trie shm info */
        for (i = 0; i < share_count; i++) {
            KmppObjDefInfo *p = &info[i];
            KmppTrieInfo *tr = kmpp_trie_get_info(new_trie, p->name);
            rk_s32 *val = kmpp_trie_info_ctx(tr);

            /* update trie root base */
            val[0] = p->offset + trie_size;
            impl->defs[i] = p->def;
            osal_memcpy(kbase + val[0], p->root, p->buf_size);
        }

        impl->trie = new_trie;
        impl->buf_size = size;
        impl->count = share_count;
    }
    kmpp_trie_deinit(trie);

    ret = rk_ok;

done:
    if (ret) {
        kmpp_free(impl);
        if (trie) {
            kmpp_trie_deinit(trie);
            trie = NULL;
        }
    }
    kmpp_free(info);

    *defs = impl;

    return ret;
}
EXPORT_SYMBOL(kmpp_objdefset_get);

rk_s32 kmpp_objdefset_put(KmppObjDefSet *defs)
{
    if (defs) {
        KmppTrie trie = defs->trie;
        void *buf = kmpp_trie_get_node_root(trie);

        kmpp_trie_deinit(trie);
        kmpp_free_share(buf);
        kmpp_free(defs);
    }

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_objdefset_put);

void kmpp_objdefset_dump(KmppObjDefSet *defs)
{
    if (defs) {
        rk_s32 i;

        kmpp_logi("dump objdefset %#px count %d buf size %d\n",
                  defs, defs->count, defs->buf_size);

        for (i = 0; i < defs->count; i++)
            kmpp_objdef_dump(defs->defs[i], OBJDEF_DUMP_INFO);
    }
}
EXPORT_SYMBOL(kmpp_objdefset_dump);

rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def, const rk_u8 *caller)
{
    KmppObjDefImpl *impl_def;
    KmppObjImpl *impl;

    if (!obj || !def) {
        kmpp_loge_f("invalid param obj %p def %p\n", obj, def);
        return rk_nok;
    }

    *obj = NULL;
    impl_def = (KmppObjDefImpl *)def;
    if (!impl_def->pool) {
        kmpp_loge_f("objdef %s pool is not ready\n", impl_def->name);
        return rk_nok;
    }
    impl = (KmppObjImpl *)kmpp_mem_pool_get(impl_def->pool, caller);
    if (!impl) {
        kmpp_loge_f("malloc %d failed\n", impl_def->buf_size);
        return rk_nok;
    }

    impl->name_check = impl_def->name_check;
    impl->def = def;
    impl->trie = impl_def->trie;
    impl->pool = impl_def->pool;
    impl->shm = NULL;
    impl->entry = (void *)(impl + 1);

    if (impl_def->init)
        impl_def->init(impl->entry, impl, NULL, caller);

    *obj = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_get);

rk_s32 kmpp_obj_get_by_name(KmppObj *obj, const rk_u8 *name, const rk_u8 *caller)
{
    KmppObjDef def;

    if (!obj || !name) {
        kmpp_loge_f("invalid param obj %p name %s\n", obj, name);
        return rk_nok;
    }

    def = find_objdef(name);
    if (!def) {
        kmpp_loge_f("can not get objdef %s\n", name);
        return rk_nok;
    }

    return kmpp_obj_get(obj, def, caller);
}
EXPORT_SYMBOL(kmpp_obj_get_by_name);

rk_s32 kmpp_obj_assign(KmppObj *obj, KmppObjDef def, void *buf, rk_s32 size, const rk_u8 *caller)
{
    KmppObjSrv *srv = get_obj_srv(caller);
    KmppObjImpl *impl;
    KmppObjDefImpl *impl_def;

    if (!obj || !def || !buf || !size) {
        kmpp_loge_f("invalid param obj %p def %p buf %p size %d\n", obj, def, buf, size);
        return rk_nok;
    }

    *obj = NULL;
    impl = (KmppObjImpl *)kmpp_mem_pool_get_f(srv->head_pool);
    impl_def = (KmppObjDefImpl *)def;
    if (size != impl_def->entry_size) {
        kmpp_loge_f("mismatch size %d def size %d\n", size, impl_def->buf_size);
        return rk_nok;
    }
    impl->def = def;
    impl->trie = impl_def->trie;
    impl->pool = srv->head_pool;
    impl->shm = NULL;
    impl->entry = buf;

    if (impl_def->init)
        impl_def->init(impl->entry, impl, NULL, caller);

    *obj = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_assign);

rk_s32 kmpp_obj_get_share(KmppObj *obj, KmppObjDef def, osal_fs_dev *file, const rk_u8 *caller)
{
    KmppObjSrv *srv = get_obj_srv(caller);
    KmppObjDefImpl *impl_def = (KmppObjDefImpl *)def;
    KmppObjImpl *impl = NULL;
    KmppShm shm = NULL;

    if (!srv || !obj || !def || !file) {
        kmpp_loge_f("invalid srv %px param obj %p def %p file %px\n",
                    srv, obj, def, file);
        return rk_nok;
    }

    *obj = NULL;

    kmpp_shm_get(&shm, file, impl_def->name);
    if (!shm) {
        kmpp_loge_f("kmpp_shm_get %s failed\n", impl_def->name);
        return rk_nok;
    }

    impl = (KmppObjImpl *)kmpp_mem_pool_get_f(srv->head_pool);
    if (!impl) {
        kmpp_loge_f("malloc obj head size %d failed\n", sizeof(KmppObjImpl));
        return rk_nok;
    }

    impl->name_check = impl_def->name_check;
    impl->def = def;
    impl->trie = impl_def->trie;
    impl->pool = srv->head_pool;
    impl->shm = shm;
    impl->entry = kmpp_shm_get_kaddr(shm);

    obj_dbg_share("obj %s get share %px [u:k] %#llx:%#llx\n", impl_def->name,
                  shm, kmpp_shm_get_uaddr(shm), kmpp_shm_get_kaddr(shm));

    kmpp_shm_set_kpriv(shm, impl);

    if (impl_def->init)
        impl_def->init(impl->entry, impl, file, caller);

    *obj = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_get_share);

rk_s32 kmpp_obj_put(KmppObj obj, const rk_u8 *caller)
{
    if (obj) {
        KmppObjImpl *impl = (KmppObjImpl *)obj;
        KmppObjDefImpl *def = impl->def;

        if (!def) {
            kmpp_loge_f("obj %px has empty def at %s\n", impl, caller);
            return rk_nok;
        }

        /* handle object implement reference here */
        if (def->deinit) {
            if (def->deinit(impl->entry, caller))
                return rk_ok;
        }

        if (impl->shm) {
            kmpp_shm_set_kpriv(impl->shm, NULL);
            kmpp_shm_put(impl->shm);
            impl->shm = NULL;
        }

        if (impl->pool) {
            kmpp_mem_pool_put(impl->pool, impl, caller);
            impl->pool = NULL;
        }

        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_put);

rk_s32 kmpp_obj_check(KmppObj obj, const rk_u8 *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!impl) {
        kmpp_loge_f("from %s failed for NULL arg\n", caller);
        return rk_nok;
    }

    if (!impl->name_check || !impl->def ||
        impl->name_check != impl->def->name_check) {
        kmpp_loge_f("from %s failed for name check %s but %s\n", caller,
                    impl->def ? impl->def->name_check : NULL, impl->name_check);
        return rk_nok;
    }

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_check);

rk_s32 kmpp_obj_reset(KmppObj obj, const rk_u8 *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!impl || !impl->def) {
        kmpp_loge_f("from %s failed for NULL arg\n", caller);
        return rk_nok;
    }

    if (impl->def->init)
        return impl->def->init(impl->entry, impl, NULL, caller);

    osal_memset(impl->entry, 0, impl->def->entry_size);
    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_reset);

void *kmpp_obj_to_entry(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return impl ? impl->entry : NULL;
}
EXPORT_SYMBOL(kmpp_obj_to_entry);

void *kmpp_obj_to_flags(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (impl && impl->def && impl->def->flag_offset)
        return impl->entry + impl->def->flag_offset;

    return NULL;
}
EXPORT_SYMBOL(kmpp_obj_to_flags);

KmppShm kmpp_obj_to_shm(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return impl ? impl->shm : NULL;
}
EXPORT_SYMBOL(kmpp_obj_to_shm);

rk_s32 kmpp_obj_to_shmptr(KmppObj obj, KmppShmPtr *sptr)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (!sptr)
        return rk_nok;

    if (!impl) {
        sptr->uaddr = 0;
        sptr->kaddr = 0;
        return rk_ok;
    }

    if (impl->shm) {
        KmppShmPtr *p = kmpp_shm_to_shmptr(impl->shm);

        sptr->uaddr = p->uaddr;
        sptr->kaddr = p->kaddr;
    } else {
        sptr->uaddr = 0;
        sptr->kaddr = (rk_u64)(uintptr_t)impl;
    }

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_to_shmptr);

KmppObj kmpp_obj_from_shmptr(KmppShmPtr *sptr)
{
    if (!sptr || !sptr->kptr)
        return NULL;

    if (sptr->uaddr) {
        KmppShm shm = sptr->kptr;

        return kmpp_shm_get_kpriv(shm);
    }

    return sptr->kptr;
}
EXPORT_SYMBOL(kmpp_obj_from_shmptr);

#define KMPP_OBJ_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_set_##type(KmppObj obj, const rk_u8 *name, base_type val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            kmpp_loge("obj %s set %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_set_##type); \
    rk_s32 kmpp_obj_get_##type(KmppObj obj, const rk_u8 *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            kmpp_loge("obj %s get %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_get_##type);

KMPP_OBJ_ACCESS(s32, rk_s32)
KMPP_OBJ_ACCESS(u32, rk_u32)
KMPP_OBJ_ACCESS(s64, rk_s64)
KMPP_OBJ_ACCESS(u64, rk_u64)
KMPP_OBJ_ACCESS(kobj, KmppObj)
KMPP_OBJ_ACCESS(kptr, void *)
KMPP_OBJ_ACCESS(kfp, void *)
KMPP_OBJ_ACCESS(uobj, rk_u64)
KMPP_OBJ_ACCESS(uptr, rk_u64)
KMPP_OBJ_ACCESS(ufp, rk_u64)

#define KMPP_OBJ_ACCESS_ST(type, base_type) \
    rk_s32 kmpp_obj_set_##type(KmppObj obj, const rk_u8 *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            kmpp_loge("obj %s set %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_set_##type); \
    rk_s32 kmpp_obj_get_##type(KmppObj obj, const rk_u8 *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            kmpp_loge("obj %s get %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_get_##type);

KMPP_OBJ_ACCESS_ST(st, void)
KMPP_OBJ_ACCESS_ST(shm, KmppShmPtr)

#define KMPP_OBJ_TBL_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppEntry *tbl, base_type val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl) \
            ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
        if (ret) \
            kmpp_loge("obj %s tbl %08x set " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_tbl_set_##type); \
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppEntry *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl) \
            ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
        if (ret) \
            kmpp_loge("obj %s tbl %08x get " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_tbl_get_##type);

KMPP_OBJ_TBL_ACCESS(s32, rk_s32)
KMPP_OBJ_TBL_ACCESS(u32, rk_u32)
KMPP_OBJ_TBL_ACCESS(s64, rk_s64)
KMPP_OBJ_TBL_ACCESS(u64, rk_u64)
KMPP_OBJ_TBL_ACCESS(kobj, KmppObj)
KMPP_OBJ_TBL_ACCESS(kptr, void *)
KMPP_OBJ_TBL_ACCESS(kfp, void *)
KMPP_OBJ_TBL_ACCESS(uobj, rk_u64)
KMPP_OBJ_TBL_ACCESS(uptr, rk_u64)
KMPP_OBJ_TBL_ACCESS(ufp, rk_u64)

#define KMPP_OBJ_TBL_ACCESS_ST(type, base_type) \
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppEntry *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl) \
            ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
        if (ret) \
            kmpp_loge("obj %s tbl %08x set " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_tbl_set_##type); \
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppEntry *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl) \
            ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
        if (ret) \
            kmpp_loge("obj %s tbl %08x get " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return ret; \
    } \
    EXPORT_SYMBOL(kmpp_obj_tbl_get_##type);

KMPP_OBJ_TBL_ACCESS_ST(st, void)
KMPP_OBJ_TBL_ACCESS_ST(shm, KmppShmPtr)

rk_s32 kmpp_obj_test(KmppObj obj, const rk_u8 *name)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (impl && impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info);

            return ENTRY_TEST_FLAG(tbl, impl->entry);;
        }
    }

    return 0;
}
EXPORT_SYMBOL(kmpp_obj_test);

rk_s32 kmpp_obj_tbl_test(KmppObj obj, KmppEntry *tbl)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return (impl && tbl) ? ENTRY_TEST_FLAG(tbl, impl->entry) : 0;
}
EXPORT_SYMBOL(kmpp_obj_tbl_test);

rk_s32 kmpp_obj_update(KmppObj dst, KmppObj src)
{
    KmppObjImpl *dst_impl = (KmppObjImpl *)dst;
    KmppObjImpl *src_impl = (KmppObjImpl *)src;
    KmppTrie trie = NULL;
    KmppTrieInfo *info = NULL;

    if (kmpp_obj_check_f(src) || kmpp_obj_check_f(dst) || src_impl->def != dst_impl->def) {
        kmpp_loge_f("obj %p update to %p failed invalid param\n", src, dst);
        return rk_nok;
    }

    trie = src_impl->def->trie;

    info = kmpp_trie_get_info_first(trie);
    do {
        KmppEntry *e;

        if (kmpp_trie_info_is_self(info))
            continue;

        e = (KmppEntry *)kmpp_trie_info_ctx(info);
        if (ENTRY_TEST_FLAG(e, src_impl->entry)) {
            rk_s32 offset = e->tbl.elem_offset;
            rk_s32 size = e->tbl.elem_size;

            osal_memcpy(dst_impl->entry + offset, src_impl->entry + offset, size);
        }
    } while ((info = kmpp_trie_get_info_next(trie, info)));

    return rk_ok;
}

rk_s32 kmpp_obj_update_entry(void *entry, KmppObj src)
{
    KmppObjImpl *src_impl = (KmppObjImpl *)src;
    KmppTrie trie = NULL;
    KmppTrieInfo *info = NULL;

    if (kmpp_obj_check_f(src) || !entry) {
        kmpp_loge_f("obj %p update to entry %p failed invalid param\n", src, entry);
        return rk_nok;
    }

    trie = src_impl->def->trie;

    info = kmpp_trie_get_info_first(trie);
    do {
        KmppEntry *e;

        if (kmpp_trie_info_is_self(info))
            continue;

        e = (KmppEntry *)kmpp_trie_info_ctx(info);
        if (ENTRY_TEST_FLAG(e, src_impl->entry)) {
            rk_s32 offset = e->tbl.elem_offset;
            rk_s32 size = e->tbl.elem_size;

            osal_memcpy(entry + offset, src_impl->entry + offset, size);
        }
    } while ((info = kmpp_trie_get_info_next(trie, info)));

    return rk_ok;
}

static inline rk_s32 kmpp_obj_impl_run(rk_s32 (*run)(void *ctx, void *arg), void *ctx, void *arg)
{
    return run(ctx, arg);
}

rk_s32 kmpp_obj_run(KmppObj obj, const rk_u8 *name, void *arg)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);
        void *val = NULL;

        if (info) {
            KmppEntry *tbl = (KmppEntry *)kmpp_trie_info_ctx(info);

            ret = kmpp_obj_impl_get_kfp(tbl, impl->entry, &val);
        }

        if (val)
            ret = kmpp_obj_impl_run(val, impl->entry, arg);
    }

    return ret;
}
EXPORT_SYMBOL(kmpp_obj_run);

rk_s32 kmpp_obj_tbl_run(KmppObj obj, KmppEntry *tbl, void *arg)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    void *val = NULL;
    rk_s32 ret;

    ret = kmpp_obj_impl_get_kfp(tbl, impl->entry, &val);
    if (val)
        ret = kmpp_obj_impl_run(val, impl->entry, arg);

    return ret;
}
EXPORT_SYMBOL(kmpp_obj_tbl_run);

rk_s32 kmpp_obj_idx_run(KmppObj obj, rk_s32 idx, void *arg, const rk_u8 *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl) {
        KmppObjDefImpl *def = impl->def;

        if (def && idx >= 0 && idx < def->hook_cnt) {
            KmppObjHook hook = def->hooks[idx];

            if (hook)
                ret = hook(impl->entry, arg, caller);
            else {
                kmpp_loge_f("invalid NULL hook idx %d\n", idx);
            }
        } else {
            kmpp_loge_f("invalid objdef %px hook_cnt %d idx %d\n",
                        def, def ? def->hook_cnt : -1, idx);
        }
    } else {
        kmpp_loge_f("invalid NULL obj\n");
    }

    return ret;
}
EXPORT_SYMBOL(kmpp_obj_idx_run);

rk_s32 kmpp_obj_dump(KmppObj obj, const rk_u8 *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (impl && impl->def && impl->def->dump) {
        kmpp_logi_f("%s obj from %s\n", impl->def->name, caller);
        kmpp_logi_f("object - [u:k] %#llx : %px\n",
                    kmpp_shm_get_upriv(impl->shm), impl->entry);
        if (impl->shm)
            kmpp_logi_f("entry  - [u:k] %#llx : %px\n",
                        kmpp_shm_get_uaddr(impl->shm),
                        kmpp_shm_get_kaddr(impl->shm));
        return impl->def->dump(impl->entry);
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_dump);

