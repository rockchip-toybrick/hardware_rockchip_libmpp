/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_DEV_H__
#define __KMPP_DEV_H__

#include "rk_type.h"

typedef struct osal_dev_res_t {
    const char *name;
    rk_u32 *base;
    rk_u32 *end;
} osal_dev_res;

typedef struct osal_drv_t {
    /* driver session and task context size from driver property */
    rk_s32 session_size;
    rk_s32 task_size;
    rk_s32 result_size;
} osal_drv;

typedef struct osal_grf_t {
    rk_u32 offset;
    rk_u32 value;
} osal_grf;

/* interface for device users, MUST be used in pointer form */
typedef struct osal_dev_t {
    /* device name */
    const char *name;

    rk_s32 dev_uid;
    rk_s32 core_id;

    /* device resources */
    rk_s32 res_cnt;
    osal_dev_res *res;

    rk_s32 irq;
    rk_s32 irq_mmu;

    osal_drv *drv;
    osal_grf *grf;

    /* driver data for device probe process */
    void *drv_data;

    /* private data for driver ops functions */
    void *private;
} osal_dev;

typedef struct osal_drv_id_t {
    const char *compatible_name;
    const void *drv_data;
} osal_drv_id;

/* device driver ops */
typedef struct osal_drv_ops_t {
    rk_s32 (*probe)(osal_dev *);
    rk_s32 (*remove)(osal_dev *);

    void (*shutdown)(osal_dev *);
    rk_s32 (*suspend)(osal_dev *);
    rk_s32 (*resume)(osal_dev *);
} osal_drv_ops;

typedef struct osal_drv_prop_t {
    const char *name;

    rk_s32 id_cnt;
    osal_drv_id *ids;

    osal_drv_ops dops;

    rk_s32 session_size;
    rk_s32 task_size;
} osal_drv_prop;

/* device cluster info for multi core control */
typedef struct osal_devs_t {
    rk_s32 count;
    osal_dev *dev[];
} osal_devs;

typedef rk_s32 (*osal_irq)(osal_dev *dev);
typedef rk_s32 (*osal_irq_mmu)(osal_dev *dev, rk_ul iova, rk_s32 status);

rk_s32 osal_dev_init(void);
rk_s32 osal_dev_deinit(void);

rk_s32 osal_drv_probe(osal_drv_prop *prop, osal_devs **devs);
rk_s32 osal_drv_release(osal_devs *devs);

rk_s32 osal_dev_request_irq(osal_dev *dev, osal_irq irq, osal_irq isr, rk_u32 flag);
rk_s32 osal_dev_request_irq_mmu(osal_dev *dev, osal_irq_mmu handler);

rk_s32 osal_dev_prop_read_s32(osal_dev *dev, const char *name, rk_s32 *val);
rk_s32 osal_dev_prop_read_u32(osal_dev *dev, const char *name, rk_u32 *val);
rk_s32 osal_dev_prop_read_string(osal_dev *dev, const char *name, const char **val);

rk_s32 osal_dev_power_on(osal_dev *dev);
rk_s32 osal_dev_power_off(osal_dev *dev);

rk_s32 osal_dev_mmu_flush_tlb(osal_dev *dev);
rk_s32 osal_dev_iommu_refresh(osal_dev *dev);

osal_dev *osal_dev_get(const char *name);
void *osal_dev_get_device(osal_dev *dev);

#endif /* __KMPP_DEV_H__ */