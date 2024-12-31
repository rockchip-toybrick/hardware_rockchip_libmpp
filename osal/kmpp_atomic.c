/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/atomic.h>
#include <linux/slab.h>

#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_string.h"

struct osal_atomic_t {
    rk_u32 need_free;
    atomic_t atomic[];
};

rk_s32 osal_atomic_size(void)
{
    return sizeof(osal_atomic) + sizeof(atomic_t);
}
EXPORT_SYMBOL(osal_atomic_size);

rk_s32 osal_atomic_init(osal_atomic **atomic)
{
    if (atomic) {
        rk_s32 size = osal_atomic_size();
        osal_atomic *p = osal_kcalloc(size, osal_gfp_normal);

        *atomic = NULL;
        if (!p)
            return rk_nok;

        p->need_free = 1;

        *atomic = p;
        return 0;
    }

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_init);

rk_s32 osal_atomic_assign(osal_atomic **atomic, void *buf, rk_s32 size)
{
    if (atomic && buf && size >= osal_atomic_size()) {
        osal_atomic *p = (osal_atomic *)buf;

        p->need_free = 0;

        *atomic = p;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_atomic_assign);

void osal_atomic_deinit(osal_atomic **atomic)
{
    if (atomic) {
        osal_atomic *p = osal_cmpxchg(atomic, *atomic, NULL);

        if (p) {
            if (p->need_free)
                osal_kfree(p);
            else
                osal_memset(p, 0, osal_atomic_size());
        }
    }
}
EXPORT_SYMBOL(osal_atomic_deinit);

rk_s32 osal_atomic_read(osal_atomic *v)
{
    if (v)
        return atomic_read(v->atomic);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_read);

void osal_atomic_set(osal_atomic *v, rk_s32 val)
{
    if (v)
        atomic_set(v->atomic, val);
}
EXPORT_SYMBOL(osal_atomic_set);

rk_s32 osal_atomic_fetch_inc(osal_atomic *v)
{
    if (v)
        return atomic_fetch_inc(v->atomic);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_fetch_inc);

rk_s32 osal_atomic_fetch_dec(osal_atomic *v)
{
    if (v)
        return atomic_fetch_dec(v->atomic);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_fetch_dec);

rk_s32 osal_atomic_inc_return(osal_atomic *v)
{
    if (v)
        return atomic_inc_return(v->atomic);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_inc_return);

rk_s32 osal_atomic_dec_return(osal_atomic *v)
{
    if (v)
        return atomic_dec_return(v->atomic);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_dec_return);

rk_s32 osal_atomic_dec_and_test(osal_atomic *v)
{
    if (v)
        return atomic_dec_and_test(v->atomic);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_dec_and_test);

rk_s32 osal_atomic_xchg(osal_atomic *v, rk_s32 i)
{
    if (v)
        return atomic_xchg(v->atomic, i);

    return -EINVAL;
}
EXPORT_SYMBOL(osal_atomic_xchg);