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
#include <linux/dma-buf.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_buffer.h"
#include "rk_export_func.h"
#include "mpp_frame.h"
#include "mpp_mem_pool.h"
#include "mpp_maths.h"
#include "mpp_allocator.h"

#define CACHE_LINE_SIZE (64)

static const char *module_name = MODULE_TAG;
struct MppBufferImpl {
	MppBufferInfo info;
	size_t offset;
	RK_U32 cir_flag;
	atomic_t ref_count;
};

static MppMemPool g_mppbuf_pool = NULL;

MPP_RET mpp_buffer_pool_init(RK_U32 max_cnt)
{
	if (g_mppbuf_pool)
		return MPP_OK;

	g_mppbuf_pool = mpp_mem_pool_init(module_name, sizeof(struct MppBufferImpl), max_cnt);
	mpp_allocator_init(__func__);

	return MPP_OK;
}

MPP_RET mpp_buffer_pool_deinit(void)
{
	if (!g_mppbuf_pool)
		return MPP_OK;

	mpp_allocator_deinit(__func__);
	mpp_mem_pool_deinit(g_mppbuf_pool);
	g_mppbuf_pool = NULL;

	return MPP_OK;
}

void mpp_buf_pool_info_show(void *seq_file)
{
	mpp_mem_pool_info_show(seq_file, g_mppbuf_pool);
}

MPP_RET mpp_buffer_import_with_tag(MppBufferGroup group, MppBufferInfo *info,
				   MppBuffer *buffer, const char *tag,
				   const char *caller)
{
	MPP_RET ret = MPP_OK;

	if (NULL == info) {
		mpp_err_f("invalid input: info NULL from %s\n", caller);
		return MPP_ERR_NULL_PTR;
	}
	if (buffer) {
		struct MppBufferImpl *buf = NULL;

		buf = mpp_mem_pool_get(g_mppbuf_pool);
		if (!buf) {
			mpp_err("mpp_buffer_import fail %s\n", caller);
			return MPP_ERR_NULL_PTR;
		}
		ret = mpp_allocator_import(info, caller);
		if (ret) {
			mpp_err_f("buffer import fail %s\n", caller);
			mpp_mem_pool_put(g_mppbuf_pool, buf);
			return ret;
		}
		buf->info = *info;
		atomic_inc(&buf->ref_count);
		buf->info.fd = -1;
		*buffer = buf;
	}

	return ret;
}

MPP_RET mpi_buf_ref_with_tag(struct mpi_buf *buf, const char *tag,
			     const char *caller)
{
	struct vcodec_mpibuf_fn *mpibuf_fn = get_mpibuf_ops();

	if (!mpibuf_fn) {
		mpp_err_f("mpibuf_ops get fail");
		return MPP_NOK;
	}
	if (buf) {
		if (mpibuf_fn->buf_ref)
			mpibuf_fn->buf_ref(buf);
	}

	return MPP_OK;
}

MPP_RET mpi_buf_unref_with_tag(struct mpi_buf *buf, const char *tag,
			       const char *caller)
{
	struct vcodec_mpibuf_fn *mpibuf_fn = get_mpibuf_ops();

	if (!mpibuf_fn) {
		mpp_err_f("mpibuf_ops get fail");
		return MPP_NOK;
	}
	if (buf) {
		if (mpibuf_fn->buf_unref)
			mpibuf_fn->buf_unref(buf);
	}

	return MPP_OK;
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

	buf_impl = mpp_mem_pool_get(g_mppbuf_pool);
	if (NULL == buf_impl) {
		mpp_err("buf impl malloc fail : group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		return MPP_ERR_UNKNOW;
	}
	buf_impl->info.size = size;
	if (mpp_allocator_alloc(&buf_impl->info, caller)) {
		mpp_err("mpp_buffer_get failed: group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		mpp_mem_pool_put(g_mppbuf_pool, buf_impl);
		return MPP_ERR_UNKNOW;
	}

	atomic_inc(&buf_impl->ref_count);
	*buffer = buf_impl;

	return (buf_impl) ? (MPP_OK) : (MPP_NOK);
}
EXPORT_SYMBOL(mpp_buffer_get_with_tag);

MPP_RET mpp_ring_buffer_get_with_tag(MppBufferGroup group, MppBuffer *buffer,
				     size_t size, const char *tag,
				     const char *caller)
{
	struct MppBufferImpl *buf_impl = NULL;

	buf_impl = mpp_mem_pool_get(g_mppbuf_pool);
	if (NULL == buf_impl) {
		mpp_err("buf impl malloc fail : group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		return MPP_ERR_UNKNOW;
	}

	buf_impl->info.size = size;
	if (mpp_allocator_alloc(&buf_impl->info, caller)) {
		mpp_err("mpp_buffer_get failed: group %p buffer %p size %u from %s\n",
			group, buffer, (RK_U32)size, caller);
		mpp_mem_pool_put(g_mppbuf_pool, buf_impl);
		return MPP_ERR_UNKNOW;
	}

	buf_impl->info.fd = -1;
	atomic_inc(&buf_impl->ref_count);
	buf_impl->cir_flag = 1;
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
		if (buf_impl->cir_flag) {
			vunmap(buf_impl->info.ptr);
			buf_impl->info.ptr = NULL;
		}
		if (buf_impl->info.iova && buf_impl->info.attach)
			mpp_buffer_dettach_dev(buffer, caller);
		mpp_allocator_free(&buf_impl->info, caller);

		mpp_mem_pool_put(g_mppbuf_pool, buf_impl);
	}

	return MPP_OK;
}
EXPORT_SYMBOL(mpp_buffer_put_with_caller);

void *mpp_buffer_map_ring_ptr(struct MppBufferImpl *p)
{
	RK_U32 end = 0, start = 0;
	RK_S32 i = 0;
	struct page **pages;
	RK_S32 page_count;
	RK_S32 phy_addr = mpp_buffer_get_phy(p);

	if (phy_addr == -1)
		phy_addr = mpp_srv_get_phy(p->info.dma_buf);

	end = phy_addr + p->info.size;
	start = phy_addr;
	end = round_up(end, PAGE_SIZE);

	if (phy_addr & 0xfff) {
		mpp_err("alloc buf start is no 4k align");
		return NULL;
	}

	page_count = (((end - start) >> PAGE_SHIFT) + 1) * 2;
	pages = kmalloc_array(page_count, sizeof(*pages), GFP_KERNEL);
	if (!pages)
		return NULL;

	i = 0;
	while (start < end) {
		mpp_assert(i < page_count);
		pages[i++] = phys_to_page(start);
		start += PAGE_SIZE;
	}

	start = phy_addr;
	while (start < end) {
		mpp_assert(i < page_count);
		pages[i++] = phys_to_page(start);
		start += PAGE_SIZE;
	}

	p->info.ptr = vmap(pages, i, VM_MAP, PAGE_KERNEL);

	kfree(pages);
	return p->info.ptr;
}

void *mpp_buffer_map_ring_buffer(MppBuffer buffer)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("invalid input: buffer NULL\n");
		return NULL;
	}

	if (!p->info.sgt) {
		mpp_err_f("invalid sgt NULL\n");
		return NULL;
	}

	{
		struct sg_table *table = p->info.sgt;
		int npages = PAGE_ALIGN(p->info.size) / PAGE_SIZE;
		struct page **pages = vmalloc(sizeof(struct page *) * npages * 2);
		struct page **tmp = pages;
		struct sg_page_iter piter;
		void *vaddr;

		if (!pages)
			return ERR_PTR(-ENOMEM);

		for_each_sgtable_page(table, &piter, 0) {
			// WARN_ON(tmp - pages >= npages);
			*tmp++ = sg_page_iter_page(&piter);
		}
		table = p->info.sgt;
		for_each_sgtable_page(table, &piter, 0) {
			// WARN_ON(tmp - pages >= npages);
			*tmp++ = sg_page_iter_page(&piter);
		}

		vaddr = vmap(pages, 2 * npages, VM_MAP, PAGE_KERNEL);
		vfree(pages);

		if (!vaddr) {
			mpp_err_f("vmap failed\n");
			return ERR_PTR(-ENOMEM);
		}
		p->info.ptr = vaddr;

		return vaddr;
	}
}

static MPP_RET mpp_buffer_map(struct MppBufferImpl *p)
{
	if (NULL == p->info.ptr) {
		if (p->cir_flag)
			p->info.ptr = mpp_buffer_map_ring_ptr(p);
		else
			mpp_allocator_mmap(&p->info, __func__);
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
EXPORT_SYMBOL(mpp_buffer_get_ptr_with_caller);

int mpp_buffer_get_fd_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	int fd = -1;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_fd invalid NULL input from %s\n",
			caller);
		return -1;
	}

	if (p->info.fd > 0) {
		struct dma_buf *dma_buf = dma_buf_get(p->info.fd);

		if (IS_ERR_OR_NULL(dma_buf)) {
			RK_S32 new_fd = dma_buf_fd(p->info.dma_buf, 0);
			mpp_err_f("return new fd %d from %s\n", new_fd, caller);
			return new_fd;
		}
		dma_buf_put(dma_buf);
		return p->info.fd;
	}

	fd = dma_buf_fd(p->info.dma_buf, 0);
	mpp_assert(fd > 0);

	p->info.fd = fd;

	return fd;
}
EXPORT_SYMBOL(mpp_buffer_get_fd_with_caller);
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
EXPORT_SYMBOL(mpp_buffer_get_dma_with_caller);

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

	if (NULL == p->info.ptr)
		mpp_allocator_mmap(&p->info, caller);

	*info = p->info;

	return MPP_OK;
}

MPP_RET mpp_buffer_flush_for_cpu_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (!p || !p->info.dma_buf) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	dma_buf_begin_cpu_access(p->info.dma_buf, DMA_FROM_DEVICE);

	return MPP_OK;
}

MPP_RET mpp_buffer_flush_for_device_with_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (!p || !p->info.dma_buf) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	dma_buf_end_cpu_access(p->info.dma_buf, DMA_TO_DEVICE);

	return MPP_OK;
}

MPP_RET mpp_buffer_flush_for_cpu_partial_with_caller(MppBuffer buffer, RK_U32 offset, RK_U32 len,
						     const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	RK_U32 _offset = 0;
	RK_U32 _len = 0;

	if (!p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	_offset = MPP_ALIGN_DOWN(offset, CACHE_LINE_SIZE);
	_len = MPP_ALIGN(len + offset - _offset, CACHE_LINE_SIZE);

	dma_buf_begin_cpu_access_partial(p->info.dma_buf, DMA_FROM_DEVICE, _offset, _len);

	return MPP_OK;
}

MPP_RET mpp_buffer_flush_for_device_partial_with_caller(MppBuffer buffer, RK_U32 offset, RK_U32 len,
							const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	RK_U32 _offset = 0;
	RK_U32 _len = 0;

	if (!p) {
		mpp_err_f("invalid NULL input from %s\n", caller);
		return MPP_ERR_UNKNOW;
	}

	_offset = MPP_ALIGN_DOWN(offset, CACHE_LINE_SIZE);
	_len = MPP_ALIGN(len + offset - _offset, CACHE_LINE_SIZE);

	dma_buf_end_cpu_access_partial(p->info.dma_buf, DMA_TO_DEVICE, _offset, _len);

	return MPP_OK;
}


RK_S32 mpp_buffer_get_mpi_buf_id_with_caller(MppBuffer buffer,
					     const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	struct vcodec_mpibuf_fn *mpibuf_fn = get_mpibuf_ops();
	struct mpp_frame_infos frm_info;

	if (!mpibuf_fn) {
		// mpp_err_f("mpibuf_ops get fail");
		return -1;
	}

	if (NULL == buffer) {
		mpp_err("mpp_buffer_get_mpi_buf_id invalid input buffer %p from %s\n",
			buffer, caller);
		return MPP_ERR_UNKNOW;
	}
	memset(&frm_info, 0, sizeof(frm_info));
	if (mpibuf_fn->get_buf_frm_info) {
		if (mpibuf_fn->get_buf_frm_info(p->info.hnd, &frm_info, -1))
			return -1;
	} else {
		mpp_err("get buf info fail");
		return -1;
	}

	return frm_info.mpi_buf_id;
}

void mpp_buffer_set_phy_caller(MppBuffer buffer, RK_U32 phy_addr, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_offset invalid NULL input from %s\n",
			caller);
		return;
	}
	p->info.phy_flg = 1;
	p->info.phy_addr = phy_addr;

	return;
}
RK_S32 mpp_buffer_get_phy_caller(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	struct vcodec_mpibuf_fn *mpibuf_fn = get_mpibuf_ops();
	RK_S32 phy_addr = -1;

	if (NULL == p) {
		mpp_err("mpp_buffer_get_offset invalid NULL input from %s\n", caller);
		return -1;
	}

	if (p->info.phy_flg)
		phy_addr = (RK_S32)p->info.phy_addr;
	else if (mpibuf_fn && mpibuf_fn->buf_get_paddr) {
		phy_addr = mpibuf_fn->buf_get_paddr(p->info.hnd);
		if (phy_addr != -1) {
			p->info.phy_addr = phy_addr;
			p->info.phy_flg = 1;
		}
	}

	return phy_addr;
}

RK_S32 mpp_buffer_attach_dev(MppBuffer buffer, MppDev dev, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	RK_S32 ret = 0;

	if (NULL == p) {
		mpp_err_f("mpp_buffer_get_offset invalid NULL input from %s\n", caller);
		return MPP_ERR_NULL_PTR;
	}

	attach = dma_buf_attach(p->info.dma_buf, mpp_get_dev(dev));
	if (IS_ERR(attach)) {
		ret = PTR_ERR(attach);
		mpp_err_f("dma_buf_attach failed(%d) caller %s\n", ret, caller);
		return ret;
	}

	sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
	if (IS_ERR(sgt)) {
		ret = PTR_ERR(sgt);
		mpp_err_f("dma_buf_map_attachment failed(%d) caller %s\n", ret, caller);
		goto fail_map;
	}
	p->info.iova = sg_dma_address(sgt->sgl);
	p->info.sgt = sgt;
	p->info.attach = attach;

	return 0;
fail_map:
	dma_buf_detach(p->info.dma_buf, attach);
	return ret;
}

RK_S32 mpp_buffer_dettach_dev(MppBuffer buffer, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;

	if (NULL == p) {
		mpp_err_f("mpp_buffer_get_offset invalid NULL input from %s\n", caller);
		return MPP_ERR_NULL_PTR;
	}

	dma_buf_unmap_attachment(p->info.attach, p->info.sgt, DMA_BIDIRECTIONAL);
	dma_buf_detach(p->info.dma_buf, p->info.attach);
	p->info.iova = 0;
	p->info.attach = NULL;
	p->info.sgt = NULL;

	return 0;
}

RK_U32 mpp_buffer_get_iova_f(MppBuffer buffer, MppDev dev, const char *caller)
{
	struct MppBufferImpl *p = (struct MppBufferImpl *)buffer;
	MPP_RET ret;

	if (NULL == p) {
		mpp_err_f("mpp_buffer_get_offset invalid NULL input from %s\n", caller);
		return -1;
	}

	if (p->info.iova) {
		return (RK_U32)p->info.iova;
	}

	ret = mpp_buffer_attach_dev(buffer, dev, caller);

	if (ret)
		return -1;

	return p->info.iova;
}
