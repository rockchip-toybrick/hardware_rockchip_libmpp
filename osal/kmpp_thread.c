/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/kthread.h>

#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_atomic.h"
#include "kmpp_thread.h"

typedef struct OsalWorkImpl_t {
    osal_work           work;
    struct kthread_work work_s;
} OsalWorkImpl;

static void mpp_work_run(struct kthread_work *work_s)
{
    OsalWorkImpl *work = container_of(work_s, OsalWorkImpl, work_s);

    if (work->work.func)
            work->work.ret = work->work.func(work->work.data);
}

void osal_worker_init(osal_worker **worker, const char *name)
{
    if (worker) {
        osal_worker *p = osal_kcalloc(sizeof(osal_worker), osal_gfp_normal);

        if (p)
            p->worker = kthread_create_worker(0, name);

        *worker = p;
    }
}
EXPORT_SYMBOL(osal_worker_init);

void osal_worker_deinit(osal_worker **worker)
{
    if (worker) {
        osal_worker *p = osal_cmpxchg(worker, *worker, NULL);

        if (p && p->worker) {
            kthread_destroy_worker((struct kthread_worker *)(p->worker));
            osal_kfree(p);
        }
    }
}
EXPORT_SYMBOL(osal_worker_deinit);

void osal_worker_flush(osal_worker *worker)
{
    if (IS_ERR_OR_NULL(worker) || IS_ERR_OR_NULL(worker->worker)) {
        kmpp_loge("can not flush invalid worker\n");
        return;
    }
    kthread_flush_worker((struct kthread_worker *)(worker->worker));
}
EXPORT_SYMBOL(osal_worker_flush);

rk_s32 osal_work_init(osal_work **work, osal_work_func func, void *data)
{
    if (work) {
        OsalWorkImpl *p = osal_kmalloc(sizeof(*p), osal_gfp_normal);

        *work = NULL;
        if (!p)
            return -ENOMEM;

        kthread_init_work(&p->work_s, mpp_work_run);
        p->work.func = func;
        p->work.data = data;
        p->work.ret = rk_ok;
        p->work.delay_ms = 0;
        p->work.work = (void *)&p->work_s;
        *work = &p->work;

        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_work_init);

void osal_work_deinit(osal_work **work)
{
    if (work) {
        void *p = osal_cmpxchg(work, *work, NULL);

        if (p)
            osal_kfree(p);
    }
}
EXPORT_SYMBOL(osal_work_deinit);

rk_s32 osal_work_queue(osal_worker *worker, osal_work *work)
{
    if (IS_ERR_OR_NULL(worker) || IS_ERR_OR_NULL(worker->worker)) {
        kmpp_loge("can not queue invalid worker %px\n", worker);
        return -EINVAL;
    }

    if (IS_ERR_OR_NULL(work) || IS_ERR_OR_NULL(work->work)) {
        kmpp_loge("can not queue invalid work %px\n", work);
        return -EINVAL;
    }

    return kthread_queue_work((struct kthread_worker *)worker->worker,
                              &((OsalWorkImpl *)work)->work_s);
}
EXPORT_SYMBOL(osal_work_queue);

rk_s32 osal_delayed_work_init(osal_work **work, osal_work_func func, void *data)
{
    if (work) {
        osal_work *p = osal_kmalloc(sizeof(osal_work) + sizeof(struct kthread_delayed_work), osal_gfp_normal);
        struct kthread_delayed_work *s;

        *work = NULL;
        if (!p)
            return -ENOMEM;

        s = (struct kthread_delayed_work *)(p + 1);
        kthread_init_delayed_work(s, mpp_work_run);
        p->func = func;
        p->data = data;
        p->ret = rk_ok;
        p->delay_ms = 0;
        p->work = (void *)s;
        *work = p;

        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_delayed_work_init);

void osal_delayed_work_deinit(osal_work **work)
{
    if (work) {
        osal_work *p = osal_cmpxchg(work, *work, NULL);

        osal_kfree(p);
    }
}
EXPORT_SYMBOL(osal_delayed_work_deinit);

rk_s32 osal_delayed_work_queue(osal_worker *worker, osal_work *work, rk_s32 delay_ms)
{
    if (IS_ERR_OR_NULL(worker) || IS_ERR_OR_NULL(worker->worker) ||
        IS_ERR_OR_NULL(work) || IS_ERR_OR_NULL(work->work)) {
        kmpp_loge("can not queue invalid delayed work\n");
        return -EINVAL;
    }

    return kthread_queue_delayed_work((struct kthread_worker *)worker->worker,
                                      (struct kthread_delayed_work *)work->work,
                                      delay_ms);
}
EXPORT_SYMBOL(osal_delayed_work_queue);