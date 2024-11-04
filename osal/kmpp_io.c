/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/io.h>

#include "kmpp_io.h"

void osal_writel(rk_u32 val, void *p)
{
    writel(val, p);
}
EXPORT_SYMBOL(osal_writel);

rk_u32 osal_readl(void *p)
{
    return readl(p);
}
EXPORT_SYMBOL(osal_readl);

void osal_writel_relaxed(rk_u32 val, void *p)
{
    writel_relaxed(val, p);
}
EXPORT_SYMBOL(osal_writel_relaxed);

rk_u32 osal_readl_relaxed(void *p)
{
    return readl_relaxed(p);
}
EXPORT_SYMBOL(osal_readl_relaxed);