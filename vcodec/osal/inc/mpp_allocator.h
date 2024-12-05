/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_ALLOCATOR_H__
#define __MPP_ALLOCATOR_H__

#include "mpp_buffer.h"

enum mpp_allocator_type {
        MPP_ALLOCATOR_MPI_BUF,
        MPP_ALLOCATOR_RK_DMABUF,
        MPP_ALLOCATOR_DMABUF,
};

MPP_RET mpp_allocator_init(enum mpp_allocator_type type, const char* caller);
MPP_RET mpp_allocator_alloc(MppBufferInfo *info, const char *caller);
MPP_RET mpp_allocator_free(MppBufferInfo *info, const char* caller);
MPP_RET mpp_allocator_import(MppBufferInfo *info, const char* caller);
MPP_RET mpp_allocator_mmap(MppBufferInfo *info, const char* caller);

#endif
