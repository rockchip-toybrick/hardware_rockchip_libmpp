/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_CLS_H__
#define __KMPP_CLS_H__

#include "kmpp_dev.h"

typedef struct osal_cls_t {
    const rk_u8     *name;
} osal_cls;

typedef struct osal_fs_dev_mgr_t osal_fs_dev_mgr;

typedef struct osal_fs_dev_t {
    /* device name */
    const rk_u8     *name;
    /* device manager pointer */
    osal_fs_dev_mgr *mgr;
    /* device global data pointer registered on managner init */
    void            *drv_data;
    /* device local struct file pointer */
    void            *file_data;
    /* device local data pointer on file handle open */
    void            *priv_data;
} osal_fs_dev;

typedef rk_s32 (*kmpp_fs_open)(osal_fs_dev *ctx);
typedef rk_s32 (*kmpp_fs_release)(osal_fs_dev *ctx);
typedef rk_s32 (*kmpp_fs_ioctl)(osal_fs_dev *ctx, rk_s32 cmd, void *arg);
typedef rk_s32 (*kmpp_fs_mmap)(osal_fs_dev *ctx, rk_ul start, rk_ul end);

typedef struct osal_dev_fops_t {
    kmpp_fs_open    open;
    kmpp_fs_release release;
    kmpp_fs_ioctl   ioctl;
    kmpp_fs_mmap    mmap;
} osal_dev_fops;

typedef struct osal_fs_dev_cfg_t {
    osal_cls        *cls;
    const rk_u8     *name;      /* name will be update to mgr internal address on osal_fs_dev_mgr_init */
    osal_dev_fops   *fops;

    /* for driver global data and file handle private data */
    void            *drv_data;
    rk_s32          priv_size;

    /* for buffer assign */
    void            *buf;
    rk_s32          size;
} osal_fs_dev_cfg;

rk_s32 osal_env_class_init(void);
rk_s32 osal_class_size(const rk_u8 *name);
rk_s32 osal_class_init(osal_cls **cls, const rk_u8 *name);
rk_s32 osal_class_assign(osal_cls **cls, const rk_u8 *name, void *buf, rk_s32 size);
rk_s32 osal_class_deinit(osal_cls *cls);

rk_s32 osal_fs_dev_mgr_size(const rk_u8 *name);
rk_s32 osal_fs_dev_mgr_init(osal_fs_dev_mgr **mgr, osal_fs_dev_cfg *cfg);
rk_s32 osal_fs_dev_mgr_deinit(osal_fs_dev_mgr *mgr);

#endif /* __KMPP_CLS_H__ */