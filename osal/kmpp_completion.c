/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/kernel.h>
#include <linux/completion.h>

#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_completion.h"

struct osal_completion_t {
    rk_u32 need_free;
    void *completion;
};

void osal_completion_init(osal_completion **completion)
{
    if (completion) {
        osal_completion *p = osal_kcalloc(sizeof(osal_completion) + sizeof(struct completion), osal_gfp_normal);

        *completion = NULL;
        if (p) {
            struct completion *s = (struct completion *)(p + 1);

            init_completion(s);
            p->need_free = 1;
            p->completion = (void *)s;
            *completion = p;
        }
    }
}
EXPORT_SYMBOL(osal_completion_init);

void osal_completion_deinit(osal_completion **completion)
{
    if (completion) {
        osal_completion *p = KMPP_FETCH_AND(completion, NULL);

        if (p && p->need_free)
            osal_kfree(p);
    }
}
EXPORT_SYMBOL(osal_completion_deinit);

rk_s32 osal_completion_size(void)
{
    return sizeof(osal_completion) + sizeof(struct completion);
}
EXPORT_SYMBOL(osal_completion_size);

rk_s32 osal_completion_assign(osal_completion **completion, void *buf, rk_s32 size)
{
    if (completion && buf && size >= sizeof(osal_completion) + sizeof(struct completion)) {
        osal_completion *p = (osal_completion *)buf;
        struct completion *s = (struct completion *)(p + 1);

        init_completion(s);
        p->need_free = 0;
        p->completion = (void *)s;
        *completion = p;
        return rk_ok;
    }

    if (completion)
        *completion = NULL;

    return rk_nok;
}
EXPORT_SYMBOL(osal_completion_assign);

void osal_completion_reinit(osal_completion *completion)
{
    if (completion && completion->completion)
        reinit_completion((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_completion_reinit);

void osal_wait_for_completion(osal_completion *completion)
{
    if (completion && completion->completion)
        wait_for_completion((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_wait_for_completion);

rk_s32 osal_wait_for_completion_interruptible(osal_completion *completion)
{
    if (!completion || !completion->completion)
        return rk_nok;

    return wait_for_completion_interruptible((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_wait_for_completion_interruptible);

rk_s32 osal_wait_for_completion_timeout(osal_completion *completion, rk_s32 timeout)
{
    if (!completion || !completion->completion)
        return rk_nok;

    return wait_for_completion_timeout((struct completion *)completion->completion, timeout);
}
EXPORT_SYMBOL(osal_wait_for_completion_timeout);

rk_s32 osal_try_wait_for_completion(osal_completion *completion)
{
    if (!completion || !completion->completion)
        return rk_nok;

    return try_wait_for_completion((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_try_wait_for_completion);

rk_s32 osal_complete_done(osal_completion *completion)
{
    if (!completion || !completion->completion)
        return rk_nok;

    return completion_done((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_complete_done);

void osal_complete(osal_completion *completion)
{
    if (completion && completion->completion)
        complete((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_complete);

void osal_complete_all(osal_completion *completion)
{
    if (completion && completion->completion)
        complete_all((struct completion *)completion->completion);
}
EXPORT_SYMBOL(osal_complete_all);