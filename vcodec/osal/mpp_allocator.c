/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_allocator"

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_allocator_api.h"
#include "mpp_allocator.h"
#include "allocator_mpibuf.h"
#include "allocator_dmaheap.h"
#include "allocator_rkdmaheap.h"

static mpp_allocator_api *g_allocator = NULL;

MPP_RET mpp_allocator_init(enum mpp_allocator_type type, const char* caller)
{
	switch (type) {
	case MPP_ALLOCATOR_MPI_BUF: {
		g_allocator = &mpi_buf_allocator;
	} break;
	case MPP_ALLOCATOR_RK_DMABUF: {
		g_allocator = &rk_dma_heap_allocator;
	} break;
	case MPP_ALLOCATOR_DMABUF: {
		g_allocator = &dma_heap_allocator;
	} break;
	default:
		break;
	}
	if (g_allocator && g_allocator->init)
		return g_allocator->init(caller);

	return MPP_NOK;
}

void mpp_allocator_deinit(const char *caller)
{
	if (!g_allocator || !g_allocator->deinit)
		return;

	g_allocator->deinit(caller);
	g_allocator = NULL;
}

MPP_RET mpp_allocator_alloc(MppBufferInfo *info, const char *caller)
{
	if (!g_allocator || !g_allocator->alloc)
		return MPP_NOK;

	return g_allocator->alloc(info, caller);
}

MPP_RET mpp_allocator_free(MppBufferInfo *info, const char *caller)
{
	if (!g_allocator || !g_allocator->free)
		return MPP_NOK;

	return g_allocator->free(info, caller);
}

MPP_RET mpp_allocator_import(MppBufferInfo *info, const char *caller)
{
	if (!g_allocator || !g_allocator->import)
		return MPP_NOK;

	return g_allocator->import(info, caller);
}

MPP_RET mpp_allocator_mmap(MppBufferInfo *info, const char *caller)
{
	if (!g_allocator || !g_allocator->mmap)
		return MPP_NOK;

	return g_allocator->mmap(info, caller);
}
