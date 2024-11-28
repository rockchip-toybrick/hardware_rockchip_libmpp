/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/fs.h>

#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_file.h"

osal_file *osal_fopen(const char *filename, int flags, int mode)
{
    if (filename) {
        osal_file *p = NULL;
        struct file *f;

        p = osal_kmalloc(sizeof(osal_file), osal_gfp_normal);
        if (!p)
            return NULL;

        f = filp_open(filename, flags, mode);
        if (IS_ERR_OR_NULL(f)) {
            osal_kfree(p);
            return NULL;
        }

        p->file = f;
        return p;
    }

    return NULL;
}
EXPORT_SYMBOL(osal_fopen);

void osal_fclose(osal_file *file)
{
    if (file) {
        filp_close(file->file, NULL);
        osal_kfree(file);
    }
}
EXPORT_SYMBOL(osal_fclose);

rk_s32 osal_fread(osal_file *file, char *buf, unsigned int len)
{
    if (file && file->file) {
        struct file *f = file->file;

        return kernel_read(f, buf, len, &f->f_pos);
    }

    return -1;
}
EXPORT_SYMBOL(osal_fread);

rk_s32 osal_fwrite(osal_file *file, const char *buf, int len)
{
    if (file && file->file) {
        struct file *f = file->file;

        return kernel_write(f, buf, len, &f->f_pos);
    }

    return -1;
}
EXPORT_SYMBOL(osal_fwrite);

rk_s32 osal_fseek(osal_file *file, rk_s32 offset, int whence)
{
    if (file && file->file) {
        struct file *f = file->file;

        return default_llseek(f, offset, whence);
    }

    return -1;
}
EXPORT_SYMBOL(osal_fseek);