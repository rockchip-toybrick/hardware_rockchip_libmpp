/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/spinlock.h>

#include "rk_list.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_atomic.h"

#define MEM_DBG_DEBUG               (0x00000001)
#define MEM_DBG_FLOW                (0x00000002)

#define mem_dbg(flag, fmt, ...)     kmpp_dbg(kmem_debug, flag, fmt, ## __VA_ARGS__)

#define mem_dbg_flow(fmt, ...)      mem_dbg(MEM_DBG_FLOW, fmt, ## __VA_ARGS__)

#define MEM_NODE_FUNC_LEN 32

typedef struct KmppMemNode_t {
    osal_list_head list;
    void *ptr;
    rk_u32 size;
    char func[MEM_NODE_FUNC_LEN];
} KmppMemNode;

static rk_s32 kmem_debug = 1;
static rk_s32 kmem_size;
static rk_s32 vmem_size;
static rk_s32 kmem_node_size = sizeof(KmppMemNode);
static osal_list_head kmem_list;
static spinlock_t kmem_lock;

static inline void *kmem_add(void *p, const char *func, rk_u32 size)
{
    KmppMemNode *node = (KmppMemNode *)p;

    node->ptr = p + kmem_node_size;
    node->size = size;

    snprintf(node->func, sizeof(node->func) - 1, "%s", func);

    spin_lock(&kmem_lock);
    osal_list_add_tail(&node->list, &kmem_list);
    KMPP_FETCH_ADD(&kmem_size, size);
    spin_unlock(&kmem_lock);

    mem_dbg_flow("kmem_add %px size %d at %s\n", p, size, func);

    return node->ptr;
}

static inline void *kmem_del(void *p, const char *func)
{
    KmppMemNode *node = (KmppMemNode *)((rk_u8 *)p - kmem_node_size);

    mem_dbg_flow("kmem_del %px size %d at %s from %s\n",
                 node, node->size, node->func, func);

    spin_lock(&kmem_lock);
    if (node->ptr != p)
        kmpp_loge("kmem_del mismatch: node %px input %px at %s\n",
                  node->ptr, p, func);

    osal_list_del_init(&node->list);
    KMPP_FETCH_SUB(&kmem_size, node->size);
    spin_unlock(&kmem_lock);

    return node;
}

void osal_mem_init(void)
{
    kmem_size = 0;
    vmem_size = 0;
    OSAL_INIT_LIST_HEAD(&kmem_list);
    spin_lock_init(&kmem_lock);
}
EXPORT_SYMBOL(osal_mem_init);

void osal_mem_deinit(void)
{
    KmppMemNode *node, *next;

    if (kmem_size)
        kmpp_loge("%d kmem leaked\n", kmem_size);

    if (vmem_size)
        kmpp_loge("%d vmem leaked\n", vmem_size);

    spin_lock(&kmem_lock);
    osal_list_for_each_entry_safe(node, next, &kmem_list, KmppMemNode, list) {
        kmpp_loge("%s: %px, size: %d\n", node->func, node->ptr, node->size);
        osal_list_del_init(&node->list);
        kfree(node);
    }
    spin_unlock(&kmem_lock);
}
EXPORT_SYMBOL(osal_mem_deinit);

void *osal_kmalloc_f(const char *func, rk_u32 size, rk_u32 gfp)
{
    void *p;

    if (!kmem_debug) {
        p = kvmalloc(size, gfp ? GFP_ATOMIC : GFP_KERNEL);
    } else {
        p = kvmalloc(size + kmem_node_size, gfp ? GFP_ATOMIC : GFP_KERNEL);
        if (!IS_ERR_OR_NULL(p)) {
            p = kmem_add(p, func, size);
        }
    }

    return p;
}
EXPORT_SYMBOL(osal_kmalloc_f);

void *osal_kcalloc_f(const char *func, rk_u32 size, rk_u32 gfp)
{
    void *p;

    if (!kmem_debug) {
        p = kvzalloc(size, gfp ? GFP_ATOMIC : GFP_KERNEL);
    } else {
        p = kvzalloc(size + kmem_node_size, gfp ? GFP_ATOMIC : GFP_KERNEL);
        if (!IS_ERR_OR_NULL(p))
            p = kmem_add(p, func, size);
    }

    return p;
}
EXPORT_SYMBOL(osal_kcalloc_f);

void *osal_krealloc_f(const char *func, void *ptr, rk_u32 oldsize, rk_u32 newsize, rk_u32 gfp)
{
    void *p;

    if (!kmem_debug) {
        p = kvrealloc(ptr, oldsize, newsize, gfp ? GFP_ATOMIC : GFP_KERNEL);
    } else {
        if (ptr)
            ptr = kmem_del(ptr, func);

        p = kvrealloc(ptr, oldsize + kmem_node_size, newsize + kmem_node_size,
                      gfp ? GFP_ATOMIC : GFP_KERNEL);
        if (!IS_ERR_OR_NULL(p))
            p = kmem_add(p, func, newsize);
    }

    return p;
}
EXPORT_SYMBOL(osal_krealloc_f);

void osal_kfree_f(const char *func, void *ptr)
{
    if (!IS_ERR_OR_NULL(ptr)) {
        if (kmem_debug)
            ptr = kmem_del(ptr, func);

        kvfree(ptr);
    } else {
        kmpp_loge("can not kfree %px from %s\n", ptr, func);
    }
}
EXPORT_SYMBOL(osal_kfree_f);

void *osal_malloc_share_f(const char *func, rk_u32 size)
{
    size = PAGE_ALIGN(size);
    return vmalloc_user(size);
}
EXPORT_SYMBOL(osal_malloc_share_f);

void osal_kfree_share_f(const char *func, void *ptr)
{
    if (!IS_ERR_OR_NULL(ptr)) {
        vfree(ptr);
    } else {
        kmpp_loge("can not kfree %px from %s\n", ptr, func);
    }
}
EXPORT_SYMBOL(osal_kfree_share_f);
