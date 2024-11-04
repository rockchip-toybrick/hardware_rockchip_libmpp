/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_DEV_IMPL_H__
#define __KMPP_DEV_IMPL_H__

#include <linux/iommu.h>
#include <linux/platform_device.h>

#include "rk_list.h"
#include "kmpp_clk_impl.h"
#include "kmpp_spinlock.h"
#include "kmpp_dev.h"
#include "kmpp_cls.h"

struct device;
struct device_node;
struct platform_driver;

typedef struct osal_drv_impl_t {
    osal_drv drv;

    /* list to list_drv in osal_dev_srv */
    osal_list_head list_srv;
    /* list head for list_drv in osal_dev_impl */
    osal_list_head list_dev;

    /* driver name */
    const char *name;

    /* for platform driver register */
    rk_s32 id_cnt;
    struct of_device_id *ids;
    osal_drv_ops dops;

    /* register driver for kernel */
    struct platform_driver *pdev;

    /* driver session property */
    osal_spinlock *lock_session;
    osal_list_head list_session;

    /* record all driver device */
    rk_s32 dev_cnt;
    osal_devs devs;
} osal_drv_impl;

typedef struct osal_dev_impl_t {
    /* device structure for user */
    osal_dev dev;

    /* device cluster using the same driver */
    osal_devs *devs;

    /* list to list_dev in osal_dev_srv */
    osal_list_head list_srv;
    /* list to list_dev in osal_drv_impl */
    osal_list_head list_drv;

    /* device property */
    struct platform_device *pdev;
    struct device *device;
    struct device_node *np;
    osal_drv_impl *drv;

    /* device mmu property */
    struct platform_device *pdev_mmu;
    struct iommu_group *group;
    struct iommu_domain *domain;

    /* device clock property */
    kmpp_clk *clk;

    /* device reset property */
    rk_s32 rst_cnt;
    struct reset_control_bulk_data *rstcs;

    /* device interrupt callback */
    osal_irq irq;
    osal_irq isr;
    osal_irq_mmu irq_mmu;
} osal_dev_impl;

typedef struct osal_cls_impl_t {
    osal_cls cls;

    /* always service kmpp class */
    struct class *class;

    rk_u32 need_free;
} osal_cls_impl;

struct osal_fs_dev_mgr_t {
    const rk_u8 *name;
    rk_s32 is_root;

    /* always service kmpp class */
    rk_s32 cdev_size;
    struct class *cls;
    osal_dev_fops fops;
    void *drv_data;
    rk_s32 priv_size;

    /* cdev register */
    dev_t dev_id;
    struct cdev *cdev;
    struct device *dev;

    rk_u32 need_free;
};

#define devs_to_drv(x) container_of(x, osal_drv_impl, devs)
#define dev_to_impl(x) container_of(x, osal_dev_impl, dev)
#define drv_to_impl(x) container_of(x, osal_drv_impl, drv)
#define cls_to_impl(x) container_of(x, osal_cls_impl, cls)

#endif /* __KMPP_DEV_IMPL_H__ */
