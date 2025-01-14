/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_BUFFER_IMPL_H__
#define __MPP_BUFFER_IMPL_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_stream_ring_buf.h"
#include "mpp_device.h"
#include "mpp_buffer.h"
#include "rk_export_func.h"

#define mpp_ring_buffer_get(group, buffer, size) \
        mpp_ring_buffer_get_with_tag(group, buffer, size, MODULE_TAG, __FUNCTION__)

#define mpp_buffer_get_iova(buffer, dev) \
        mpp_buffer_get_iova_f(buffer, dev, __FUNCTION__)

#ifdef __cplusplus
extern "C" {
#endif

RK_U32 mpp_buffer_get_iova_f(MppBuffer buffe, MppDev dev, const char *caller);
RK_S32 mpp_buffer_attach_dev(MppBuffer buffer, MppDev dev, const char *caller);
RK_S32 mpp_buffer_dettach_dev(MppBuffer buffer, const char *caller);
void *mpp_buffer_map_ring_buffer(MppBuffer buffer);

/* mpp buffer pool */
MPP_RET mpp_buffer_pool_init(RK_U32 max_cnt);
MPP_RET mpp_buffer_pool_deinit(void);
void mpp_buf_pool_info_show(void *seq_file);
RK_UL mpp_buffer_get_uaddr(MppBuffer buffer);


#ifndef CONFIG_DMABUF_PARTIAL
#define dma_buf_begin_cpu_access_partial(dma_buf, dir, offset, size) \
	dma_buf_begin_cpu_access(dma_buf, dir)

#define dma_buf_end_cpu_access_partial(dma_buf, dir, offset, size) \
	dma_buf_end_cpu_access(dma_buf, dir)
#endif

#ifdef __cplusplus
}
#endif

#endif /*__MPP_BUFFER_IMPL_H__*/
