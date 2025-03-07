// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2022 Rockchip Electronics Co., Ltd.
 *
 * author: timkingh.huang@rock-chips.com
 *
 */

#ifndef __VEPU_PP_COMMON_H__
#define __VEPU_PP_COMMON_H__

#include <linux/kernel.h>

#include "mpp_debug.h"
#include "mpp_common.h"
#include "kmpp_dmabuf.h"

struct dma_buf;

#define pp_err(fmt, args...)					\
		pr_err("%s:%d: " fmt, __func__, __LINE__, ##args)

#if __SIZEOF_POINTER__ == 4
#define REQ_DATA_PTR(ptr) ((u32)ptr)
#elif __SIZEOF_POINTER__ == 8
#define REQ_DATA_PTR(ptr) ((u64)ptr)
#endif

#define MPP_DEVICE_RKVENC_PP  (22)

/* define flags for mpp_request */
#define MPP_FLAGS_MULTI_MSG         (0x00000001)
#define MPP_FLAGS_LAST_MSG          (0x00000002)

#define PP_ALIGN(x, a)         (((x)+(a)-1)&~((a)-1))

enum PP_RET {
	VEPU_PP_NOK = -1,
	VEPU_PP_OK = 0,
};

typedef enum pp_hw_fmt_e {
	VEPU_PP_HW_FMT_BGRA8888,     // 0
	VEPU_PP_HW_FMT_RGB888,       // 1
	VEPU_PP_HW_FMT_RGB565,       // 2
	VEPU_PP_HW_FMT_RSV_3,        // 3
	VEPU_PP_HW_FMT_YUV422SP,     // 4
	VEPU_PP_HW_FMT_YUV422P,      // 5
	VEPU_PP_HW_FMT_YUV420SP,     // 6
	VEPU_PP_HW_FMT_YUV420P,      // 7
	VEPU_PP_HW_FMT_YUYV422,      // 8
	VEPU_PP_HW_FMT_UYVY422,      // 9
	VEPU_PP_HW_FMT_YUV400,       // 10
	VEPU_PP_HW_FMT_RSV_11,       // 11
	VEPU_PP_HW_FMT_YUV444SP,     // 12
	VEPU_PP_HW_FMT_YUV444P,      // 13
	VEPU_PP_HW_FMT_BUTT,
} pp_hw_fmt;

struct dev_reg_wr_t {
	void *data_ptr;
	u32 size;
	u32 offset;
};

struct dev_reg_rd_t {
	void *data_ptr;
	u32 size;
	u32 offset;
};

struct pp_srv_api_t {
	const char *name;
	int ctx_size;
	enum PP_RET(*init) (void *ctx, int type);
	enum PP_RET(*deinit) (void *ctx);
	enum PP_RET(*reg_wr) (void *ctx, struct dev_reg_wr_t *cfg);
	enum PP_RET(*reg_rd) (void *ctx, struct dev_reg_rd_t *cfg);
	enum PP_RET(*cmd_send) (void *ctx);

	/* poll cmd from hardware */
	enum PP_RET(*cmd_poll) (void *ctx);
	u32 (*get_address) (void *ctx, struct dma_buf *buf, u32 offset);
	void (*release_address) (void *ctx, struct dma_buf *buf);
	struct device *(*get_dev)(void *ctx);
};

typedef struct vepu_pp_hw_fmt_cfg_t {
        pp_hw_fmt format;
	u32 alpha_swap;
	u32 rbuv_swap;
	u32 src_endian;
} vepu_pp_hw_fmt_cfg;

struct pp_buffer_t {
	osal_dmabuf *osal_buf;
	dma_addr_t iova;
};

struct pp_output_t {
	u32 od_out_pix_sum; /* 0x4070 */
};

void vepu_pp_transform_hw_fmt_cfg(vepu_pp_hw_fmt_cfg *cfg, MppFrameFormat format);

#endif
