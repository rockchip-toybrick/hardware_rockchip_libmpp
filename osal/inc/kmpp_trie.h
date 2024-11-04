/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_TRIE_H__
#define __KMPP_TRIE_H__

#include "rk_type.h"

typedef void* KmppTrie;

/*
 * KmppTrie node buffer layout
 * +----------------+
 * |  KmppTrieImpl  |
 * +----------------+
 * |  KmppTrieNodes |
 * +----------------+
 * |  KmppTrieInfos |
 * +----------------+
 *
 * KmppTrieInfo element layout
 * +----------------+
 * |  KmppTrieInfo  |
 * +----------------+
 * |  User context  |
 * +----------------+
 * |  name string   |
 * +----------------+
 */
typedef struct KmppTrieInfo_t {
    const rk_u8 *name;
    void        *ctx;
    RK_S16      index;
    RK_S16      str_len;
} KmppTrieInfo;

rk_s32 kmpp_trie_init(KmppTrie *trie, rk_s32 info_size);
rk_s32 kmpp_trie_deinit(KmppTrie trie);

rk_s32 kmpp_trie_add_info(KmppTrie trie, const rk_u8 *name, void *ctx);

rk_s32 kmpp_trie_get_node_count(KmppTrie trie);
rk_s32 kmpp_trie_get_info_count(KmppTrie trie);
rk_s32 kmpp_trie_get_buf_size(KmppTrie trie);

/* trie lookup function */
KmppTrieInfo *kmpp_trie_get_info(KmppTrie trie, const rk_u8 *name);
KmppTrieInfo *kmpp_trie_get_info_first(KmppTrie trie);
KmppTrieInfo *kmpp_trie_get_info_next(KmppTrie trie, KmppTrieInfo *info);

void kmpp_trie_dump(KmppTrie trie, const rk_u8 *func);
#define kmpp_trie_dump_f(tire)  kmpp_trie_dump(tire, __FUNCTION__)

void kmpp_trie_timing_test(KmppTrie trie);

#endif /* __KMPP_TRIE_H__ */