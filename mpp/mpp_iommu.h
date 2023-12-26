/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *	Alpha Lin, alpha.lin@rock-chips.com
 *	Randy Li, randy.li@rock-chips.com
 *	Ding Wei, leo.ding@rock-chips.com
 *
 */
#ifndef __ROCKCHIP_MPP_IOMMU_H__
#define __ROCKCHIP_MPP_IOMMU_H__

#include <linux/iommu.h>
#include <linux/dma-mapping.h>

struct mpp_dma_buffer {
	/* link to dma session buffer list */
	struct list_head link;

	/* DMABUF information */
	struct dma_buf *dmabuf;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct sg_table *copy_sgt;
	enum dma_data_direction dir;

	dma_addr_t iova;
	unsigned long size;
	void *vaddr;

	struct kref ref;
	ktime_t last_used;
	/* alloc by device */
	struct device *dev;
};

struct mpp_dma_buffer *
mpp_dma_alloc(struct device *dev, size_t size);
int mpp_dma_free(struct mpp_dma_buffer *buffer);

int mpp_dma_get_iova(struct dma_buf *dmabuf, struct device *dev);

#endif
