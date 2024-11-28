/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_FILE_H__
#define __KMPP_FILE_H__

#include "rk_type.h"

#define OSAL_SEEK_SET       0
#define OSAL_SEEK_CUR       1
#define OSAL_SEEK_END       2

#define OSAL_O_ACCMODE      00000003
#define OSAL_O_RDONLY       00000000
#define OSAL_O_WRONLY       00000001
#define OSAL_O_RDWR         00000002
#define OSAL_O_CREAT        00000100
#define OSAL_O_EXCL         00000200
#define OSAL_O_NOCTTY       00000400
#define OSAL_O_TRUNC        00001000
#define OSAL_O_APPEND       00002000
#define OSAL_O_NONBLOCK     00004000

typedef struct osal_file_t {
    void *file;
} osal_file;

osal_file *osal_fopen(const char *filename, int flags, int mode);
void   osal_fclose(osal_file *file);

rk_s32 osal_fread(osal_file *file, char *buf, unsigned int len);
rk_s32 osal_fwrite(osal_file *file, const char *buf, int len);
rk_s32 osal_fseek(osal_file *file, rk_s32 offset, int whence);

#endif /* __KMPP_FILE_H__ */