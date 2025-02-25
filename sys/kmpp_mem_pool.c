/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_mem_pool"

#include "kmpp_env.h"
#include "kmpp_log.h"
#include "kmpp_mem.h"
#include "kmpp_macro.h"
#include "kmpp_atomic.h"
#include "kmpp_string.h"
#include "kmpp_spinlock.h"

#include "kmpp_sys.h"
#include "kmpp_mem_pool_impl.h"

#define MEM_POOL_DEG_FLOW               (0x00000001)

#define mem_pool_dbg(flag, fmt, ...)    kmpp_dbg(kmpp_mem_pool_debug, flag, fmt, ## __VA_ARGS__)

#define mem_pool_dbg_flow(fmt, ...)     mem_pool_dbg(MEM_POOL_DEG_FLOW, fmt, ## __VA_ARGS__)

static rk_u32 kmpp_mem_pool_debug = 0;

typedef struct KmppMemPoolSrv_t {
    /* list for list_srv in KmppMemPoolImpl */
    osal_list_head      list_pool;
    osal_spinlock       *lock;
    KmppEnvNode         env;
    rk_s32              pool_id;
    rk_s32              pool_cnt;
} KmppMemPoolSrv;

static KmppMemPoolSrv *srv_mempool = NULL;

#define get_pool_srv(caller) \
    ({ \
        KmppMemPoolSrv *__tmp; \
        if (srv_mempool) { \
            __tmp = srv_mempool; \
        } else { \
            kmpp_err_f("mem pool srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

static const rk_u8 space_12[13] = "            ";

static void kmpp_mem_pool_show(KmppEnvNode env, void *data)
{
    KmppMemPoolSrv *srv = get_pool_srv(__FUNCTION__);
    KmppMemPoolImpl *pool, *m;

    kmpp_env_log(env, "\n-------------------------------- total %d mem pool --------------------------------\n", srv->pool_cnt);

    if (!srv->pool_cnt)
        return;


    osal_spin_lock(srv->lock);

    kmpp_env_log(env, "|%-4s|%-16s|%-12s|%-12s|%-12s|%-12s|%-12s|\n",
                 "id", "name", "size", "unused", "used", "total", "max");

    osal_list_for_each_entry_safe(pool, m, &srv->list_pool, KmppMemPoolImpl, list_srv) {
        KmppMemPoolNode *node, *n;

        kmpp_env_log(env, "|%-4d|%-16s|%-12d|%-12d|%-12d|%-12d|%-12d|\n",
                     pool->pool_id, pool->name, pool->size, pool->max_cnt, pool->used_count,
                     pool->max_cnt + pool->used_count, pool->unused_count);

        if (pool->used_count) {
            osal_list_for_each_entry_safe(node, n, &pool->used, KmppMemPoolNode, list) {
                kmpp_env_log(env, "|%4d|%-16px|%-12d|%-12s|%-12s|%-12s|%-12s|\n",
                             node->id, node->ptr, node->size, space_12, "used", space_12, space_12);
            }
        }

        if (pool->unused_count) {
            osal_list_for_each_entry_safe(node, n, &pool->unused, KmppMemPoolNode, list) {
                kmpp_env_log(env, "|%4d|%-16px|%-12d|%-12s|%-12s|%-12s|%-12s|\n",
                             node->id, node->ptr, node->size, "unused", space_12, space_12, space_12);
            }
        }
    }

    osal_spin_unlock(srv->lock);
}

void kmpp_mem_pool_init(void)
{
    KmppMemPoolSrv *srv;
    rk_s32 lock_size = osal_spinlock_size();
    rk_s32 size;

    if (srv_mempool)
        return;

    size = sizeof(KmppMemPoolSrv) + lock_size;
    srv = kmpp_calloc(size);
    if (!srv) {
        kmpp_err_f("failed to alloc mem pool srv size %d\n", size);
        return;
    }

    OSAL_INIT_LIST_HEAD(&srv->list_pool);
    osal_spinlock_assign(&srv->lock, (osal_spinlock *)(srv + 1), lock_size);

    {
        KmppEnvInfo info;

        info.name = "kmpp_mem_pool";
        info.readonly = 1;
        info.type = KmppEnv_user;
        info.val = NULL;
        info.env_show = kmpp_mem_pool_show;

        kmpp_env_add(sys_get_env(), &srv->env, &info);
    }

    srv_mempool = srv;
}

void kmpp_mem_pool_deinit(void)
{
    KmppMemPoolSrv *srv = KMPP_FETCH_AND(&srv_mempool, NULL);
    KmppMemPoolImpl *pool, *m;
    KmppMemPoolNode *node, *n;

    if (!srv)
        return;

    osal_list_for_each_entry_safe(pool, m, &srv->list_pool, KmppMemPoolImpl, list_srv) {
        kmpp_loge_f("pool %2d - %-12s %px size %d max %d not released\n",
                    pool->pool_id, pool->name, pool, pool->size, pool->max_cnt);

        osal_list_for_each_entry_safe(node, n, &pool->unused, KmppMemPoolNode, list) {
            kmpp_loge_f("    node %d:%px ptr %px\n", node->id, node, node->ptr);
            kmpp_free(node);
            pool->unused_count--;
        }

        kmpp_free(pool);
        srv->pool_cnt--;
    }

    kmpp_free(srv);
}

KmppMemPool kmpp_mem_get_pool(const rk_u8 *name, rk_s32 size, rk_u32 max_cnt, rk_u32 flag, const rk_u8 *caller)
{
    KmppMemPoolSrv *srv = get_pool_srv(caller);
    KmppMemPoolImpl *pool;
    rk_ul flags;
    rk_s32 str_len;

    if (!srv)
        return NULL;

    if (!name || !size) {
        kmpp_err_f("invalid mem pool name %s size %d\n", name, size);
        return NULL;
    }

    str_len = KMPP_ALIGN(osal_strlen(name) + 1, 4);
    pool = kmpp_calloc_atomic(sizeof(KmppMemPoolImpl) + str_len);
    if (!pool) {
        kmpp_err_f("failed to alloc mem pool %2d - %-12s\n", name);
        return NULL;
    }

    pool->check = pool;
    pool->max_cnt = max_cnt;
    pool->size = size;

    osal_memcpy((void *)(pool + 1), name, str_len);

    OSAL_INIT_LIST_HEAD(&pool->list_srv);
    OSAL_INIT_LIST_HEAD(&pool->used);
    OSAL_INIT_LIST_HEAD(&pool->unused);

    osal_spin_lock_irqsave(srv->lock, &flags);
    osal_list_add_tail(&pool->list_srv, &srv->list_pool);
    pool->pool_id = srv->pool_id++;
    srv->pool_cnt++;
    osal_spin_unlock_irqrestore(srv->lock, &flags);

    mem_pool_dbg_flow("pool %2d - %-12s get size %4d max %3d at %s\n",
                      pool->pool_id ,name, pool->size, pool->max_cnt, caller);

    return pool;
}

void kmpp_mem_put_pool(KmppMemPool pool, const rk_u8 *caller)
{
    KmppMemPoolSrv *srv = get_pool_srv(caller);
    KmppMemPoolImpl *impl = (KmppMemPoolImpl *)pool;
    KmppMemPoolNode *node, *m;
    rk_ul flags;

    if (!srv)
        return;

    if (impl != impl->check) {
        kmpp_err_f("invalid mem pool %2d - %-12s %px check %px\n", impl->name, impl, impl->check);
        return;
    }

    osal_spin_lock_irqsave(srv->lock, &flags);
    osal_list_del_init(&impl->list_srv);
    srv->pool_cnt--;
    osal_spin_unlock_irqrestore(srv->lock, &flags);

    mem_pool_dbg_flow("pool %2d - %-12s put %4d max:used:unused [%d:%d:%d] at %s\n",
                      impl->pool_id, impl->name, impl->size, impl->max_cnt,
                      impl->used_count, impl->unused_count, caller);

    osal_list_for_each_entry_safe(node, m, &impl->unused, KmppMemPoolNode, list) {
        kmpp_free(node);
        impl->unused_count--;
    }

    if (!osal_list_empty(&impl->used)) {
        kmpp_err_f("pool %2d - %-12s size %d found %d used buffer at %s\n",
                    impl->pool_id, impl->name, impl->size, impl->used_count, caller);

        osal_list_for_each_entry_safe(node, m, &impl->used, KmppMemPoolNode, list) {
            kmpp_free(node);
            impl->used_count--;
        }
    }

    if (impl->used_count || impl->unused_count)
        kmpp_err_f("pool %2d - %-12s size %d max %d found leaked buffer used:unused [%d:%d]\n",
                   impl->pool_id, impl->name, impl->size,
                   impl->max_cnt, impl->used_count, impl->unused_count);

    kmpp_free(impl);
}

void *kmpp_mem_pool_get(KmppMemPool pool, const rk_u8 *caller)
{
    KmppMemPoolSrv *srv = get_pool_srv(caller);
    KmppMemPoolImpl *impl = (KmppMemPoolImpl *)pool;
    KmppMemPoolNode *node = NULL;
    void* ptr = NULL;
    rk_ul flags;

    if (!srv)
        return NULL;

    osal_spin_lock_irqsave(srv->lock, &flags);

    node = osal_list_first_entry_or_null(&impl->unused, KmppMemPoolNode, list);
    if (node) {
        osal_list_del_init(&node->list);
        osal_list_add_tail(&node->list, &impl->used);
        impl->unused_count--;
        impl->used_count++;
        ptr = node->ptr;
        node->check = node;
        goto DONE;
    }

    if (impl->max_cnt > 0 && (impl->unused_count + impl->used_count) >= impl->max_cnt) {
        kmpp_log("pool %2d - %-12s size %d reach max cnt %d\n",
                 impl->pool_id, impl->name, impl->size, impl->max_cnt);
        goto DONE;
    }

    node = kmpp_calloc_atomic(sizeof(KmppMemPoolNode) + KMPP_ALIGN(impl->size, 4));
    if (!node) {
        kmpp_err_f("pool %2d - %-12s failed to create node at %s\n",
                   impl->pool_id, impl->name, caller);
    } else {
        node->check = node;
        node->ptr = (void *)(node + 1);
        node->size = impl->size;
        node->id = impl->node_id++;
        OSAL_INIT_LIST_HEAD(&node->list);
        osal_list_add_tail(&node->list, &impl->used);
        impl->used_count++;
        ptr = node->ptr;
    }

DONE:
    mem_pool_dbg_flow("pool %2d - %-12s size %d node_id %d get used:unused [%d:%d] at %s\n",
                      impl->pool_id, impl->name, impl->size, node->id,
                      impl->used_count, impl->unused_count, caller);
    osal_spin_unlock_irqrestore(srv->lock, &flags);

    return ptr;
}

void kmpp_mem_pool_put(KmppMemPool pool, void *p, const rk_u8 *caller)
{
    KmppMemPoolSrv *srv = get_pool_srv(caller);
    KmppMemPoolImpl *impl = (KmppMemPoolImpl *)pool;
    KmppMemPoolNode *node = (KmppMemPoolNode *)((rk_u8 *)p - sizeof(KmppMemPoolNode));
    rk_ul flags;

    if (!srv)
        return;

    if (impl != impl->check) {
        kmpp_err_f("invalid mem pool %2d - %-12s %px check %px\n", impl->name, impl, impl->check);
        return;
    }

    if (node != node->check) {
        kmpp_err_f("invalid mem pool %2d - %-12s ptr %px node %px check %px\n",
              impl->name, p, node, node->check);
        return;
    }

    osal_spin_lock_irqsave(srv->lock, &flags);

    osal_list_del_init(&node->list);
    osal_list_add(&node->list, &impl->unused);
    impl->used_count--;
    impl->unused_count++;
    node->check = NULL;

    mem_pool_dbg_flow("pool %2d - %-12s size %d node_id %d put used:unused [%d:%d] at %s\n",
                      impl->pool_id, impl->name, impl->size, node->id,
                      impl->used_count, impl->unused_count, caller);

    osal_spin_unlock_irqrestore(srv->lock, &flags);
}

void kmpp_mem_pool_dump(KmppMemPool pool, const rk_u8 *caller)
{
    KmppMemPoolImpl *impl = (KmppMemPoolImpl *)pool;
    KmppMemPoolNode *node, *n;

    kmpp_logi("dump pool %2d - %-16s size %4d max:used:unused [%d:%d:%d] at %s\n",
              impl->pool_id, impl->name, impl->size,
              impl->max_cnt, impl->used_count, impl->unused_count, caller);

    if (impl->used_count) {
        kmpp_logi("used %d node:\n", impl->used_count);

        osal_list_for_each_entry_safe(node, n, &impl->used, KmppMemPoolNode, list) {
            kmpp_logi_f("  %2d [%px:%px]\n", node->id, node, node->ptr);
        }
    }

    if (impl->unused_count) {
        kmpp_logi_f("unused %d node:\n", impl->unused_count);

        osal_list_for_each_entry_safe(node, n, &impl->unused, KmppMemPoolNode, list) {
            kmpp_logi_f("  %2d [%px:%px]\n", node->id, node, node->ptr);
        }
    }
}

void kmpp_mem_pool_dump_all(const rk_u8 *caller)
{
    KmppMemPoolSrv *srv = get_pool_srv(caller);
    KmppMemPoolImpl *pool, *m;
    rk_ul flag;

    if (!srv)
        return;

    osal_spin_lock_irqsave(srv->lock, &flag);

    kmpp_logi_f("total %d pool at %s\n", srv->pool_cnt, caller);

    osal_list_for_each_entry_safe(pool, m, &srv->list_pool, KmppMemPoolImpl, list_srv) {
        kmpp_mem_pool_dump(pool, caller);
    }

    osal_spin_unlock_irqrestore(srv->lock, &flag);
}
