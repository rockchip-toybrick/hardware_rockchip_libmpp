/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "allocator_mpibuf"

#include <linux/dma-buf.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_mem.h"
#include "rk_export_func.h"
#include "allocator_mpibuf.h"

struct mpi_buf;
static struct vcodec_mpibuf_fn *g_mpibuf_fn = NULL;

#define rockit_allocator g_mpibuf_fn

static MPP_RET allocator_init(const char *caller)
{
	g_mpibuf_fn = get_mpibuf_ops();
	if (!g_mpibuf_fn)
		return MPP_NOK;

	return MPP_OK;
}

static MPP_RET allocator_alloc(MppBufferInfo *info, const char *caller)
{
	struct mpi_buf *mpi_buf;
	struct dma_buf *dma_buf;

	mpi_buf = rockit_allocator->buf_alloc_with_name(info->size, caller);
	if (!mpi_buf) {
		mpp_err_f("alloc mpi buf failed\n");
		return MPP_ERR_NOMEM;
	}
	dma_buf = rockit_allocator->buf_get_dmabuf(mpi_buf);
	if (!dma_buf) {
		mpp_err_f("get dma buf failed\n");
		return MPP_ERR_NOMEM;
	}

	info->dma_buf = dma_buf;
	info->hnd = (void*)mpi_buf;

	return MPP_OK;
}

static MPP_RET allocator_free(MppBufferInfo *info, const char *caller)
{
	struct mpi_buf *mpi_buf = info->hnd;

	if (info->ptr) {
		rockit_allocator->buf_unmap(mpi_buf);
		info->ptr = NULL;
	}

	rockit_allocator->buf_unref(mpi_buf);

	return MPP_OK;
}
static MPP_RET allocator_import(MppBufferInfo *info, const char *caller)
{
	struct mpi_buf *mpi_buf = info->hnd;
	struct dma_buf *dma_buf;

	if (!mpi_buf) {
		mpp_err_f("invalid mpi buf\n");
		return MPP_ERR_VALUE;
	}
	/* maybe dma buf is null when online mode */
	dma_buf = rockit_allocator->buf_get_dmabuf(mpi_buf);
	if (dma_buf) {
		info->size = dma_buf->size;
		info->dma_buf = dma_buf;
	}
	rockit_allocator->buf_ref(mpi_buf);

	return MPP_OK;
}

static MPP_RET allocator_mmap(MppBufferInfo *info, const char *caller)
{
	struct mpi_buf *mpi_buf = info->hnd;
	void *ptr;

	ptr = rockit_allocator->buf_map(mpi_buf);
	if (!ptr) {
		mpp_err_f("map mpi buf failed\n");
		return MPP_ERR_NOMEM;
	}
	info->ptr = ptr;

	return MPP_OK;
}


mpp_allocator_api mpi_buf_allocator = {
	.init   = allocator_init,
	.deinit = NULL,
	.alloc  = allocator_alloc,
	.free   = allocator_free,
	.import = allocator_import,
	.mmap   = allocator_mmap,
};