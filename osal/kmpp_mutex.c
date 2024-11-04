/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/mutex.h>

#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_mutex.h"

typedef struct osal_mutex_t {
    rk_u32 need_free;
    void *mutex;
} osal_mutex;

rk_s32 osal_mutex_init(osal_mutex **mutex)
{
    if (mutex) {
        osal_mutex *p = osal_kcalloc(sizeof(osal_mutex) + sizeof(struct mutex), osal_gfp_normal);
        struct mutex *s;

        *mutex = NULL;
        if (!p)
            return -ENOMEM;

        s = (struct mutex *)(p + 1);

        mutex_init(s);
        p->mutex = (void *)s;
        p->need_free = 1;
        *mutex = p;

        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_mutex_init);

void osal_mutex_deinit(osal_mutex **mutex)
{
    if (mutex) {
        osal_mutex *p = KMPP_FETCH_AND(mutex, NULL);

        if (p) {
            if (p->mutex)
                mutex_destroy((struct mutex *)(p->mutex));

            if (p->need_free)
                osal_kfree(p);
        }
    }
}
EXPORT_SYMBOL(osal_mutex_deinit);

rk_s32 osal_mutex_size(void)
{
    return sizeof(osal_mutex) + sizeof(struct mutex);
}
EXPORT_SYMBOL(osal_mutex_size);

rk_s32 osal_mutex_assign(osal_mutex **mutex, void *buf, rk_s32 size)
{
    if (mutex && buf && size >= osal_mutex_size()) {
        osal_mutex *p = (osal_mutex *)buf;
        struct mutex *s = (struct mutex *)(p + 1);

        mutex_init(s);
        p->mutex = (void *)s;
        p->need_free = 0;
        *mutex = p;

        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_mutex_assign);

rk_s32 osal_mutex_lock(osal_mutex *mutex)
{
    if (mutex && mutex->mutex) {
        mutex_lock((struct mutex *)(mutex->mutex));
        return 0;
    }

    return -1;
}
EXPORT_SYMBOL(osal_mutex_lock);

rk_s32 osal_mutex_lock_interruptible(osal_mutex *mutex)
{
    if (mutex && mutex->mutex)
        return mutex_lock_interruptible((struct mutex *)(mutex->mutex));

    return -1;
}
EXPORT_SYMBOL(osal_mutex_lock_interruptible);

rk_s32 osal_mutex_trylock(osal_mutex *mutex)
{
    if (mutex && mutex->mutex)
        return mutex_trylock((struct mutex *)(mutex->mutex));

    return -1;
}
EXPORT_SYMBOL(osal_mutex_trylock);

void osal_mutex_unlock(osal_mutex *mutex)
{
    if (mutex && mutex->mutex)
        mutex_unlock((struct mutex *)(mutex->mutex));
}
EXPORT_SYMBOL(osal_mutex_unlock);