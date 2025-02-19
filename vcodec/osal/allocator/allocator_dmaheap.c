/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "allocator_dmaheap"

#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/dma-heap.h>
#include <linux/module.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_maths.h"
#include "mpp_mem.h"
#include "allocator_dmaheap.h"

struct mpi_buf;

static struct dma_heap *g_dma_heap = NULL;
#define allocator_dma_heap g_dma_heap
static DEFINE_SPINLOCK(lock);

typedef struct dma_buf *(*dma_heap_alloc_func)(struct dma_heap *heap, size_t len,
					  unsigned int fd_flags,
					  unsigned int heap_flags);
typedef void *(*dma_heap_find_func)(const char *name);

static dma_heap_alloc_func dma_heap_alloc = NULL;

static const char *dma_heap_find_name = "find";
static dma_heap_find_func find = NULL;
static const char *alloc_symbol_name;
static char *symbol_name[] = {
	"dma_heap_buffer_alloc",
};

static MPP_RET allocator_init(const char *caller)
{
	unsigned long flags;
	MPP_RET ret = MPP_OK;
	const char *name = "system";

	spin_lock_irqsave(&lock, flags);
	if (!g_dma_heap) {
		struct dma_heap *heap = NULL;
		RK_U32 i;

		find = (dma_heap_find_func)__symbol_get(dma_heap_find_name);
		if (!find) {
			mpp_err_f("dma_heap_find_func not found\n");
			goto __return;
		}
		heap = find(name);
		if (!heap) {
			mpp_err_f("allocator %s not found\n", name);
			ret = MPP_NOK;
			goto __return;
		}
		/* find dma_heap_buffer_alloc func symbol */
		for (i = 0; i < MPP_ARRAY_ELEMS(symbol_name); i++) {
			dma_heap_alloc = (dma_heap_alloc_func)__symbol_get(symbol_name[i]);
			if (dma_heap_alloc) {
				alloc_symbol_name = symbol_name[i];
				break;
			}
		}
		if (!dma_heap_alloc) {
			mpp_err_f("dma_heap_alloc_func not found\n");
			goto __return;
		}
		g_dma_heap = heap;
	}

__return:
	spin_unlock_irqrestore(&lock, flags);
	return ret;
}

static void allocator_deinit(const char *caller)
{
	if (!g_dma_heap)
		return;

	if (dma_heap_alloc)
		__symbol_put(alloc_symbol_name);

	if (find)
		__symbol_put(dma_heap_find_name);
}

static MPP_RET allocator_alloc(MppBufferInfo *info, const char *caller)
{
	struct dma_buf *dma_buf;

	if (!info->size)
		return MPP_NOK;

	if (!dma_heap_alloc) {
		mpp_err_f("dma_heap_alloc_func not found\n");
		return MPP_NOK;
	}

	dma_buf = dma_heap_alloc(allocator_dma_heap, info->size, O_CLOEXEC | O_RDWR, 0);
	if (IS_ERR_OR_NULL(dma_buf)) {
		mpp_err_f("get dma buf failed ret %ld size %zu caller %s\n", PTR_ERR(dma_buf), info->size, caller);
		return MPP_ERR_NOMEM;
	}

	info->dma_buf = dma_buf;

	return MPP_OK;
}

static MPP_RET allocator_free(MppBufferInfo *info, const char *caller)
{
	struct dma_buf *dma_buf = info->dma_buf;

	if (!dma_buf) {
		mpp_err_f("invalid dma buf\n");
		return MPP_ERR_VALUE;
	}

	if (info->ptr) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 01, 0)
		dma_buf_vunmap(dma_buf, info->ptr);
#else
		struct iosys_map map = IOSYS_MAP_INIT_VADDR(info->ptr);

		dma_buf_vunmap(dma_buf, &map);
#endif
		info->ptr = NULL;
	}

	dma_buf_put(dma_buf);

	info->dma_buf = NULL;

	return MPP_OK;
}
static MPP_RET allocator_import(MppBufferInfo *info, const char *caller)
{
	struct dma_buf *dma_buf = NULL;

	if (!info) {
		mpp_err_f("invalid info\n");
		return MPP_ERR_VALUE;
	}

	if (info->fd) {
		dma_buf = dma_buf_get(info->fd);
	}

	if (info->dma_buf) {
		get_dma_buf(info->dma_buf);
		dma_buf = info->dma_buf;
	}

	if (IS_ERR_OR_NULL(dma_buf)) {
		mpp_err_f("dma_buf_get failed fd %d\n", info->fd);
		return MPP_NOK;
	}

	info->size = dma_buf->size;
	info->dma_buf = dma_buf;

	return MPP_OK;
}

static MPP_RET allocator_mmap(MppBufferInfo *info, const char *caller)
{
	void *ptr;
	struct dma_buf *dma_buf = info->dma_buf;

	if (!dma_buf) {
		mpp_err_f("invalid dma buf\n");
		return MPP_ERR_VALUE;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 01, 0)
	ptr = dma_buf_vmap(dma_buf);
#else
	{
		struct iosys_map map;
		dma_buf_vmap(dma_buf, &map);
		ptr = map.vaddr;
	}
#endif
	if (!ptr) {
		mpp_err_f("map mpi buf failed\n");
		return MPP_ERR_NOMEM;
	}
	info->ptr = ptr;

	return MPP_OK;
}


mpp_allocator_api dma_heap_allocator = {
	.init   = allocator_init,
	.deinit = allocator_deinit,
	.alloc  = allocator_alloc,
	.free   = allocator_free,
	.import = allocator_import,
	.mmap   = allocator_mmap,
};