/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_DMABUF_H__
#define __KMPP_DMABUF_H__

#include "kmpp_dev.h"

typedef enum osal_dma_direction_e {
    OSAL_DMA_BIDIRECTIONAL  = 0,
    OSAL_DMA_TO_DEVICE      = 1,
    OSAL_DMA_FROM_DEVICE    = 2,
    OSAL_DMA_NONE           = 3,
} osal_dma_direction;

typedef struct osal_dmabuf_t {
    rk_u32 size;
    rk_u32 flag;

    /* daddr is iova address for the below device */
    rk_u64 daddr;
    osal_dev *dev;
    void *device;

    /* kernel address for cpu access */
    void *kaddr;
} osal_dmabuf;

rk_s32 osal_dmabuf_mgr_init(void);
rk_s32 osal_dmabuf_mgr_deinit(void);

rk_s32 osal_dmabuf_alloc(osal_dmabuf **dmabuf, osal_dev *dev, rk_s64 size, rk_u32 flag);
rk_s32 osal_dmabuf_free(osal_dmabuf *dmabuf);
rk_s32 osal_dmabuf_sync_for_cpu(osal_dmabuf *dmabuf, osal_dma_direction dir);
rk_s32 osal_dmabuf_sync_for_dev(osal_dmabuf *dmabuf, osal_dma_direction dir);

rk_s32 osal_dmabuf_attach(osal_dmabuf **dst, osal_dmabuf *src, osal_dev *dev);
rk_s32 osal_dmabuf_detach(osal_dmabuf *dmabuf);

/* osal_dmabuf by struct device */
rk_s32 osal_dmabuf_alloc_by_device(osal_dmabuf **dmabuf, void *device, rk_s64 size, rk_u32 flag);
rk_s32 osal_dmabuf_attach_by_device(osal_dmabuf **dst, osal_dmabuf *src, void *device);

#endif /* __KMPP_DMABUF_H__ */