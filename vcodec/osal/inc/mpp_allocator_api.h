/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_ALLOCATOR_API_H__
#define __MPP_ALLOCATOR_API_H__

#include "mpp_buffer.h"
#include "rk_type.h"

typedef enum {
	MPP_ALLOC_TYPE_MPI_BUF,
	MPP_ALLOC_TYPE_CMA_HEAP,
} mpp_allocator_type;

typedef struct mpp_allocator_api_s {
	mpp_allocator_type type;
	MPP_RET (*init)(const char *caller);
	void 	(*deinit)(const char *caller);
	MPP_RET (*alloc)(MppBufferInfo *info, const char* caller);
	MPP_RET (*free)(MppBufferInfo *info, const char* caller);
	MPP_RET (*import)(MppBufferInfo *info, const char* caller);
	MPP_RET (*mmap)(MppBufferInfo *info, const char* caller);
} mpp_allocator_api;

#endif				/* __MPP_SERVICE_API_H__ */
