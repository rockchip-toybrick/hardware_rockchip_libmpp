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

#endif
