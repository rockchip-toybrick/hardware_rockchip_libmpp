/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/rwsem.h>

#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_rwsem.h"

struct osal_rwsemaphore_t {
    rk_u32 need_free;
    void *rwsem;
};

rk_s32 osal_rwsem_init(osal_rwsemaphore **rwsem)
{
    if (rwsem) {
        osal_rwsemaphore *p = osal_kcalloc(osal_rwsem_size(), osal_gfp_normal);
        struct rw_semaphore *s;

        *rwsem = NULL;
        if (!p)
            return -ENOMEM;

        s = (struct rw_semaphore *)(p + 1);
        init_rwsem(s);
        p->rwsem = s;
        p->need_free = 1;
        *rwsem = p;

        return rk_ok;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_rwsem_init);

void osal_rwsem_deinit(osal_rwsemaphore **rwsem)
{
    if (rwsem && *rwsem) {
        osal_rwsemaphore *p = KMPP_FETCH_AND(rwsem, NULL);

        if (p->need_free)
            kmpp_free(p);
    }
}
EXPORT_SYMBOL(osal_rwsem_deinit);

rk_s32 osal_rwsem_size(void)
{
    return sizeof(osal_rwsemaphore) + sizeof(struct rw_semaphore);
}
EXPORT_SYMBOL(osal_rwsem_size);

rk_s32 osal_rwsem_assign(osal_rwsemaphore **rwsem, void *buf, rk_s32 size)
{
    if (size >= osal_rwsem_size()) {
        osal_rwsemaphore *p = (osal_rwsemaphore *)buf;
        struct rw_semaphore *s = (struct rw_semaphore *)(p + 1);;

        init_rwsem(s);
        p->rwsem = s;
        p->need_free = 0;
        *rwsem = p;

        return rk_ok;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_rwsem_assign);

void osal_down_read(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        down_read((struct rw_semaphore *)rwsem->rwsem);
}
EXPORT_SYMBOL(osal_down_read);

rk_s32 osal_down_read_interruptible(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        return down_read_interruptible((struct rw_semaphore *)rwsem->rwsem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_read_interruptible);

rk_s32 osal_down_read_killable(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        return down_read_killable((struct rw_semaphore *)rwsem->rwsem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_read_killable);

rk_s32 osal_down_read_trylock(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        return down_read_trylock((struct rw_semaphore *)rwsem->rwsem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_read_trylock);

void osal_down_write(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        down_write((struct rw_semaphore *)rwsem->rwsem);
}
EXPORT_SYMBOL(osal_down_write);

rk_s32 osal_down_write_killable(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        return down_write_killable((struct rw_semaphore *)rwsem->rwsem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_write_killable);

rk_s32 osal_down_write_trylock(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        return down_write_trylock((struct rw_semaphore *)rwsem->rwsem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_down_write_trylock);

void osal_up_read(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        up_read((struct rw_semaphore *)rwsem->rwsem);
}
EXPORT_SYMBOL(osal_up_read);

void osal_up_write(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        up_write((struct rw_semaphore *)rwsem->rwsem);
}
EXPORT_SYMBOL(osal_up_write);

void osal_downgrade_write(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        downgrade_write((struct rw_semaphore *)rwsem->rwsem);
}
EXPORT_SYMBOL(osal_downgrade_write);

rk_s32 osal_rwsem_is_locked(osal_rwsemaphore *rwsem)
{
    if (rwsem && rwsem->rwsem)
        return rwsem_is_locked((struct rw_semaphore *)rwsem->rwsem);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_rwsem_is_locked);
