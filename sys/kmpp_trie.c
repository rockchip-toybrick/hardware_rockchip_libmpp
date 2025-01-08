/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_trie"

#include <linux/kernel.h>
#include <linux/math64.h>

#include "kmpp_osal.h"
#include "kmpp_trie.h"

#define KMPP_TRIE_DBG_FUNC              (0x00000001)
#define KMPP_TRIE_DBG_SET               (0x00000002)
#define KMPP_TRIE_DBG_GET               (0x00000004)
#define KMPP_TRIE_DBG_CNT               (0x00000008)
#define KMPP_TRIE_DBG_WALK              (0x00000010)
#define KMPP_TRIE_DBG_LAST              (0x00000020)
#define KMPP_TRIE_DBG_LAST_STEP         (0x00000040)
#define KMPP_TRIE_DBG_LAST_CHECK        (0x00000080)
#define KMPP_TRIE_DBG_IMPORT            (0x00000100)

#define trie_dbg(flag, fmt, ...)        kmpp_dbg_f(kmpp_trie_debug, flag, fmt, ## __VA_ARGS__)
#define trie_dbg_func(fmt, ...)         trie_dbg(KMPP_TRIE_DBG_FUNC, fmt, ## __VA_ARGS__)
#define trie_dbg_set(fmt, ...)          trie_dbg(KMPP_TRIE_DBG_SET, fmt, ## __VA_ARGS__)
#define trie_dbg_get(fmt, ...)          trie_dbg(KMPP_TRIE_DBG_GET, fmt, ## __VA_ARGS__)
#define trie_dbg_cnt(fmt, ...)          trie_dbg(KMPP_TRIE_DBG_CNT, fmt, ## __VA_ARGS__)
#define trie_dbg_walk(fmt, ...)         trie_dbg(KMPP_TRIE_DBG_WALK, fmt, ## __VA_ARGS__)
#define trie_dbg_last(fmt, ...)         trie_dbg(KMPP_TRIE_DBG_LAST, fmt, ## __VA_ARGS__)

#define KMPP_TRIE_KEY_LEN               (4)
#define KMPP_TRIE_KEY_MAX               (1 << (KMPP_TRIE_KEY_LEN))
#define KMPP_TRIE_INFO_MAX              (1 << 12)
#define KMPP_TRIE_NAME_MAX              (1 << 12)

#define DEFAULT_NODE_COUNT              900
#define DEFAULT_INFO_COUNT              80
#define INVALID_NODE_ID                 (-1)
#define KMPP_TRIE_TAG_LEN_MAX           ((sizeof(rk_u64) * 8) / KMPP_TRIE_KEY_LEN)

/* spatial optimized trie tree */
typedef struct KmppTrieNode_t {
    /* next         - next trie node index */
    rk_s16          next[KMPP_TRIE_KEY_MAX];
    /* id           - payload data offset of current trie node */
    rk_s32          id;
    /* idx          - trie node index in ascending order */
    rk_s16          idx;
    /* prev         - previous trie node index */
    rk_s16          prev;

    /* tag_val      - prefix tag */
    rk_u64          tag_val;
    /* key          - current key value in previous node as next */
    rk_u16          key;
    /*
     * tag len      - prefix tag length
     * zero         - normal node with 16 next node
     * positive     - tag node with 64bit prefix tag
     */
    rk_s16          tag_len;

    /* next_cnt     - valid next node count */
    rk_u16          next_cnt;
} KmppTrieNode;

typedef struct KmppTrieInfoInt_t {
    rk_s32          index;
    rk_s32          ctx_len;
    rk_s32          str_len;
    rk_s32          ctx_offset;
    rk_s32          name_offset;
} KmppTrieInfoInt;

typedef struct KmppTrieImpl_t {
    rk_u8           *name;
    rk_s32          name_len;
    rk_s32          buf_size;

    rk_s32          infos_size;

    rk_s32          info_count;
    rk_s32          info_used;
    KmppTrieInfoInt *info;
    rk_s32          node_count;
    rk_s32          node_used;
    KmppTrieNode     *nodes;

    /* info and name record buffer */
    void            *info_buf;
    void            *name_buf;
    rk_s32          info_buf_size;
    rk_s32          info_buf_pos;
    rk_s32          name_buf_size;
    rk_s32          name_buf_pos;
} KmppTrieImpl;

rk_u32 kmpp_trie_debug = 0;

static rk_s32 trie_get_node(KmppTrieImpl *trie, rk_s32 prev, rk_u64 key)
{
    KmppTrieNode *node;
    rk_s32 idx;

    if (trie->node_used >= trie->node_count) {
        rk_s32 old_count = trie->node_count;
        rk_s32 new_count = old_count * 2;
        KmppTrieNode *new_nodes = kmpp_realloc(trie->nodes, sizeof(KmppTrieNode) * old_count,
                                               sizeof(KmppTrieNode) * new_count);

        if (!new_nodes) {
            kmpp_loge_f("failed to realloc new nodes %d\n", new_count);
            return -1;
        }

        /* NOTE: new memory should be osal_memset to zero */
        osal_memset(new_nodes + old_count, 0, sizeof(*new_nodes) * old_count);

        trie_dbg_cnt("trie %p enlarge node %p:%d -> %p:%d\n",
                     trie, trie->nodes, trie->node_count, new_nodes, new_count);

        trie->nodes = new_nodes;
        trie->node_count = new_count;
    }

    idx = trie->node_used++;
    node = &trie->nodes[idx];
    node->idx = idx;
    node->prev = (prev > 0) ? prev : 0;
    node->key = key;
    node->id = INVALID_NODE_ID;

    if (prev >= 0)
        trie->nodes[prev].next_cnt++;

    trie_dbg_cnt("get node %d\n", idx);

    return idx;
}

rk_s32 kmpp_trie_init(KmppTrie *trie, const rk_u8 *name)
{
    KmppTrieImpl *p;
    rk_s32 ret = rk_nok;

    if (!trie) {
        kmpp_loge_f("invalid NULL input trie automation\n");
        return ret;
    }

    if (name) {
        rk_s32 name_len = osal_strnlen(name, KMPP_TRIE_NAME_MAX) + 1;

        p = kmpp_calloc(sizeof(KmppTrieImpl) + name_len);
        if (!p) {
            kmpp_loge_f("create trie impl %s failed\n", name);
            goto done;
        }

        p->name = (rk_u8 *)(p + 1);
        p->name_len = name_len;
        osal_strncpy(p->name, name, name_len);

        p->node_count = DEFAULT_NODE_COUNT;
        p->nodes = kmpp_calloc(sizeof(KmppTrieNode) * p->node_count);
        if (!p->nodes) {
            kmpp_loge_f("create %d nodes failed\n", p->node_count);
            goto done;
        }

        p->info_count = DEFAULT_INFO_COUNT;
        p->info = kmpp_calloc(sizeof(KmppTrieInfoInt) * p->info_count);
        if (!p->info) {
            kmpp_loge_f("failed to alloc %d info\n", p->info_count);
            goto done;
        }

        p->info_buf_size = 4096;
        p->info_buf = kmpp_calloc(p->info_buf_size);
        if (!p->info_buf) {
            kmpp_loge_f("failed to alloc %d info buffer\n", p->info_buf_size);
            goto done;
        }

        p->name_buf_size = 4096;
        p->name_buf = kmpp_calloc(p->name_buf_size);
        if (!p->name_buf) {
            kmpp_loge_f("failed to alloc %d name buffer\n", p->name_buf_size);
            goto done;
        }
    } else {
        p = kmpp_calloc(sizeof(KmppTrieImpl));
        if (!p) {
            kmpp_loge_f("create trie impl failed\n");
            goto done;
        }
    }

    /* get node 0 as root node*/
    trie_get_node(p, -1, 0);
    ret = rk_ok;

done:
    if (ret && p) {
        kmpp_free(p->info);
        kmpp_free(p->info_buf);
        kmpp_free(p->name_buf);
        kmpp_free(p->nodes);
        kmpp_free(p);
    }

    *trie = p;
    return ret;
}

rk_s32 kmpp_trie_deinit(KmppTrie trie)
{
    if (trie) {
        KmppTrieImpl *p = (KmppTrieImpl *)trie;

        /* NOTE: do not remvoe imported root */
        if (p->node_count)
            kmpp_free(p->nodes);
        else
            p->nodes = NULL;

        kmpp_free(p->info);
        kmpp_free(p->info_buf);
        kmpp_free(p->name_buf);
        kmpp_free(p);
        return rk_ok;
    }

    return rk_nok;
}

static rk_s32 kmpp_trie_walk(KmppTrieNode *node, rk_u64 *tag_val, rk_s32 *tag_len, rk_u32 key)
{
    rk_u64 val = *tag_val;
    rk_s32 len = *tag_len;

    if (node->tag_len > len) {
        *tag_val = (val << 4) | key;
        *tag_len = len + 1;

        trie_dbg_walk("node %d:%d tag len %d - %d val %016llx - %016llx -> key %x -> tag fill\n",
                      node->idx, node->id, node->tag_len, *tag_len, node->tag_val, *tag_val, key);

        return node->idx;
    }

    /* normal next switch node */
    if (!node->tag_len) {
        trie_dbg_walk("node %d:%d -> key %x -> next %d\n",
                      node->idx, node->id, key, node->next[key]);

        return node->next[key];
    }

    *tag_val = 0;
    *tag_len = 0;

    if (node->tag_val != val) {
        trie_dbg_walk("node %d:%d tag len %d - %d val %016llx - %016llx -> tag mismatch\n",
                      node->idx, node->id, node->tag_len, len, node->tag_val, val);
        return INVALID_NODE_ID;
    }

    trie_dbg_walk("node %d:%d tag len %d - %d val %016llx - %016llx -> tag match -> key %d next %d\n",
                  node->idx, node->id, node->tag_len, len, node->tag_val, val, key, node->next[key]);

    return node->next[key];
}

static KmppTrieNode *kmpp_trie_get_node(KmppTrieNode *root, const rk_u8 *name)
{
    KmppTrieNode *ret = NULL;
    const rk_u8 *s = name;
    rk_u64 tag_val = 0;
    rk_s32 tag_len = 0;
    rk_s32 idx = 0;

    if (!root || !name) {
        kmpp_loge_f("invalid root %p name %p\n", root, name);
        return NULL;
    }

    trie_dbg_get("root %p search %s start\n", root, name);

    do {
        rk_u8 key = *s++;
        rk_u32 key0 = (key >> 4) & 0xf;
        rk_u32 key1 = key & 0xf;
        rk_s32 end = (s[0] == '\0');

        idx = kmpp_trie_walk(&root[idx], &tag_val, &tag_len, key0);
        if (idx < 0)
            break;

        idx = kmpp_trie_walk(&root[idx], &tag_val, &tag_len, key1);
        if (idx < 0 || end)
            break;
    } while (1);

    ret = (idx >= 0) ? &root[idx] : NULL;

    trie_dbg_get("get node %d:%d\n", idx, ret ? ret->id : INVALID_NODE_ID);

    return ret;
}

static rk_s32 kmpp_trie_check(KmppTrie trie, const rk_u8 *log)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;
    rk_u8 *buf = (rk_u8 *)p->name_buf;
    rk_s32 i;

    for (i = 0; i < p->info_used; i++) {
        const rk_u8 *name = buf + p->info[i].name_offset;
        KmppTrieNode *node = kmpp_trie_get_node(p->nodes, name);

        if (node && node->id >= 0 && node->id == i)
            continue;

        kmpp_loge("trie check on %s found mismatch info %s %d - %d\n",
                  log, name, i, node ? node->id : -1);
        return rk_nok;
    }

    return rk_ok;
}

rk_s32 kmpp_trie_last_info(KmppTrie trie)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;
    KmppTrieNode *root;
    KmppTrieNode *node;
    rk_u8 *buf;
    rk_s32 node_count;
    rk_s32 node_valid;
    rk_s32 nodes_size;
    rk_s32 len = 0;
    rk_s32 pos = 0;
    rk_s32 i;
    rk_s32 j;

    if (!trie) {
        kmpp_loge_f("invalid NULL trie\n");
        return rk_nok;
    }

    /* write trie self entry info */
    pos = p->info_used + 3;
    kmpp_trie_add_info(trie, "__name__", p->name, p->name_len);
    kmpp_trie_add_info(trie, "__info__", &pos, sizeof(pos));
    /* NOTE: node count need to be update after shrinking */
    kmpp_trie_add_info(trie, "__node__", &p->node_used, sizeof(p->node_used));

    root = p->nodes;
    node_count = p->node_used;
    node_valid = node_count;

    trie_dbg_last("shrink trie node start node %d info %d\n", node_count, p->info_used);

    if (kmpp_trie_debug & KMPP_TRIE_DBG_LAST_STEP)
        kmpp_trie_dump_f(trie);

    for (i = node_count - 1; i > 0; i--) {
        KmppTrieNode *prev;
        rk_s32 prev_idx;

        node = &root[i];
        prev_idx = node->prev;
        prev = &root[prev_idx];

        if (prev->next_cnt > 1) {
            trie_dbg_last("node %d:%d prev %d next count %d stop shrinking for multi next\n",
                          i, node->id, prev_idx, prev->next_cnt);
            continue;
        }

        if (node->tag_len >= (rk_s16)KMPP_TRIE_TAG_LEN_MAX) {
            trie_dbg_last("node %d:%d tag %d - %016llx stop shrinking for max tag len\n",
                          i, node->id, node->tag_len, node->tag_val);
            continue;
        }

        if (prev->id >= 0) {
            trie_dbg_last("node %d:%d tag %d - %016llx stop shrinking for valid info node\n",
                          i, node->id, node->tag_len, node->tag_val);
            continue;
        }

        prev->id = node->id;
        /* NOTE: do NOT increase tag length on root node */
        prev->tag_len = node->tag_len + 1;
        prev->tag_val = ((rk_u64)node->key << (node->tag_len * 4)) | node->tag_val;
        prev->next_cnt = node->next_cnt;
        osal_memcpy(prev->next, node->next, sizeof(node->next));

        trie_dbg_last("node %d:%d shrink prev %d key %x tag %016llx -> %016llx\n",
                      i, node->id, prev->idx, prev->key, node->tag_val, prev->tag_val);

        for (j = 0; j < KMPP_TRIE_KEY_MAX; j++) {
            if (!prev->next[j])
                continue;

            root[prev->next[j]].prev = prev_idx;
        }

        osal_memset(node, 0, sizeof(*node));
        node->id = INVALID_NODE_ID;
        node_valid--;
    }

    trie_dbg_last("shrink trie node finish count %d -> %d\n", node_count, node_valid);

    if (kmpp_trie_debug & KMPP_TRIE_DBG_LAST_STEP)
        kmpp_trie_dump_f(trie);

    if (kmpp_trie_debug & KMPP_TRIE_DBG_LAST_CHECK)
        kmpp_trie_check(trie, "shrink merge tag stage");

    trie_dbg_last("move trie node start to reduce memory %d -> %d\n", node_count, node_valid);

    for (i = 1; i < node_valid; i++) {
        node = &root[i];

        /* skip valid node */
        if (node->idx)
            continue;

        for (j = i; j < node_count; j++) {
            KmppTrieNode *tmp = &root[j];
            KmppTrieNode *prev;
            rk_s32 k;

            /* skip empty node */
            if (!tmp->idx)
                continue;

            trie_dbg_last("move node %d to %d prev %d\n", j, i, tmp->prev);

            prev = &root[tmp->prev];

            /* relink previous node */
            for (k = 0; k < KMPP_TRIE_KEY_MAX; k++) {
                if (prev->next[k] != tmp->idx)
                    continue;

                prev->next[k] = i;
                break;
            }

            osal_memcpy(node, tmp, sizeof(*node));
            node->idx = i;
            osal_memset(tmp, 0, sizeof(*tmp));

            /* relink next node */
            for (k = 0; k < KMPP_TRIE_KEY_MAX; k++) {
                if (!node->next[k])
                    continue;

                root[node->next[k]].prev = i;
            }

            break;
        }
    }

    p->node_used = node_valid;

    trie_dbg_last("move trie node finish used %d\n", p->node_used);

    if (kmpp_trie_debug & KMPP_TRIE_DBG_LAST_STEP)
        kmpp_trie_dump_f(trie);

    if (kmpp_trie_debug & KMPP_TRIE_DBG_LAST_CHECK)
        kmpp_trie_check(trie, "shrink move node stage");

    trie_dbg_last("create user buffer start\n");

    nodes_size = sizeof(KmppTrieNode) * p->node_used;
    p->buf_size = nodes_size + sizeof(KmppTrieInfo) * p->info_used + p->info_buf_pos + p->name_buf_pos;

    buf = kmpp_calloc(p->buf_size);
    if (!buf) {
        kmpp_loge("failed to alloc trie buffer size %d\n", len);
        return rk_nok;
    }

    p->nodes = (KmppTrieNode *)buf;
    osal_memcpy(p->nodes, root, nodes_size);
    pos = nodes_size;

    for (i = 0; i < p->info_used; i++) {
        KmppTrieInfoInt *src = &p->info[i];
        KmppTrieInfo *dst;
        const rk_u8 *name = (rk_u8 *)p->name_buf + src->name_offset;

        node = kmpp_trie_get_node(p->nodes, name);
        node->id = pos;

        /* reserve node info */
        dst = (KmppTrieInfo *)(buf + pos);
        dst->index = src->index;
        dst->ctx_len = src->ctx_len;
        dst->str_len = src->str_len;
        pos += sizeof(KmppTrieInfo);

        /* copy info name */
        osal_strncpy(buf + pos, name, dst->str_len);
        pos += dst->str_len;

        /* reserve user context space */
        osal_memcpy(buf + pos, (rk_u8 *)p->info_buf + src->ctx_offset, dst->ctx_len);
        pos += dst->ctx_len;
    }

    kmpp_free(root);
    kmpp_free(p->info);
    kmpp_free(p->info_buf);
    kmpp_free(p->name_buf);

    /* NOTE: udpate final shrinked node count */
    {
        KmppTrieInfo *info = kmpp_trie_get_info_from_root(p->nodes, "__node__");

        if (info)
            *(rk_u32 *)kmpp_trie_info_ctx(info) = p->node_used;
    }

    return rk_ok;
}

rk_s32 kmpp_trie_add_info(KmppTrie trie, const rk_u8 *name, void *ctx, rk_u32 ctx_len)
{
    KmppTrieImpl *p;
    KmppTrieInfoInt *info;
    KmppTrieNode *node;
    const rk_u8 *s;
    rk_s32 info_buf_pos;
    rk_s32 name_buf_pos;
    rk_s32 info_used;
    rk_s32 next;
    rk_s32 len;
    rk_s32 idx;
    rk_s32 i;

    if (!trie) {
        kmpp_loge_f("invalid trie %p name %s ctx %p\n", trie, name, ctx);
        return rk_nok;
    }

    if (!name)
        return kmpp_trie_last_info(trie);

    p = (KmppTrieImpl *)trie;

    info_buf_pos = p->info_buf_pos;
    name_buf_pos = p->name_buf_pos;
    info_used = p->info_used;
    len = osal_strnlen(name, KMPP_TRIE_NAME_MAX);

    if (len >= KMPP_TRIE_NAME_MAX) {
        kmpp_loge_f("invalid trie name %s len %d larger than max %d\n",
                    name, len, KMPP_TRIE_NAME_MAX);
        return rk_nok;
    }
    if (info_used >= KMPP_TRIE_INFO_MAX) {
        kmpp_loge_f("invalid trie info count %d larger than max %d\n",
                    len, KMPP_TRIE_INFO_MAX);
        return rk_nok;
    }

    /* check and enlarge info record buffer */
    if (info_used >= p->info_count) {
        rk_s32 old_count = p->info_count;
        rk_s32 new_count = old_count * 2;
        void *ptr = kmpp_realloc(p->info, sizeof(KmppTrieInfoInt) * old_count,
                                 sizeof(KmppTrieInfoInt) * new_count);

        if (!ptr) {
            kmpp_loge_f("failed to realloc new info %d\n", new_count);
            return rk_nok;
        }

        trie_dbg_cnt("trie %p enlarge info %p:%d -> %p:%d\n",
                     trie, p->info, old_count, ptr, new_count);

        p->info = (KmppTrieInfoInt *)ptr;
        p->info_count = new_count;
    }

    /* check and enlarge contex buffer */
    if (info_buf_pos + ctx_len > p->info_buf_size) {
        rk_s32 old_size = p->info_buf_size;
        rk_s32 new_size = old_size * 2;
        void *ptr = kmpp_realloc(p->info_buf, old_size, new_size);

        if (!ptr) {
            kmpp_loge_f("failed to realloc new info buffer %d\n", new_size);
            return rk_nok;
        }

        trie_dbg_cnt("trie %p enlarge info_buf %p:%d -> %p:%d\n",
                     trie, p->info_buf, old_size, ptr, new_size);

        p->info_buf = ptr;
        p->info_buf_size = new_size;
    }

    /* check and enlarge name string buffer */
    if (name_buf_pos + len + 1 >= p->name_buf_size) {
        rk_s32 old_size = p->name_buf_size;
        rk_s32 new_size = old_size * 2;
        void *ptr = kmpp_realloc(p->name_buf, old_size, new_size);

        if (!ptr) {
            kmpp_loge_f("failed to realloc new name buffer %d\n", new_size);
            return rk_nok;
        }

        trie_dbg_cnt("trie %p enlarge name %p:%d -> %p:%d\n",
                     trie, p->name_buf, old_size, ptr, new_size);

        p->name_buf = ptr;
        p->name_buf_size = new_size;
    }

    node = NULL;
    s = name;
    next = 0;
    idx = 0;

    trie_dbg_set("trie %p add info %s len %d\n", trie, s, len);

    for (i = 0; i < len && s[i]; i++) {
        rk_u32 key = s[i];
        rk_s32 key0 = (key >> 4) & 0xf;
        rk_s32 key1 = (key >> 0) & 0xf;

        node = p->nodes + idx;
        next = node->next[key0];

        trie_dbg_set("trie %p add %s at %2d rk_u8 %c:%3d:%x:%x node %d -> %d\n",
                     trie, s, i, key, key, key0, key1, idx, next);

        if (!next) {
            next = trie_get_node(p, idx, key0);
            /* realloc may cause memory address change */
            node = p->nodes + idx;
            node->next[key0] = next;

            trie_dbg_set("trie %p add %s at %2d rk_u8 %c:%3d node %d -> %d as new key0\n",
                         trie, s, i, key, key, node->idx, next);
        }

        idx = next;
        node = p->nodes + idx;
        next = node->next[key1];

        trie_dbg_set("trie %p add %s at %2d rk_u8 %c:%3d:%x:%x node %d -> %d as key0\n",
                     trie, s, i, key, key, key0, key1, idx, next);

        if (!next) {
            next = trie_get_node(p, idx, key1);
            /* realloc may cause memory address change */
            node = p->nodes + idx;
            node->next[key1] = next;

            trie_dbg_set("trie %p add %s at %2d rk_u8 %c:%3d node %d -> %d as new child\n",
                         trie, s, i, key, key, node->idx, next);
        }

        idx = next;

        trie_dbg_set("trie %p add %s at %2d rk_u8 %c:%3d:%x:%x node %d -> %d as key1\n",
                     trie, s, i, key, key, key0, key1, idx, next);
    }

    if (p->nodes[idx].id != -1) {
        kmpp_loge_f("trie %p add info %s already exist\n", trie, name);
        return rk_nok;
    }

    p->nodes[idx].id = info_used;
    p->info_used++;

    info = &p->info[info_used];
    info->index = info_used;
    info->ctx_len = ctx_len;
    info->str_len = KMPP_ALIGN(len + 1, sizeof(rk_u32));
    info->ctx_offset = info_buf_pos;
    info->name_offset = name_buf_pos;

    osal_memcpy((rk_u8 *)p->info_buf + info_buf_pos, ctx, ctx_len);
    p->info_buf_pos += info->ctx_len;

    osal_snprintf((rk_u8 *)p->name_buf + name_buf_pos,
                   p->name_buf_size - p->name_buf_pos - 1, "%s", name);
    p->name_buf_pos += info->str_len;

    trie_dbg_set("trie %p add %d info %s at node %d pos %d ctx %p size %d done\n",
                 trie, i, s, idx, info_used, ctx, ctx_len);

    return rk_ok;
}

rk_s32 kmpp_trie_import(KmppTrie trie, void *root)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;
    KmppTrieInfo *info;
    rk_s32 i;

    if (!p || !root) {
        kmpp_loge_f("invalid trie %p root %p\n", trie, root);
        return rk_nok;
    }

    /* free all old buffer */
    kmpp_free(p->nodes);
    kmpp_free(p->info);
    kmpp_free(p->info_buf);
    kmpp_free(p->name_buf);

    info = kmpp_trie_get_info_from_root(root, "__name__");
    if (info)
        p->name = kmpp_trie_info_ctx(info);

    info = kmpp_trie_get_info_from_root(root, "__node__");
    if (info)
        p->node_used = *(rk_u32 *)kmpp_trie_info_ctx(info);

    info = kmpp_trie_get_info_from_root(root, "__info__");
    if (info)
        p->info_used = *(rk_u32 *)kmpp_trie_info_ctx(info);

    /* import and update new buffer */
    p->nodes = (KmppTrieNode *)root;

    /* set count to zero to avoid free on deinit */
    p->node_count = 0;
    p->info_count = 0;
    p->info_buf_size = 0;
    p->info_buf_pos = 0;
    p->name_buf_size = 0;
    p->name_buf_pos = 0;

    if (kmpp_trie_debug & KMPP_TRIE_DBG_IMPORT)
        kmpp_trie_dump(trie, "root import");

    info = kmpp_trie_get_info_first(trie);

    for (i = 0; i < p->info_used; i++) {
        const char *name = kmpp_trie_info_name(info);
        KmppTrieInfo *info_set = info;
        KmppTrieInfo *info_ret = kmpp_trie_get_info(p, name);

        info = kmpp_trie_get_info_next(trie, info);

        if (info_ret && info_set == info_ret && info_ret->index == i)
            continue;

        kmpp_loge_f("trie check on import found mismatch info %s [%d:%p] - [%d:%p]\n",
                    name, i, info_set, info_ret ? info_ret->index : -1, info_ret);
        return rk_nok;
    }

    return rk_ok;
}

rk_s32 kmpp_trie_get_node_count(KmppTrie trie)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;

    return (p) ? p->node_used : 0;
}

rk_s32 kmpp_trie_get_info_count(KmppTrie trie)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;

    return (p) ? p->info_used : 0;
}

rk_s32 kmpp_trie_get_buf_size(KmppTrie trie)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;

    return (p) ? p->buf_size : 0;
}

void *kmpp_trie_get_node_root(KmppTrie trie)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;

    return (p) ? p->nodes : NULL;
}

KmppTrieInfo *kmpp_trie_get_info(KmppTrie trie, const rk_u8 *name)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;
    KmppTrieNode *node;

    if (!trie || !name) {
        kmpp_loge_f("invalid trie %p name %p\n", trie, name);
        return NULL;
    }

    node = kmpp_trie_get_node(p->nodes, name);
    if (!node || node->id < 0)
        return NULL;

    return (KmppTrieInfo *)(((rk_u8 *)p->nodes) + node->id);
}

KmppTrieInfo *kmpp_trie_get_info_first(KmppTrie trie)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;

    return (p) ? (KmppTrieInfo *)(((rk_u8 *)p->nodes) + p->node_used * sizeof(KmppTrieNode)) : NULL;
}

KmppTrieInfo *kmpp_trie_get_info_next(KmppTrie trie, KmppTrieInfo *info)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;

    if (!p || !info || info->index >= p->info_used - 1)
        return NULL;

    return (KmppTrieInfo *)((rk_u8 *)(info + 1) + info->str_len + info->ctx_len);
}

KmppTrieInfo *kmpp_trie_get_info_from_root(void *root, const rk_u8 *name)
{
    KmppTrieNode *node;

    if (!root || !name) {
        kmpp_loge_f("invalid root %p name %p\n", root, name);
        return NULL;
    }

    node = kmpp_trie_get_node(root, name);
    if (!node || node->id < 0)
        return NULL;

    return (KmppTrieInfo *)(((rk_u8 *)root) + node->id);
}

void kmpp_trie_dump(KmppTrie trie, const rk_u8 *func)
{
    KmppTrieImpl *p = (KmppTrieImpl *)trie;
    rk_s32 i;
    rk_s32 next_cnt[17];
    rk_s32 tag_len[17];

    osal_memset(next_cnt, 0, sizeof(next_cnt));
    osal_memset(tag_len, 0, sizeof(tag_len));

    kmpp_logi("%s dumping trie %p\n", func, trie);
    kmpp_logi("name %s size %d node %d info %d\n",
              p->name, p->buf_size, p->node_used, p->info_used);

    for (i = 0; i < p->node_used; i++) {
        KmppTrieNode *node = &p->nodes[i];
        rk_s32 valid_count = 0;
        rk_s32 j;

        if (i && !node->idx)
            continue;

        if (node->id >= 0) {
            /* check before and after last info */
            if (node->id < p->node_used * sizeof(KmppTrieNode))
                kmpp_logi("node %d key %x info %d - %s\n", node->idx, node->key, node->id,
                          p->name_buf ? (rk_u8 *)p->name_buf + p->info[node->id].name_offset :
                          kmpp_trie_info_name((KmppTrieInfo *)((rk_u8 *)p->nodes + node->id)));
            else
                kmpp_logi("node %d key %x info %d - %s\n", node->idx, node->key, node->id,
                          kmpp_trie_info_name((KmppTrieInfo *)((rk_u8 *)p->nodes + node->id)));
        } else
            kmpp_logi("node %d key %x\n", node->idx, node->key);

        if (node->tag_len)
            kmpp_logi("    prev %d count %d tag %d - %llx\n", node->prev, node->next_cnt, node->tag_len, node->tag_val);
        else
            kmpp_logi("    prev %d count %d\n", node->prev, node->next_cnt);

        for (j = 0; j < KMPP_TRIE_KEY_MAX; j++) {
            if (node->next[j] > 0) {
                kmpp_logi("    next %d:%d -> %d\n", valid_count, j, node->next[j]);
                valid_count++;
            }
        }

        next_cnt[valid_count]++;
        tag_len[node->tag_len]++;
    }

    kmpp_logi("node | next |  tag | used %d\n", p->node_used);

    for (i = 0; i < 17; i++) {
        if (next_cnt[i] || tag_len[i])
            kmpp_logi("%2d   | %4d | %4d |\n", i, next_cnt[i], tag_len[i]);
    }

    kmpp_logi("%s dumping node end\n", func, p->node_used);
}

void kmpp_trie_timing_test(KmppTrie trie)
{
    KmppTrieInfo *root = kmpp_trie_get_info_first(trie);
    KmppTrieInfo *info = root;
    rk_s64 sum = 0;
    rk_s32 loop = 0;

    do {
        rk_s64 start, end;

        start = kmpp_time();
        kmpp_trie_get_info(trie, kmpp_trie_info_name(info));
        end = kmpp_time();

        info = kmpp_trie_get_info_next(trie, info);
        sum += end - start;
        loop++;
    } while (info);

    kmpp_logi("trie access %d info %lld us averga %lld us\n",
              loop, sum, div64_s64(sum, loop));
}

EXPORT_SYMBOL(kmpp_trie_init);
EXPORT_SYMBOL(kmpp_trie_deinit);
EXPORT_SYMBOL(kmpp_trie_add_info);
EXPORT_SYMBOL(kmpp_trie_import);
EXPORT_SYMBOL(kmpp_trie_get_node_count);
EXPORT_SYMBOL(kmpp_trie_get_info_count);
EXPORT_SYMBOL(kmpp_trie_get_buf_size);
EXPORT_SYMBOL(kmpp_trie_get_node_root);
EXPORT_SYMBOL(kmpp_trie_get_info);
EXPORT_SYMBOL(kmpp_trie_get_info_first);
EXPORT_SYMBOL(kmpp_trie_get_info_next);
EXPORT_SYMBOL(kmpp_trie_get_info_from_root);
EXPORT_SYMBOL(kmpp_trie_dump);
EXPORT_SYMBOL(kmpp_trie_timing_test);