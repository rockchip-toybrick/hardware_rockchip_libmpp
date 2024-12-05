/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_uaccess"

#include <linux/uaccess.h>

#include "kmpp_uaccess.h"

rk_ul osal_copy_from_user(void *dst, const void *src, rk_ul size)
{
    if (access_ok(src, size))
        return copy_from_user(dst, src, size);

    memcpy(dst, src, size);
    return 0;
}
EXPORT_SYMBOL(osal_copy_from_user);

rk_ul osal_copy_to_user(void *dst, const void *src, rk_ul size)
{
    if (access_ok(dst, size))
        return copy_to_user(dst, src, size);

    memcpy(dst, src, size);
    return 0;
}
EXPORT_SYMBOL(osal_copy_to_user);

