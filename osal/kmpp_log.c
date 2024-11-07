/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/kernel.h>
#include <linux/printk.h>

#include "kmpp_log.h"

#define LINE_SZ 512

static rk_s32 mpp_log_level = RK_LOG_INFO;

void _kmpp_log(int level, const char *tag, const char *fmt, const char *fname, ...)
{
    char line[LINE_SZ];
    rk_s32 len;
    va_list args;
    int log_level;

    if (level <= RK_LOG_UNKNOWN || level >= RK_LOG_SILENT)
        return;

    log_level = mpp_log_level;
    if (log_level >= RK_LOG_SILENT)
        return;

    if (level > log_level)
        return;

    len = 0;

    if (tag)
        len += snprintf(line, sizeof(line) - 1, "%s: ", tag);

    if (fname)
        len += snprintf(line + len, sizeof(line) - 1 - len, "%s: ", fname);

    snprintf(line + len, sizeof(line) - 1 - len, "%s", fmt);

    va_start(args, fname);
    vprintk(line, args);
    va_end(args);
}
EXPORT_SYMBOL(_kmpp_log);