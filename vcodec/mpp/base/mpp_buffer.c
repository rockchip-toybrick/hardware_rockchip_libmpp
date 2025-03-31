// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#define MODULE_TAG "mpp_buffer"

#include <linux/string.h>
#include <linux/dma-buf-cache.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/mman.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_buffer_impl.h"
#include "kmpp_mem_pool.h"
#include "mpp_maths.h"
#include "kmpp_dmaheap.h"

static const char *module_name = MODULE_TAG;
struct MppBufferImpl {
	MppBufferInfo   info;
	size_t          offset;
	RK_U32          cir_flag;
	atomic_t        ref_count;
	RK_UL           uaddr;
	RK_U64          iova;
	struct device   *dev;
	KmppDmaBuf	buf;
};

static KmppMemPool mppbuf_pool = NULL;
static KmppDmaHeap mppbuf_heap = NULL;

MPP_RET mpp_buffer_pool_init(RK_U32 max_cnt)
{
	if (!mppbuf_pool)
		mppbuf_pool = kmpp_mem_get_pool_f(module_name, sizeof(struct MppBufferImpl), max_cnt, 0);

	if (!mppbuf_heap)
		kmpp_dmaheap_get_f(&mppbuf_heap, KMPP_DMAHEAP_FLAGS_CACHABLE);

	return MPP_OK;
}

MPP_RET mpp_buffer_pool_deinit(void)
{
	if (mppbuf_heap) {
		kmpp_dmaheap_put_f(mppbuf_heap);
		mppbuf_heap = NULL;
	}

	if (mppbuf_pool) {
		kmpp_mem_put_pool_f(mppbuf_pool);
		mppbuf_pool = NULL;
	}

	return MPP_OK;
}

void mpp_buf_pool_info_show(void *seq_file)
{
	return;
}

MPP_RET mpp_buffer_import_with_tag(MppBufferGroup group, MppBufferInfo *info,
				   MppBuffer *buffer, const char *tag,
				   const char *caller)
{
	MPP_RET ret = MPP_NOK;

	if (NULL == info) {
		mpp_err_f("invalid input: info NULL from %s\n", caller);
		return MPP_ERR_NULL_PTR;
	}
	if (buffer) {
		struct MppBufferImpl *buf = NULL;

		buf = kmpp_mem_pool_get_f(mppbuf_pool);
		if (!buf) {
			mpp_err("mpp_buffer_import fail %s\n", caller);
			return MPP_ERR_NULL_PTR;
		}
		if (info->fd > 0) {
			ret = kmpp_dmabuf_import_fd_f(&buf->buf, info->fd, 0);
		} else if (info->dma_buf) {
			ret = kmpp_dmabuf_import_ctx_f(&buf->buf, info->dma_buf, 0);
		}
		if (ret) {
			mpp_err_f("buffer fd %d dma_buf %p import fail ret %d %s\n",
				  info->fd, info->dma_buf, ret, caller);
			kmpp_mem_pool_put_f(mppbuf_pool, buf);
			return ret;
		}
		info->size = kmpp_dmabuf_get_size(buf->buf);
		buf->info = *info;
		buf->iova = kmpp_invalid_iova();
		atomic_inc(&buf->ref_count);
		*buffer = buf;
	}

	return ret;
}

MPP_RET mpp_buffer_get_with_tag(MppBufferGroup group, MppBuffer *buffer,
				size_t size, const char *tag,
				const char *caller)
{
	struct MppBufferImpl *buf_impl = NULL;

	if (size <= 0) {
		mpp_err_f("size is 0 caller %s\n", caller);
		return MPP_NOK;
	}

	buf_impl = kmpp_mem_pool_get_f(mppbuf_pool);
	if (NULL == buf_impl) {
		mpp_err("buf impl malloc fail : group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		return MPP_ERR_UNKNOW;
	}
	buf_impl->info.size = size;
	if (kmpp_dmabuf_alloc_f(&buf_impl->buf, mppbuf_heap, size, 0)) {
		mpp_err("kmpp_dmabuf_alloc fail : group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		kmpp_mem_pool_put_f(mppbuf_pool, buf_impl);
		return MPP_ERR_UNKNOW;
	}
	buf_impl->iova = kmpp_invalid_iova();
	atomic_inc(&buf_impl->ref_count);
	*buffer = buf_impl;

	return (buf_impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_ring_buffer_get_with_tag(MppBufferGroup group, MppBuffer *buffer,
				     size_t size, const char *tag,
				     const char *caller)
{
	struct MppBufferImpl *buf_impl = NULL;

	buf_impl = kmpp_mem_pool_get_f(mppbuf_pool);
	if (NULL == buf_impl) {
		mpp_err("buf impl malloc fail : group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		return MPP_ERR_UNKNOW;
	}

	if (kmpp_dmabuf_alloc_f(&buf_impl->buf, mppbuf_heap, size, KMPP_DMABUF_FLAGS_DUP_MAP)) {
		mpp_err_f("mpp_buffer_get failed: group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		kmpp_mem_pool_put_f(mppbuf_pool, buf_impl);
		return MPP_ERR_UNKNOW;
	}

	buf_impl->info.size = size;
	buf_impl->info.fd = -1;
	atomic_inc(&buf_impl->ref_count);
	buf_impl->cir_flag = 1;
	buf_impl->iova = kmpp_invalid_iova();
	*buffer = buf_impl;

	return (buf_impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET mpp_buffer_put_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *buf_impl = (struct MppBufferImpl *)buffer;

	if (NULL == buf_impl) {
		mpp_err("mpp_buffer_put invalid input: buffer NULL from %s\n",
			caller);
		return MPP_ERR_UNKNOW;
	}
	if (atomic_dec_and_test(&buf_impl->ref_count)) {
		if (buf_impl->iova != kmpp_invalid_iova())
			kmpp_dmabuf_put_iova_by_device(buf_impl->buf, buf_impl->iova, buf_impl->dev);
		kmpp_dmabuf_free_f(buf_impl->buf);
		kmpp_mem_pool_put_f(mppbuf_pool, buf_impl);
	}

	return MPP_OK;
}

static MPP_RET mpp_buffer_map(struct MppBufferImpl *p)
{
	if (NULL == p->info.ptr) {
		void *ptr = NULL;

		ptr = kmpp_dmabuf_get_kptr(p->buf);
		if (!ptr) {
			mpp_err_f("buffer %p get kptr failed size %zu\n", p, p->info.size);
			return MPP_ERR_UNKNOW;
		}
		p->info.ptr = ptr;
	}

	return MPP_OK;
}

MPP_RET mpp_buffer_inc_ref_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *buf_impl = (struct MppBufferImpl *)buffer;

	if (NULL == buf_impl) {
		mpp_err("mpp_buffer_inc_ref invalid input: buffer NULL from %s\n",
			caller);
		return MPP_ERR_UNKNOW;
	}
	atomic_inc(&buf_impl->ref_count);

	return MPP_OK;
}

MPP_RET mpp_buffer_read_with_caller(MppBuffer buffer, size_t offset, void *data,
				    size_t size, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	void *src = NULL;

	if (NULL == p || NULL == data) {
		mpp_err("mpp_buffer_read invalid input: buffer %p data %p from %s\n",
			buffer, data, caller);
		return MPP_ERR_UNKNOW;
	}

	if (0 == size)
		return MPP_OK;

	if (mpp_buffer_map(p))
		return MPP_NOK;

	src = p->info.ptr;
	mpp_assert(src != NULL);
	if (src)
		memcpy(data, (char *)src + offset, size);

	return MPP_OK;
}

MPP_RET mpp_buffer_write_with_caller(MppBuffer buffer, size_t offset,
				     void *data, size_t size,
				     const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	void *dst = NULL;

	if (NULL == p || NULL == data) {
		mpp_err("mpp_buffer_write invalid input: buffer %p data %p from %s\n",
			buffer, data, caller);
		return MPP_ERR_UNKNOW;
	}

	if (0 == size)
		return MPP_OK;

	if (offset + size > p->info.size)
		return MPP_ERR_VALUE;

	if (mpp_buffer_map(p))
		return MPP_NOK;

	dst = p->info.ptr;
	mpp_assert(dst != NULL);
	if (dst)
		memcpy((char *)dst + offset, data, size);

	return MPP_OK;
}

void *mpp_buffer_get_ptr_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_ptr invalid NULL input from %s\n",
			caller);
		return NULL;
	}

	if (p->info.ptr)
		return p->info.ptr;

	if (mpp_buffer_map(p))
		return NULL;

	mpp_assert(p->info.ptr != NULL);
	if (NULL == p->info.ptr)
		mpp_err("mpp_buffer_get_ptr buffer %p ret NULL from %s\n",
			buffer, caller);

	return p->info.ptr;
}

int mpp_buffer_get_fd_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_fd invalid NULL input from %s\n",
			caller);
		return -1;
	}

	//TODO: maybe need a new fd by dma_buf_fd
	return p->info.fd;
}

struct dma_buf *mpp_buffer_get_dma_with_caller(MppBuffer buffer,
					       const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_dma invalid NULL input from %s\n", caller);
		return NULL;
	}

	if (!p->info.dma_buf)
		mpp_err_f("dma buf invalid buffer %p from %s\n", buffer, caller);

	return p->info.dma_buf;
}

size_t mpp_buffer_get_size_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_size invalid NULL input from %s\n", caller);
		return 0;
	}

	if (p->info.size == 0)
		mpp_err_f("get invalid size buffer %p from %s\n", buffer, caller);

	return p->info.size;
}

int mpp_buffer_get_index_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return -1;
	}

	return p->info.index;
}

MPP_RET mpp_buffer_set_index_with_caller(MppBuffer buffer, int index,
					 const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	p->info.index = index;

	return MPP_OK;
}

size_t mpp_buffer_get_offset_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return -1;
	}

	return p->offset;
}

MPP_RET mpp_buffer_set_offset_with_caller(MppBuffer buffer, size_t offset,
					  const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	p->offset = offset;

	return MPP_OK;
}

MPP_RET mpp_buffer_info_get_with_caller(MppBuffer buffer, MppBufferInfo *info,
					const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == buffer || NULL == info) {
		mpp_err_f("invalid input buffer %p info %p from %s\n", buffer, info, caller);
		return MPP_ERR_UNKNOW;
	}

	*info = p->info;

	return MPP_OK;
}

MPP_RET mpp_buffer_flush_for_cpu_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (!p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	return kmpp_dmabuf_flush_for_cpu(p->buf);
}

MPP_RET mpp_buffer_flush_for_device_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (!p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	return kmpp_dmabuf_flush_for_dev(p->buf, NULL);
}

MPP_RET mpp_buffer_flush_for_cpu_partial_with_caller(MppBuffer buffer, RK_U32 offset, RK_U32 len,
						     const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (!p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	return kmpp_dmabuf_flush_for_cpu_partial(p->buf, offset, len);
}

MPP_RET mpp_buffer_flush_for_device_partial_with_caller(MppBuffer buffer, RK_U32 offset, RK_U32 len,
							const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (!p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	return kmpp_dmabuf_flush_for_dev_partial(p->buf, NULL, offset, len);
}

RK_U32 mpp_buffer_get_iova_f(MppBuffer buffer, MppDev dev, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	MPP_RET ret;
	RK_U64 iova = 0;

	if (NULL == p) {
		mpp_err_f("mpp_buffer_get_offset invalid NULL input from %s\n", caller);
		return -1;
	}

	if (p->iova != kmpp_invalid_iova())
		return (RK_U32)p->iova;

	ret = kmpp_dmabuf_get_iova_by_device(p->buf, &iova, mpp_get_dev(dev));
	if (ret)
		return -1;
	p->dev = mpp_get_dev(dev);
	p->iova = iova;

	return (rk_u32)p->iova;
}

RK_UL mpp_buffer_get_uaddr(MppBuffer buffer)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("mpp_buffer_get_offset invalid NULL input\n");
		return -1;
	}

	if (!p->uaddr || IS_ERR_VALUE(p->uaddr)) {
		p->uaddr = kmpp_dmabuf_get_uptr(p->buf);
	}

	return p->uaddr;
}

KmppDmaBuf mpp_buffer_get_dmabuf(MppBuffer buffer)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	return p ? p->buf : NULL;
}

EXPORT_SYMBOL(mpp_buffer_import_with_tag);
EXPORT_SYMBOL(mpp_buffer_get_with_tag);
EXPORT_SYMBOL(mpp_ring_buffer_get_with_tag);
EXPORT_SYMBOL(mpp_buffer_put_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_ptr_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_fd_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_dma_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_size_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_index_with_caller);
EXPORT_SYMBOL(mpp_buffer_set_index_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_offset_with_caller);
EXPORT_SYMBOL(mpp_buffer_set_offset_with_caller);
EXPORT_SYMBOL(mpp_buffer_info_get_with_caller);
EXPORT_SYMBOL(mpp_buffer_flush_for_cpu_with_caller);
EXPORT_SYMBOL(mpp_buffer_flush_for_device_with_caller);
EXPORT_SYMBOL(mpp_buffer_flush_for_cpu_partial_with_caller);
EXPORT_SYMBOL(mpp_buffer_flush_for_device_partial_with_caller);
EXPORT_SYMBOL(mpp_buffer_get_iova_f);
EXPORT_SYMBOL(mpp_buffer_get_dmabuf);
