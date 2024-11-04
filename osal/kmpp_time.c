/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_time"

#include <linux/ktime.h>

#include "kmpp_log.h"
#include "kmpp_time.h"

rk_s64 kmpp_time(void)
{
    return ktime_to_ns(ktime_get());
}
EXPORT_SYMBOL(kmpp_time);

void kmpp_time_diff(rk_s64 start, rk_s64 end, rk_s64 limit, const char *fmt)
{
    rk_s64 diff = end - start;

    if (diff > limit)
        kmpp_logi("%s timing %lld us", fmt, diff);
}
EXPORT_SYMBOL(kmpp_time_diff);