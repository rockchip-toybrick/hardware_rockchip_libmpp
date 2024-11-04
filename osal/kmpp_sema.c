/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/semaphore.h>

#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_sema.h"

struct osal_semaphore_t {
    rk_u32 need_free;
    void *sem;
};

rk_s32 osal_sema_init(osal_semaphore **sem, rk_s32 val)
{
    if (sem) {
        osal_semaphore *p = osal_kcalloc(osal_sema_size(), osal_gfp_normal);
        struct semaphore *s;

        *sem = NULL;
        if (!p)
            return -ENOMEM;

        s = (struct semaphore *)(p + 1);
        sema_init(s, val);
        p->sem = s;
        p->need_free = 1;
        *sem = p;
        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_sema_init);

void osal_sema_deinit(osal_semaphore **sem)
{
    if (sem && *sem) {
        osal_semaphore *p = KMPP_FETCH_AND(sem, NULL);

        if (p && p->need_free)
            osal_kfree(p);
    }
}
EXPORT_SYMBOL(osal_sema_deinit);

rk_s32 osal_sema_size(void)
{
    return sizeof(osal_semaphore) + sizeof(struct semaphore);
}
EXPORT_SYMBOL(osal_sema_size);

rk_s32 osal_sema_assign(osal_semaphore **sem, rk_s32 val, void *buf, rk_s32 size)
{
    if (sem && buf && size >= osal_sema_size()) {
        osal_semaphore *p = (osal_semaphore *)buf;
        struct semaphore *s = (struct semaphore *)(p + 1);

        sema_init(s, val);
        p->sem = s;
        p->need_free = 0;
        *sem = p;
        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_sema_assign);

rk_s32 osal_down(osal_semaphore *sem)
{
    if (sem && sem->sem) {
        down((struct semaphore *)sem->sem);
        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down);

rk_s32 osal_down_interruptible(osal_semaphore *sem)
{
    if (sem && sem->sem)
        return down_interruptible((struct semaphore *)sem->sem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_interruptible);

rk_s32 osal_down_trylock(osal_semaphore *sem)
{
    if (sem && sem->sem)
        return down_trylock((struct semaphore *)sem->sem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_trylock);

void osal_up(osal_semaphore *sem)
{
    if (sem && sem->sem)
        up((struct semaphore *)sem->sem);
}
EXPORT_SYMBOL(osal_up);