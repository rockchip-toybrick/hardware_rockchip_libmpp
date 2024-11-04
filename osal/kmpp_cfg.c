/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_cfg"

#include <linux/kernel.h>

#include "rk_list.h"
#include "kmpp_macro.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_string.h"
#include "kmpp_trie.h"

#include "kmpp_cfg_impl.h"

#define MPP_CFG_DBG_SET             (0x00000001)
#define MPP_CFG_DBG_GET             (0x00000002)

#define kmpp_cfg_dbg(flag, fmt, ...) kmpp_dbg(kkmpp_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define kmpp_cfg_dbg_set(fmt, ...)   kmpp_cfg_dbg(MPP_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define kmpp_cfg_dbg_get(fmt, ...)   kmpp_cfg_dbg(MPP_CFG_DBG_GET, fmt, ## __VA_ARGS__)

#define CFG_TO_PTR(tbl, cfg)       ((rk_u8 *)cfg + tbl->data_offset)
#define CFG_TO_s32_PTR(tbl, cfg)   ((rk_s32 *)CFG_TO_PTR(tbl, cfg))
#define CFG_TO_u32_PTR(tbl, cfg)   ((rk_u32 *)CFG_TO_PTR(tbl, cfg))
#define CFG_TO_s64_PTR(tbl, cfg)   ((rk_s64 *)CFG_TO_PTR(tbl, cfg))
#define CFG_TO_u64_PTR(tbl, cfg)   ((rk_u64 *)CFG_TO_PTR(tbl, cfg))
#define CFG_TO_ptr_PTR(tbl, cfg)   ((void **)CFG_TO_PTR(tbl, cfg))

#define CFG_TO_FLAG_PTR(tbl, cfg)  ((rk_u16 *)((rk_u8 *)cfg + tbl->flag_offset))

typedef struct KmppCfgDefImpl_t {
    osal_list_head list;
    rk_s32 ref_cnt;
    rk_u32 cfg_size;
    rk_u32 buf_size;
    KmppTrie trie;
    KmppCfgPreset preset;
    KmppModuleFunc func;
    rk_u8 *name;
} KmppCfgDefImpl;

typedef struct KmppCfgImpl_t {
    /* class infomation link */
    KmppCfgDefImpl *def;
    KmppModuleFunc func;
    /* trie for fast access */
    KmppTrie trie;
    /* malloc flag */
    rk_u32 need_free;
    /* implment body here */
} KmppCfgImpl;

//static rk_u32 kkmpp_cfg_debug = MPP_CFG_DBG_SET | MPP_CFG_DBG_GET;
static rk_u32 kkmpp_cfg_debug = 0;
static OSAL_LIST_HEAD(kkmpp_cfg_list);

const rk_u8 *strof_cfgtype(CfgType type)
{
    static const rk_u8 *cfg_type_names[] = {
        "rk_s32",
        "rk_u32",
        "rk_s64",
        "rk_u64",
        "void *",
        "struct",
    };

    return cfg_type_names[type];
}

#define KMPP_CFG_ACCESS_IMPL(type, base_type, log_str) \
    rk_s32 kmpp_cfg_impl_set_##type(KmppLocTbl *tbl, void *cfg, base_type val) \
    { \
        base_type *dst = CFG_TO_##type##_PTR(tbl, cfg); \
        base_type old = dst[0]; \
        dst[0] = val; \
        if (!tbl->flag_type) { \
            kmpp_cfg_dbg_set("%px + %x set " #type " change " #log_str " -> " #log_str "\n", cfg, tbl->data_offset, old, val); \
        } else { \
            if (old != val) { \
                kmpp_cfg_dbg_set("%px + %x set " #type " update " #log_str " -> " #log_str " flag %d|%x\n", \
                                cfg, tbl->data_offset, old, val, tbl->flag_offset, tbl->flag_value); \
                CFG_TO_FLAG_PTR(tbl, cfg)[0] |= tbl->flag_value; \
            } else { \
                kmpp_cfg_dbg_set("%px + %x set " #type " keep   " #log_str "\n", cfg, tbl->data_offset, old); \
            } \
        } \
        return rk_ok; \
    } \
    rk_s32 kmpp_cfg_impl_get_##type(KmppLocTbl *tbl, void *cfg, base_type *val) \
    { \
        if (tbl && tbl->data_size) { \
            base_type *src = CFG_TO_##type##_PTR(tbl, cfg); \
            kmpp_cfg_dbg_get("%px + %x get " #type " value  " #log_str "\n", cfg, tbl->data_offset, src[0]); \
            val[0] = src[0]; \
            return rk_ok; \
        } \
        return rk_nok; \
    }

KMPP_CFG_ACCESS_IMPL(s32, rk_s32, %d)
KMPP_CFG_ACCESS_IMPL(u32, rk_u32, %u)
KMPP_CFG_ACCESS_IMPL(s64, rk_s64, %llx)
KMPP_CFG_ACCESS_IMPL(u64, rk_u64, %llx)
KMPP_CFG_ACCESS_IMPL(ptr, void *, %px)

rk_s32 kmpp_cfg_impl_set_st(KmppLocTbl *tbl, void *cfg, void *val)
{
    rk_u8 *dst = CFG_TO_PTR(tbl, cfg);

    if (!tbl->flag_type) {
        /* simple copy */
        kmpp_cfg_dbg_set("%px + %x set struct change %px -> %px\n", cfg, tbl->data_offset, dst, val);
        osal_memcpy(dst, val, tbl->data_size);
        return rk_ok;
    }

    /* copy with flag check and updata */
    if (osal_memcmp(dst, val, tbl->data_size)) {
        kmpp_cfg_dbg_set("%px + %x set struct update %px -> %px flag %d|%x\n",
                         cfg, tbl->data_offset, dst, val, tbl->flag_offset, tbl->flag_value);
        osal_memcpy(dst, val, tbl->data_size);
        CFG_TO_FLAG_PTR(tbl, cfg)[0] |= tbl->flag_value;
    } else {
        kmpp_cfg_dbg_set("%px + %x set struct keep   %px\n", cfg, tbl->data_offset, dst);
    }

    return rk_ok;
}

rk_s32 kmpp_cfg_impl_get_st(KmppLocTbl *tbl, void *cfg, void *val)
{
    if (tbl && tbl->data_size) {
        void *src = (void *)CFG_TO_PTR(tbl, cfg);

        kmpp_cfg_dbg_get("%px + %d get st from %px\n", cfg, tbl->data_offset, src);
        osal_memcpy(val, src, tbl->data_size);
        return rk_ok;
    }
    return rk_nok;
}

static void show_cfg_tbl_err(KmppLocTbl *tbl, CfgType type, const rk_u8 *func, const rk_u8 *name)
{
    kmpp_loge("%s cfg %s expect %s input NOT %s\n", func, name,
              strof_cfgtype(tbl->data_type), strof_cfgtype(type));
}

rk_s32 check_cfg_tbl(KmppLocTbl *tbl, const rk_u8 *name, CfgType type,
                     const rk_u8 *func)
{
    CfgType cfg_type;
    rk_s32 cfg_size;
    rk_s32 ret;

    if (!tbl) {
        kmpp_loge("%s: cfg %s is invalid\n", func, name);
        return rk_nok;
    }

    cfg_type = tbl->data_type;
    cfg_size = tbl->data_size;
    ret = rk_ok;

    switch (type) {
    case CFG_TYPE_st : {
        if (cfg_type != type) {
            show_cfg_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
        if (cfg_size <= 0) {
            kmpp_loge("%s: cfg %s found invalid size %d\n", func, name, cfg_size);
            ret = rk_nok;
        }
    } break;
    case CFG_TYPE_ptr : {
        if (cfg_type != type) {
            show_cfg_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
    } break;
    case CFG_TYPE_s32 :
    case CFG_TYPE_u32 : {
        if (cfg_size != sizeof(rk_s32)) {
            show_cfg_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
    } break;
    case CFG_TYPE_s64 :
    case CFG_TYPE_u64 : {
        if (cfg_size != sizeof(rk_s64)) {
            show_cfg_tbl_err(tbl, type, func, name);
            ret = rk_nok;
        }
    } break;
    default : {
        kmpp_loge("%s: cfg %s found invalid cfg type %d\n", func, name, type);
        ret = rk_nok;
    } break;
    }

    return ret;
}

rk_s32 kmpp_cfg_init(KmppCfgDef *def, rk_u32 size, const rk_u8 *name)
{
    KmppCfgDefImpl *impl = NULL;
    rk_s32 len;

    if (!def || !size || !name) {
        kmpp_loge_f("invalid param def %p size %d name %p\n", def, size, name);
        return rk_nok;
    }

    *def = NULL;
    len = KMPP_ALIGN(osal_strlen(name) + 1, sizeof(rk_u32));
    impl = (KmppCfgDefImpl *)kmpp_calloc(sizeof(KmppCfgDefImpl) + len);
    if (!impl) {
        kmpp_loge_f("malloc contex %d failed\n", sizeof(KmppCfgDefImpl) + len);
        return rk_nok;
    }

    impl->name = (rk_u8 *)(impl + 1);
    impl->cfg_size = size;
    impl->buf_size = size + sizeof(KmppCfgImpl);
    osal_strncpy(impl->name, name, len);
    kmpp_trie_init(&impl->trie, sizeof(KmppLocTbl));
    *def = impl;

    OSAL_INIT_LIST_HEAD(&impl->list);
    osal_list_add_tail(&impl->list, &kkmpp_cfg_list);
    impl->ref_cnt++;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_cfg_init);

rk_s32 kmpp_cfg_add_entry(KmppCfgDef def, const rk_u8 *name, KmppLocTbl *tbl)
{
    KmppCfgDefImpl *impl = (KmppCfgDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl->trie)
        ret = kmpp_trie_add_info(impl->trie, name, tbl);

    if (ret)
        kmpp_loge_f("class %s add entry %s failed ret %d\n",
                    impl ? impl->name : NULL, name, ret);

    return ret;
}
EXPORT_SYMBOL(kmpp_cfg_add_entry);

rk_s32 kmpp_cfg_add_preset(KmppCfgDef def, KmppCfgPreset preset)
{
    if (def) {
        KmppCfgDefImpl *impl = (KmppCfgDefImpl *)def;

        impl->preset = preset;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_cfg_add_preset);

rk_s32 kmpp_cfg_add_func(KmppCfgDef def, KmppModuleFunc func)
{
    if (def) {
        KmppCfgDefImpl *impl = (KmppCfgDefImpl *)def;

        impl->func = func;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_cfg_add_func);

rk_s32 kmpp_cfg_deinit(KmppCfgDef def)
{
    KmppCfgDefImpl *impl = (KmppCfgDefImpl *)def;
    rk_s32 ret = rk_nok;

    if (impl) {
        impl->ref_cnt--;

        if (!impl->ref_cnt) {
            if (impl->trie)
                ret = kmpp_trie_deinit(impl->trie);

            osal_list_del_init(&impl->list);

            kmpp_free(impl);
        }

        ret = rk_ok;
    }

    return ret;
}
EXPORT_SYMBOL(kmpp_cfg_deinit);

rk_u32 kmpp_cfg_size(KmppCfgDef def)
{
    KmppCfgDefImpl *impl = (KmppCfgDefImpl *)def;

    return impl ? impl->buf_size : 0;
}
EXPORT_SYMBOL(kmpp_cfg_size);

rk_u32 kmpp_cfg_lookup(KmppCfgDef *def, const rk_u8 *name)
{
    KmppCfgDefImpl *impl = NULL;
    KmppCfgDefImpl *n = NULL;

    osal_list_for_each_entry_safe(impl, n, &kkmpp_cfg_list, KmppCfgDefImpl, list) {
        if (osal_strcmp(impl->name, name) == 0) {
            impl->ref_cnt++;
            *def = impl;
            return rk_ok;
        }
    }

    *def = NULL;

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_cfg_lookup);

rk_s32 kmpp_cfg_get(KmppCfg *cfg, KmppCfgDef def)
{
    KmppCfgImpl *impl;
    KmppCfgDefImpl *def_impl;

    if (!cfg || !def) {
        kmpp_loge_f("invalid param cfg %p def %p\n", cfg, def);
        return rk_nok;
    }

    *cfg = NULL;
    def_impl = (KmppCfgDefImpl *)def;
    impl = (KmppCfgImpl *)kmpp_calloc(def_impl->buf_size);
    if (!impl) {
        kmpp_loge_f("malloc %d failed\n", def_impl->buf_size);
        return rk_nok;
    }

    impl->def = def;
    impl->func = def_impl->func;
    impl->trie = def_impl->trie;
    impl->need_free = 1;

    if (def_impl->preset)
        def_impl->preset(impl + 1);

    *cfg = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_cfg_get);

rk_s32 kmpp_cfg_assign(KmppCfg *cfg, KmppCfgDef def, void *buf, rk_u32 size)
{
    KmppCfgImpl *impl;
    KmppCfgDefImpl *def_impl;

    if (!cfg || !def || !buf || !size) {
        kmpp_loge_f("invalid param cfg %p def %p buf %p size %d\n", cfg, def, buf, size);
        return rk_nok;
    }

    *cfg = NULL;
    impl = (KmppCfgImpl *)buf;
    def_impl = (KmppCfgDefImpl *)def;
    if (size != def_impl->buf_size) {
        kmpp_loge_f("mismatch size %d def size %d\n", size, def_impl->buf_size);
        return rk_nok;
    }
    impl->def = def;
    impl->trie =def_impl->trie;

    if (def_impl->preset)
        def_impl->preset(impl + 1);

    *cfg = impl;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_cfg_assign);

rk_s32 kmpp_cfg_put(KmppCfg cfg)
{
    if (cfg) {
        KmppCfgImpl *impl = (KmppCfgImpl *)cfg;

        if (impl->need_free)
            kmpp_free(impl);

        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_cfg_put);

void *kmpp_cfg_to_impl(KmppCfg cfg)
{
    KmppCfgImpl *impl = (KmppCfgImpl *)cfg;

    return impl ? (void *)(impl + 1) : NULL;
}
EXPORT_SYMBOL(kmpp_cfg_to_impl);

#define KMPP_CFG_ACCESS(type, base_type) \
    rk_s32 kmpp_cfg_set_##type(KmppCfg cfg, const rk_u8 *name, base_type val) \
    { \
        KmppCfgImpl *impl = (KmppCfgImpl *)cfg; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info && info->ctx) \
                ret = kmpp_cfg_impl_set_##type((KmppLocTbl *)info->ctx, (void *)(impl + 1), val); \
        } \
        if (ret) \
            kmpp_loge("cfg %s set %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return rk_nok; \
    } \
    EXPORT_SYMBOL(kmpp_cfg_set_##type); \
    rk_s32 kmpp_cfg_get_##type(KmppCfg cfg, const rk_u8 *name, base_type *val) \
    { \
        KmppCfgImpl *impl = (KmppCfgImpl *)cfg; \
        rk_s32 ret = rk_nok; \
        if (impl->trie) { \
            KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name); \
            if (info && info->ctx) \
                ret = kmpp_cfg_impl_get_##type((KmppLocTbl *)info->ctx, (void *)(impl + 1), val); \
        } \
        if (ret) \
            kmpp_loge("cfg %s get %s " #type " failed ret %d\n", \
                    impl ? impl->def ? impl->def->name : NULL : NULL, name, ret); \
        return rk_nok; \
    } \
    EXPORT_SYMBOL(kmpp_cfg_get_##type);

KMPP_CFG_ACCESS(s32, rk_s32)
KMPP_CFG_ACCESS(u32, rk_u32)
KMPP_CFG_ACCESS(s64, rk_s64)
KMPP_CFG_ACCESS(u64, rk_u64)
KMPP_CFG_ACCESS(ptr, void *)

rk_s32 kmpp_cfg_set_st(KmppCfg cfg, const rk_u8 *name, void *val)
{
    KmppCfgImpl *impl = (KmppCfgImpl *)cfg;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);
        if (info && info->ctx)
            ret = kmpp_cfg_impl_set_st((KmppLocTbl *)info->ctx, (void *)(impl + 1), val);
    }

    if (ret)
        kmpp_loge("cfg %s set %s st failed ret %d\n",
                impl ? impl->def ? impl->def->name : NULL : NULL, name, ret);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_cfg_set_st);

rk_s32 kmpp_cfg_get_st(KmppCfg cfg, const rk_u8 *name, void *val)
{
    KmppCfgImpl *impl = (KmppCfgImpl *)cfg;
    rk_s32 ret = rk_nok;

    if (impl->trie) {
        KmppTrieInfo *info = kmpp_trie_get_info(impl->trie, name);
        if (info && info->ctx)
            ret = kmpp_cfg_impl_get_st((KmppLocTbl *)info->ctx, (void *)(impl + 1), val);
    }

    if (ret)
        kmpp_loge("cfg %s get %s st failed ret %d\n",
                impl ? impl->def ? impl->def->name : NULL : NULL, name, ret);

    return rk_nok;
}
EXPORT_SYMBOL(kmpp_cfg_get_st);

rk_s32 kmpp_cfg_run(KmppCfg cfg, const rk_u8 *name)
{
    KmppCfgImpl *impl = (KmppCfgImpl *)cfg;
    rk_s32 ret = rk_nok;

    if (impl && impl->func)
        ret = impl->func((void *)(impl + 1));

    return ret;
}
EXPORT_SYMBOL(kmpp_cfg_run);