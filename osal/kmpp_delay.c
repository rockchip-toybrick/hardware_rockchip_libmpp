/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/delay.h>

#include "kmpp_delay.h"

void osal_udelay(rk_u32 us)
{
    udelay(us);
}
EXPORT_SYMBOL(osal_udelay);

void osal_mdelay(rk_u32 ms)
{
    mdelay(ms);
}
EXPORT_SYMBOL(osal_mdelay);

void osal_msleep(rk_u32 ms)
{
    msleep(ms);
}
EXPORT_SYMBOL(osal_msleep);

void osal_ssleep(rk_u32 sec)
{
    ssleep(sec);
}
EXPORT_SYMBOL(osal_ssleep);
