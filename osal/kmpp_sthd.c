/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/printk.h>

#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_atomic.h"
#include "kmpp_mutex.h"
#include "kmpp_sthd.h"
#include "kmpp_thread.h"

#define KTHREAD_NAME_LEN 32

typedef struct kmpp_thd_t {
    kmpp_thds       base;
    osal_worker     *worker;
    osal_work       *work;
} kmpp_thd;

typedef struct kmpp_thds_impl_t {
    kmpp_thds       base;
    char            name[KTHREAD_NAME_LEN];
    osal_mutex      *lock;
    kmpp_func       func;
    rk_s32          count;
    kmpp_thd        thds[];
} kmpp_thds_impl;

rk_s32 kmpp_thds_init(kmpp_thds **thds, const char *name, rk_s32 count)
{
    kmpp_thds_impl *impl = NULL;
    rk_s32 total_size;
    rk_s32 elem_size;

    if (!thds || count <= 0)
        return -EINVAL;

    elem_size = ALIGN(sizeof(kmpp_thd), 8);
    total_size = ALIGN(sizeof(kmpp_thds_impl), 8) + count * elem_size;

    impl = osal_kcalloc(total_size, osal_gfp_normal);
    if (impl) {
        rk_s32 i;

        if (!name)
            name = "kmpp_thds";

        impl->base.self = &impl->base;
        impl->base.idx = -1;
        impl->count = count;

        osal_mutex_init(&impl->lock);

        for (i = 0; i < count; i++) {
            kmpp_thd *thd = &impl->thds[i];

            snprintf(impl->name, KTHREAD_NAME_LEN - 1, "%s-%d", name, i);

            osal_worker_init(&thd->worker, impl->name);
            thd->base.self = &impl->base;
            thd->base.idx = i;
        }

        snprintf(impl->name, KTHREAD_NAME_LEN - 1, "%s", name);
    }

    *thds = &impl->base;
    return rk_ok;
}
EXPORT_SYMBOL(kmpp_thds_init);

void kmpp_thds_deinit(kmpp_thds **thds)
{
    if (thds && *thds) {
        kmpp_thds_impl *impl = (kmpp_thds_impl *)osal_cmpxchg(thds, *thds, NULL);
        rk_s32 i;

        osal_mutex_lock(impl->lock);

        for (i = 0; i < impl->count; i++) {
            kmpp_thd *thd = &impl->thds[i];

            osal_worker_deinit(&thd->worker);
            osal_work_deinit(&thd->work);
            thd->base.idx = -1;
        }

        osal_mutex_unlock(impl->lock);

        osal_mutex_deinit(&impl->lock);
        osal_kfree(impl);
    }
}
EXPORT_SYMBOL(kmpp_thds_deinit);

void kmpp_thds_setup(kmpp_thds *thds, kmpp_func func, void *ctx)
{
    kmpp_thds_impl *impl = (kmpp_thds_impl *)thds;
    rk_s32 i;

    osal_assert(impl);

    osal_mutex_lock(impl->lock);
    impl->base.ctx = ctx;
    impl->func = func;

    for (i = 0; i < impl->count; i++) {
        kmpp_thd *thd = &impl->thds[i];

        thd->base.ctx = ctx;
        osal_work_init(&thd->work, func, &thd->base);
    }

    osal_mutex_unlock(impl->lock);
}
EXPORT_SYMBOL(kmpp_thds_setup);

const char *kmpp_thds_get_name(kmpp_thds *thds)
{
    kmpp_thds_impl *impl = (kmpp_thds_impl *)thds->self;

    return impl->name;
}
EXPORT_SYMBOL(kmpp_thds_get_name);

kmpp_thds *kmpp_thds_get_entry(kmpp_thds *thds, rk_s32 idx)
{
    kmpp_thds_impl *impl = (kmpp_thds_impl *)thds->self;

    if (idx >= 0 && idx < impl->count)
        return &impl->thds[idx].base;

    return NULL;
}
EXPORT_SYMBOL(kmpp_thds_get_entry);

void kmpp_thds_lock(kmpp_thds *thds)
{
    kmpp_thds_impl *impl = (kmpp_thds_impl *)(thds->self);

    osal_mutex_lock(impl->lock);
}
EXPORT_SYMBOL(kmpp_thds_lock);

void kmpp_thds_unlock(kmpp_thds *thds)
{
    kmpp_thds_impl *impl = (kmpp_thds_impl *)(thds->self);

    osal_mutex_unlock(impl->lock);
}
EXPORT_SYMBOL(kmpp_thds_unlock);

rk_s32 kmpp_thds_run_nl(kmpp_thds *thds)
{
    kmpp_thds_impl *impl = (kmpp_thds_impl *)(thds->self);
    kmpp_thd *thd = NULL;
    rk_s32 idx = thds->idx;
    rk_s32 ret = rk_nok;
    rk_s32 i;

    osal_assert(impl);

    if (idx < 0) {
        for (i = 0; i < impl->count; i++) {
            thd = &impl->thds[i];
            ret = osal_work_queue(thd->worker, thd->work);
            if (ret > 0)
                return i;
        }
    } else {
        thd = &impl->thds[idx];
        ret = osal_work_queue(thd->worker, thd->work);
        if (ret > 0)
            return idx;
    }

    return -1;
}
EXPORT_SYMBOL(kmpp_thds_run_nl);