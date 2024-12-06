// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Fuzhou Rockchip Electronics Co., Ltd
 *
 */

#include "mpp_vcodec_rockit.h"

/* struct vcodec_mpidev_fn */
struct mpi_dev *vcodec_rockit_create_dev(const char *name, struct vcodec_set_dev_fn *dev_fn)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->create_dev)
		return fn->create_dev(name, dev_fn);

	return NULL;
}

int vcodec_rockit_destory_dev(struct mpi_dev *dev)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->destory_dev)
		return fn->destory_dev(dev);

	return -1;
}

int vcodec_rockit_handle_message(struct mpi_obj *obj, int event, void *args)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->handle_message)
		return fn->handle_message(obj, event, args);

	return -1;
}

void *vcodec_rockit_get_chnl_ctx(struct mpi_obj *obj)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->get_chnl_ctx)
		return fn->get_chnl_ctx(obj);

	return NULL;
}

int vcodec_rockit_get_chnl_id(void *out_parm)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->get_chnl_id)
		return fn->get_chnl_id(out_parm);

	return -1;
}

int vcodec_rockit_get_chnl_type(void *out_parm)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->get_chnl_type)
		return fn->get_chnl_type(out_parm);

	return -1;
}

int vcodec_rockit_set_intra_info(RK_S32 chn_id, RK_U64 dts, RK_U64 pts, RK_U32 is_intra)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->set_intra_info)
		return fn->set_intra_info(chn_id, dts, pts, is_intra);

	return -1;
}

int vcodec_rockit_notify_drop_frm(RK_S32 chn_id, RK_U64 dts, enum venc_drop_frm_type type)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->notify_drop_frm)
		return fn->notify_drop_frm(chn_id, dts, type);

	return -1;
}

int vcodec_rockit_notify(RK_S32 chn_id, int cmd, void *arg)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->notify)
		return fn->notify(chn_id, cmd, arg);

	return -1;
}

int vcodec_rockit_get_struct_size(void)
{
	struct vcodec_mpidev_fn *fn = get_mpidev_ops();

	if (fn && fn->get_struct_size)
		return fn->get_struct_size();

	return -1;
}
/* end struct vcodec_mpidev_fn */

/* struct vcodec_mpibuf_fn */
struct mpi_buf *vcodec_rockit_buf_alloc(size_t size)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_alloc)
		return fn->buf_alloc(size);

	return NULL;
}

void *vcodec_rockit_buf_map(struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_map)
		return fn->buf_map(buf);

	return NULL;
}

int  vcodec_rockit_buf_unmap(struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_unmap)
		return fn->buf_unmap(buf);

	return -1;
}

void vcodec_rockit_buf_ref(struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_ref)
		fn->buf_ref(buf);
}

void vcodec_rockit_buf_unref(struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_unref)
		fn->buf_unref(buf);
}

struct dma_buf *vcodec_rockit_buf_get_dmabuf(struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_get_dmabuf)
		return fn->buf_get_dmabuf(buf);

	return NULL;
}

int vcodec_rockit_buf_get_paddr(struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_get_paddr)
		return fn->buf_get_paddr(buf);

	return -1;
}

struct mpi_queue *vcodec_rockit_buf_queue_create(int cnt)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_queue_create)
		return fn->buf_queue_create(cnt);

	return NULL;
}

int vcodec_rockit_buf_queue_destroy( struct mpi_queue *queue)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_queue_destroy)
		return fn->buf_queue_destroy(queue);

	return -1;
}

int vcodec_rockit_buf_queue_push(struct mpi_queue *queue, struct mpi_buf *buf)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_queue_push)
		return fn->buf_queue_push(queue, buf);

	return -1;
}

struct mpi_buf *vcodec_rockit_buf_queue_pop(struct mpi_queue *queue)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_queue_pop)
		return fn->buf_queue_pop(queue);

	return NULL;
}

int vcodec_rockit_buf_queue_size(int chn)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_queue_size)
		return fn->buf_queue_size(chn);

	return -1;
}

struct mpi_buf *vcodec_rockit_dma_buf_import(struct dma_buf *dma_buf, struct mpp_frame_infos *info,
					     int chan_id)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->dma_buf_import)
		return fn->dma_buf_import(dma_buf, info, chan_id);

	return NULL;
}

int vcodec_rockit_get_buf_frm_info(struct mpi_buf *buf, struct mpp_frame_infos *info,
				   RK_S32 chan_id)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->get_buf_frm_info)
		return fn->get_buf_frm_info(buf, info, chan_id);

	return -1;
}

struct mpi_buf_pool *vcodec_rockit_buf_pool_create(size_t buf_size, unsigned int num_bufs)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_pool_create)
		return fn->buf_pool_create(buf_size, num_bufs);

	return NULL;
}

int vcodec_rockit_buf_pool_destroy(struct mpi_buf_pool *pool)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_pool_destroy)
		return fn->buf_pool_destroy(pool);

	return -1;
}

struct mpi_buf *vcodec_rockit_buf_pool_request_buf(struct mpi_buf_pool *pool)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_pool_request_buf)
		return fn->buf_pool_request_buf(pool);

	return NULL;
}

int vcodec_rockit_buf_pool_get_free_num(struct mpi_buf_pool *pool)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_pool_get_free_num)
		return fn->buf_pool_get_free_num(pool);

	return 0;
}

struct mpi_buf *vcodec_rockit_buf_alloc_with_name(size_t size, const char *name)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->buf_alloc_with_name)
		return fn->buf_alloc_with_name(size, name);

	return NULL;
}

int vcodec_rockit_buf_get_struct_size(void)
{
	struct vcodec_mpibuf_fn *fn = get_mpibuf_ops();

	if (fn && fn->get_struct_size)
		return fn->get_struct_size();

	return -1;
}
/* end struct vcodec_mpibuf_fn */
