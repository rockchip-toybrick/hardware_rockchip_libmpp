/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_obj"

#include <linux/kernel.h>

#include "rk_list.h"
#include "kmpp_macro.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_string.h"
#include "kmpp_trie.h"

#include "kmpp_obj_impl.h"
#include "kmpp_shm.h"

#define KMPP_OBJ_DBG_SET                (0x00000001)
#define KMPP_OBJ_DBG_GET                (0x00000002)

#define kmpp_obj_dbg(flag, fmt, ...)    kmpp_dbg(kmpp_obj_debug, flag, fmt, ## __VA_ARGS__)

#define kmpp_obj_dbg_set(fmt, ...)      kmpp_obj_dbg(KMPP_OBJ_DBG_SET, fmt, ## __VA_ARGS__)
#define kmpp_obj_dbg_get(fmt, ...)      kmpp_obj_dbg(KMPP_OBJ_DBG_GET, fmt, ## __VA_ARGS__)

#define ENTRY_TO_PTR(tbl, entry)        ((rk_u8 *)entry + tbl->data_offset)
#define ENTRY_TO_s32_PTR(tbl, entry)    ((rk_s32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u32_PTR(tbl, entry)    ((rk_u32 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_s64_PTR(tbl, entry)    ((rk_s64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_u64_PTR(tbl, entry)    ((rk_u64 *)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_ptr_PTR(tbl, entry)    ((void **)ENTRY_TO_PTR(tbl, entry))
#define ENTRY_TO_fp_PTR(tbl, entry)     ((void **)ENTRY_TO_PTR(tbl, entry))

#define ENTRY_TO_FLAG_PTR(tbl, entry)   ((rk_u16 *)((rk_u8 *)entry + tbl->flag_offset))

typedef struct KmppObjDefImpl_t {
    osal_list_head list;
    rk_s32 ref_cnt;
    rk_s32 head_size;
    rk_s32 entry_size;
    rk_s32 buf_size;
    KmppTrie trie;
    KmppShmMgr shm_mgr;
    rk_s32 shm_bind;
    KmppObjPreset preset;
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
    /* malloc flag */
    rk_u32 need_free;
    /* implment body here */
    KmppShm shm;
    void *entry;
} KmppObjImpl;

//static rk_u32 kmpp_obj_debug = KMPP_OBJ_DBG_SET | KMPP_OBJ_DBG_GET;
static rk_u32 kmpp_obj_debug = 0;
/* NOTE: objdef get / put MUST on insmod / rmmod in spinlock */
static OSAL_LIST_HEAD(kmpp_objdef_list);
static rk_s32 kmpp_objdef_count = 0;

const rk_u8 *strof_entry_type(EntryType type)
{
    static const rk_u8 *entry_type_names[] = {
        "rk_s32",
        "rk_u32",
        "rk_s64",
        "rk_u64",
        "void *",
        "struct",
        "invalid"
    };

    if (type < 0 || type >= ENTRY_TYPE_BUTT)
        type = ENTRY_TYPE_BUTT;

    return entry_type_names[type];
}

#define KMPP_OBJ_ACCESS_IMPL(type, base_type, log_str) \
    rk_s32 kmpp_obj_impl_set_##type(KmppLocTbl *tbl, void *entry, base_type val) \
    { \
        base_type *dst = ENTRY_TO_##type##_PTR(tbl, entry); \
        base_type old = dst[0]; \
        dst[0] = val; \
        if (!tbl->flag_type) { \
            kmpp_obj_dbg_set("%px + %x set " #type " change " #log_str " -> " #log_str "\n", entry, tbl->data_offset, old, val); \
        } else { \
            if (old != val) { \
                kmpp_obj_dbg_set("%px + %x set " #type " update " #log_str " -> " #log_str " flag %d|%x\n", \
                                entry, tbl->data_offset, old, val, tbl->flag_offset, tbl->flag_value); \
                ENTRY_TO_FLAG_PTR(tbl, entry)[0] |= tbl->flag_value; \
            } else { \
                kmpp_obj_dbg_set("%px + %x set " #type " keep   " #log_str "\n", entry, tbl->data_offset, old); \
            } \
        } \
        return rk_ok; \
    } \
    rk_s32 kmpp_obj_impl_get_##type(KmppLocTbl *tbl, void *entry, base_type *val) \
    { \
        if (tbl && tbl->data_size) { \
            base_type *src = ENTRY_TO_##type##_PTR(tbl, entry); \
            kmpp_obj_dbg_get("%px + %x get " #type " value  " #log_str "\n", entry, tbl->data_offset, src[0]); \
            val[0] = src[0]; \
            return rk_ok; \
        } \
        return rk_nok; \
    }

KMPP_OBJ_ACCESS_IMPL(s32, rk_s32, %d)
KMPP_OBJ_ACCESS_IMPL(u32, rk_u32, %u)
KMPP_OBJ_ACCESS_IMPL(s64, rk_s64, %llx)
KMPP_OBJ_ACCESS_IMPL(u64, rk_u64, %llx)
KMPP_OBJ_ACCESS_IMPL(ptr, void *, %px)
KMPP_OBJ_ACCESS_IMPL(fp, void *, %px)

rk_s32 kmpp_obj_impl_set_st(KmppLocTbl *tbl, void *entry, void *val)
{
    rk_u8 *dst = ENTRY_TO_PTR(tbl, entry);

    if (!tbl->flag_type) {
        /* simple copy */
        kmpp_obj_dbg_set("%px + %x set struct change %px -> %px\n", entry, tbl->data_offset, dst, val);
        osal_memcpy(dst, val, tbl->data_size);
        return rk_ok;
    }

    /* copy with flag check and updata */
    if (osal_memcmp(dst, val, tbl->data_size)) {
        kmpp_obj_dbg_set("%px + %x set struct update %px -> %px flag %d|%x\n",
                         entry, tbl->data_offset, dst, val, tbl->flag_offset, tbl->flag_value);
        osal_memcpy(dst, val, tbl->data_size);
        ENTRY_TO_FLAG_PTR(tbl, entry)[0] |= tbl->flag_value;
    } else {
        kmpp_obj_dbg_set("%px + %x set struct keep   %px\n", entry, tbl->data_offset, dst);
    }

    return rk_ok;
}

rk_s32 kmpp_obj_impl_get_st(KmppLocTbl *tbl, void *entry, void *val)
{
    if (tbl && tbl->data_size) {
        void *src = (void *)ENTRY_TO_PTR(tbl, entry);

        kmpp_obj_dbg_get("%px + %d get st from %px\n", entry, tbl->data_offset, src);
        osal_memcpy(val, src, tbl->data_size);
        return rk_ok;
    }

    return rk_nok;
}

static void show_entry_tbl_err(KmppLocTbl *tbl, EntryType type, const rk_u8 *func, const rk_u8 *name)
{
    kmpp_loge("%s entry %s expect %s input NOT %s\n", func, name,
              strof_entry_type(tbl->data_type), strof_entry_type(type));
}

rk_s32 check_entry_tbl(KmppLocTbl *tbl, const rk_u8 *name, EntryType type,
                     const rk_u8 *func)
{
    EntryType entry_type;
    rk_s32 entry_size;
    rk_s32 ret;

    if (!tbl) {
        kmpp_loge("%s: entry %s is invalid\n", func, name);
        return rk_nok;
    }

    entry_type = tbl->data_type;
    entry_size = tbl->data_size;
    ret = rk_ok;

    switch (type) {
    case ENTRY_TYPE_st : {
        if (entry_type != type) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
        if (entry_size <= 0) {
            kmpp_loge("%s: entry %s found invalid size %d\n", func, name, entry_size);
            ret = rk_nok;
        }
    } break;
    case ENTRY_TYPE_ptr :
    case ENTRY_TYPE_fp : {
        if (entry_type != type) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
    } break;
    case ENTRY_TYPE_s32 :
    case ENTRY_TYPE_u32 : {
        if (entry_size != sizeof(rk_s32)) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
    } break;
    case ENTRY_TYPE_s64 :
    case ENTRY_TYPE_u64 : {
        if (entry_size != sizeof(rk_s64)) {
            show_entry_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
    } break;
    default : {
        kmpp_loge("%s: entry %s found invalid type %d\n", func, name, type);
        ret = rk_nok;
    } break;
    }

    return ret;
}

rk_s32 kmpp_objdef_get(KmppObjDef *def, rk_s32 size, const rk_u8 *name)
{
    KmppObjDefImpl *impl = NULL;
    rk_s32 offset;
    rk_s32 len;

    if (!def || !size || !name) {
        kmpp_loge_f("invalid param def %p size %d name %p\n", def, size, name);
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
    impl->head_size = sizeof(KmppObjImpl);
    impl->entry_size = size;
    impl->buf_size = size + sizeof(KmppObjImpl);
    osal_strncpy(impl->name, name, len);

    kmpp_trie_init(&impl->trie, name);
    /* record object impl size */
    kmpp_trie_add_info(impl->trie, "__size", &size, sizeof(size));
    offset = kmpp_shm_entry_offset();
    kmpp_trie_add_info(impl->trie, "__offset", &offset, sizeof(offset));

    OSAL_INIT_LIST_HEAD(&impl->list);
    osal_list_add_tail(&impl->list, &kmpp_objdef_list);
    kmpp_objdef_count++;
    impl->ref_cnt++;

    *def = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_objdef_get);

rk_s32 kmpp_objdef_put(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl) {
        impl->ref_cnt--;

        if (!impl->ref_cnt) {
            if (impl->shm_mgr) {
                if (impl->shm_bind)
                    kmpp_shm_mgr_unbind_objdef(impl->shm_mgr, def);
                else
                    kmpp_shm_mgr_put(impl->shm_mgr);

                impl->shm_mgr = NULL;
            }

            if (impl->trie)
                ret = kmpp_trie_deinit(impl->trie);

            osal_list_del_init(&impl->list);
            kmpp_objdef_count--;

            kmpp_free(impl);
        }

        ret = rk_ok;
    }

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_put);

rk_u32 kmpp_objdef_find(KmppObjDef *def, const rk_u8 *name)
{
    KmppObjDefImpl *impl = NULL;
    KmppObjDefImpl *n = NULL;

    osal_list_for_each_entry_safe(impl, n, &kmpp_objdef_list, KmppObjDefImpl, list) {
        if (osal_strcmp(impl->name, name) == 0) {
            impl->ref_cnt++;
            *def = impl;
            return rk_ok;
        }
    }

    *def = NULL;

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_find);

rk_s32 kmpp_objdef_add_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl *tbl)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl->trie)
        ret = kmpp_trie_add_info(impl->trie, name, tbl, sizeof(*tbl));

    if (ret)
        kmpp_loge_f("class %s add entry %s failed ret %d\n",
                    impl ? impl->name : NULL, name, ret);

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_add_entry);

rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const rk_u8 *name, KmppLocTbl **tbl)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        if (info) {
            *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info);
            ret = rk_ok;
        }
    }

    if (ret)
        kmpp_loge_f("class %s get entry %s failed ret %d\n",
                    impl ? impl->name : NULL, name, ret);

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_get_entry);

rk_s32 kmpp_objdef_add_preset(KmppObjDef def, KmppObjPreset preset)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

        impl->preset = preset;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_add_preset);

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

rk_s32 kmpp_objdef_bind_shm_mgr(KmppObjDef def)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
    KmppShmMgr mgr = NULL;
    rk_s32 ret = rk_nok;

    if (!impl) {
        kmpp_loge_f("invalid param obj def NULL\n");
        return rk_nok;
    }

    mgr = kmpp_shm_get_objs_mgr();
    if (!mgr) {
        kmpp_loge_f("kmpp_shm_get_objs_mgr failed\n");
        return ret;
    }

    ret = kmpp_shm_mgr_bind_objdef(mgr, def);
    if (ret) {
        kmpp_loge_f("bind kmpp_objs shm mgr failed ret %d\n", ret);
        mgr = NULL;
    }

    impl->shm_mgr = mgr;
    impl->shm_bind = 1;

    return ret;
}
EXPORT_SYMBOL(kmpp_objdef_bind_shm_mgr);

rk_s32 kmpp_objdef_dump(KmppObjDef def)
{
    if (def) {
        KmppObjDefImpl *impl = (KmppObjDefImpl *)def;
        KmppTrie trie = impl->trie;
        KmppTrieInfo *info = NULL;
        const rk_u8 *name = impl->name;
        RK_S32 i = 0;

        kmpp_logi("dump objdef %-16s entry_size %d element count %d\n",
                 name, impl->entry_size, kmpp_trie_get_info_count(trie));

        info = kmpp_trie_get_info_first(trie);
        while (info) {
            name = kmpp_trie_info_name(info);
            if (!osal_strstr(name, "__")) {
                KmppLocTbl *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                kmpp_logi("%-2d - %-16s offset %4d size %d\n", idx,
                          name, tbl->data_offset, tbl->data_size);
            }
            info = kmpp_trie_get_info_next(trie, info);
        }

        info = kmpp_trie_get_info_first(trie);
        while (info) {
            name = kmpp_trie_info_name(info);
            if (osal_strstr(name, "__")) {
                void *p = kmpp_trie_info_ctx(info);
                rk_s32 idx = i++;

                if (info->ctx_len == sizeof(RK_U32)) {

                    kmpp_logi("%-2d - %-16s val %d\n", idx, name, *(RK_U32 *)p);
                } else {
                    kmpp_logi("%-2d - %-16s str %s\n", idx, name, p);
                }
            }
            info = kmpp_trie_get_info_next(trie, info);
        }

        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_objdef_dump);

void kmpp_objdef_dump_all(void)
{
    KmppObjDefImpl *impl = NULL;
    KmppObjDefImpl *n = NULL;

    osal_list_for_each_entry_safe(impl, n, &kmpp_objdef_list, KmppObjDefImpl, list) {
        kmpp_objdef_dump(impl);
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

rk_s32 kmpp_obj_set_shm_mgr(KmppObjDef def, KmppShmMgr mgr)
{
    KmppObjDefImpl *impl = (KmppObjDefImpl *)def;

    if (impl) {
        impl->shm_mgr = mgr;
        return rk_ok;
    }

    return rk_nok;
}

rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def)
{
    KmppObjDefImpl *def_impl;
    KmppObjImpl *impl;

    if (!obj || !def) {
        kmpp_loge_f("invalid param obj %p def %p\n", obj, def);
        return rk_nok;
    }

    *obj = NULL;
    def_impl = (KmppObjDefImpl *)def;
    impl = (KmppObjImpl *)kmpp_calloc(def_impl->buf_size);
    if (!impl) {
        kmpp_loge_f("malloc %d failed\n", def_impl->buf_size);
        return rk_nok;
    }

    impl->name_check = def_impl->name_check;
    impl->def = def;
    impl->trie = def_impl->trie;
    impl->need_free = 1;
    impl->shm = NULL;
    impl->entry = (void *)(impl + 1);

    if (def_impl->preset)
        def_impl->preset(impl->entry);

    *obj = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_get);

rk_s32 kmpp_obj_assign(KmppObj *obj, KmppObjDef def, void *buf, rk_s32 size)
{
    KmppObjImpl *impl;
    KmppObjDefImpl *def_impl;

    if (!obj || !def || !buf || !size) {
        kmpp_loge_f("invalid param obj %p def %p buf %p size %d\n", obj, def, buf, size);
        return rk_nok;
    }

    *obj = NULL;
    impl = (KmppObjImpl *)buf;
    def_impl = (KmppObjDefImpl *)def;
    if (size != def_impl->buf_size) {
        kmpp_loge_f("mismatch size %d def size %d\n", size, def_impl->buf_size);
        return rk_nok;
    }
    impl->def = def;
    impl->trie =def_impl->trie;
    impl->need_free = 0;
    impl->shm = NULL;
    impl->entry = (void *)(impl + 1);

    if (def_impl->preset)
        def_impl->preset(impl->entry);

    *obj = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_assign);

rk_s32 kmpp_obj_get_share(KmppObj *obj, KmppObjDef def, KmppShmGrp grp)
{
    KmppObjDefImpl *def_impl;
    KmppObjImpl *impl = NULL;
    KmppShm shm = NULL;
    rk_s32 ret;

    if (!obj || !def) {
        kmpp_loge_f("invalid param obj %p def %p\n", obj, def);
        return rk_nok;
    }

    *obj = NULL;
    def_impl = (KmppObjDefImpl *)def;

    if (!def_impl->shm_mgr || kmpp_shm_grp_check(grp, def_impl->shm_mgr)) {
        kmpp_loge_f("%s found invalid shm mgr %px :and group %px\n",
                    def_impl->name, def_impl->shm_mgr, grp);
        return rk_nok;
    }

    impl = (KmppObjImpl *)kmpp_calloc(def_impl->head_size);
    if (!impl) {
        kmpp_loge_f("malloc obj head size %d failed\n", def_impl->head_size);
        return rk_nok;
    }

    impl->name_check = def_impl->name_check;
    impl->def = def;
    impl->trie = def_impl->trie;
    impl->need_free = 1;

    ret = kmpp_shm_get(&shm, grp, impl);
    if (ret || !shm) {
        kmpp_loge_f("%s kmpp_shm_get failed %d\n", def_impl->name, ret);
        kmpp_free(impl);
        return rk_nok;
    }

    impl->shm = shm;
    impl->entry = kmpp_shm_get_kaddr(shm);

    if (def_impl->preset)
        def_impl->preset(impl->entry);

    *obj = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_obj_get_share);

rk_s32 kmpp_obj_put(KmppObj obj)
{
    if (obj) {
        KmppObjImpl *impl = (KmppObjImpl *)obj;

        if (impl->shm) {
            kmpp_shm_put(impl->shm);
            impl->shm = NULL;
        }

        if (impl->need_free)
            kmpp_free(impl);

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

void *kmpp_obj_get_entry(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return impl ? impl->entry : NULL;
}
EXPORT_SYMBOL(kmpp_obj_get_entry);

KmppShm kmpp_obj_get_shm(KmppObj obj)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    return impl ? impl->shm : NULL;
}
EXPORT_SYMBOL(kmpp_obj_get_shm);

#define KMPP_OBJ_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_set_##type(KmppObj obj, const rk_u8 *name, base_type val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppLocTbl *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            kmpp_loge("obj %s set %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return rk_nok; \
    } \
    EXPORT_SYMBOL(kmpp_obj_set_##type); \
    rk_s32 kmpp_obj_get_##type(KmppObj obj, const rk_u8 *name, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info) { \
                KmppLocTbl *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info); \
                ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
            } \
        } \
        if (ret) \
            kmpp_loge("obj %s get %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return rk_nok; \
    } \
    EXPORT_SYMBOL(kmpp_obj_get_##type);

KMPP_OBJ_ACCESS(s32, rk_s32)
KMPP_OBJ_ACCESS(u32, rk_u32)
KMPP_OBJ_ACCESS(s64, rk_s64)
KMPP_OBJ_ACCESS(u64, rk_u64)
KMPP_OBJ_ACCESS(ptr, void *)
KMPP_OBJ_ACCESS(fp, void *)

rk_s32 kmpp_obj_set_st(KmppObj obj, const rk_u8 *name, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info);
            ret = kmpp_obj_impl_set_st(tbl, impl->entry, val);
        }
    }

    if (ret)
        kmpp_loge("obj %s set %s st failed ret %d\n",
                impl ? impl->def ? impl->def->name : NULL : NULL, name, ret);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_set_st);

rk_s32 kmpp_obj_get_st(KmppObj obj, const rk_u8 *name, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info);
            ret = kmpp_obj_impl_get_st(tbl, impl->entry, val);
        }
    }

    if (ret)
        kmpp_loge("obj %s get %s st failed ret %d\n",
                impl ? impl->def ? impl->def->name : NULL : NULL, name, ret);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_get_st);

#define KMPP_OBJ_TBL_ACCESS(type, base_type) \
    rk_s32 kmpp_obj_tbl_set_##type(KmppObj obj, KmppLocTbl *tbl, base_type val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl) \
            ret = kmpp_obj_impl_set_##type(tbl, impl->entry, val); \
        if (ret) \
            kmpp_loge("obj %s tbl %08x set " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return rk_nok; \
    } \
    EXPORT_SYMBOL(kmpp_obj_tbl_set_##type); \
    rk_s32 kmpp_obj_tbl_get_##type(KmppObj obj, KmppLocTbl *tbl, base_type *val) \
    { \
        KmppObjImpl *impl = (KmppObjImpl *)obj; \
        rk_s32 ret = rk_nok; \
        if (impl) \
            ret = kmpp_obj_impl_get_##type(tbl, impl->entry, val); \
        if (ret) \
            kmpp_loge("obj %s tbl %08x get " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret); \
        return rk_nok; \
    } \
    EXPORT_SYMBOL(kmpp_obj_tbl_get_##type);

KMPP_OBJ_TBL_ACCESS(s32, rk_s32)
KMPP_OBJ_TBL_ACCESS(u32, rk_u32)
KMPP_OBJ_TBL_ACCESS(s64, rk_s64)
KMPP_OBJ_TBL_ACCESS(u64, rk_u64)
KMPP_OBJ_TBL_ACCESS(ptr, void *)
KMPP_OBJ_TBL_ACCESS(fp, void *)

rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppLocTbl *tbl, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl)
        ret = kmpp_obj_impl_set_st(tbl, impl->entry, val);

    if (ret)
        kmpp_loge("obj %s tbl %08x set %s st failed ret %d\n",
                  impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_tbl_set_st);

rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppLocTbl *tbl, void *val)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl)
        ret = kmpp_obj_impl_get_st(tbl, impl->entry, val);

    if (ret)
        kmpp_loge("obj %s tbl %08x get %s st failed ret %d\n",
                  impl ? impl->def ? impl->def->name : NULL : NULL, tbl ? tbl->val : 0, ret);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_tbl_get_st);

static rk_s32 kmpp_obj_impl_run(rk_s32 (*run)(void *ctx), void *ctx)
{
    return run(ctx);
}

rk_s32 kmpp_obj_run(KmppObj obj, const rk_u8 *name)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);
        void *val = NULL;

        if (info) {
            KmppLocTbl *tbl = (KmppLocTbl *)kmpp_trie_info_ctx(info);

            ret = kmpp_obj_impl_get_fp(tbl, impl->entry, &val);
        }

        if (val)
            ret = kmpp_obj_impl_run(val, impl->entry);
    }

    return ret;
}
EXPORT_SYMBOL(kmpp_obj_run);

rk_s32 kmpp_obj_dump(KmppObj obj, const rk_u8 *caller)
{
    KmppObjImpl *impl = (KmppObjImpl *)obj;

    if (impl && impl->def && impl->def->dump) {
        kmpp_logi_f("%s obj %px entry %px from %s\n",
                  impl->def->name, impl, impl->entry, caller);
        return impl->def->dump(impl->entry);
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_obj_dump);

