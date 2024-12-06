/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_VCODEC_ROCKIT_H__
#define __MPP_VCODEC_ROCKIT_H__

#include "rk_type.h"
#include "rk_export_func.h"

/* struct vcodec_mpidev_fn */
struct mpi_dev *vcodec_rockit_create_dev(const char *name, struct vcodec_set_dev_fn *dev_fn);
int vcodec_rockit_destory_dev(struct mpi_dev *dev);
int vcodec_rockit_handle_message(struct mpi_obj *obj, int event, void *args);
void *vcodec_rockit_get_chnl_ctx(struct mpi_obj *obj);
int vcodec_rockit_get_chnl_id(void *out_parm);
int vcodec_rockit_get_chnl_type(void *out_parm);
int vcodec_rockit_set_intra_info(RK_S32 chn_id, RK_U64 dts, RK_U64 pts, RK_U32 is_intra);
int vcodec_rockit_notify_drop_frm(RK_S32 chn_id, RK_U64 dts, enum venc_drop_frm_type type);
int vcodec_rockit_notify(RK_S32 chn_id, int cmd, void *arg);
int vcodec_rockit_get_struct_size(void);
/* end struct vcodec_mpidev_fn */

/* struct vcodec_mpibuf_fn */
struct mpi_buf *vcodec_rockit_buf_alloc(size_t size);
void *vcodec_rockit_buf_map(struct mpi_buf *buf);
int  vcodec_rockit_buf_unmap(struct mpi_buf *buf);
void vcodec_rockit_buf_ref(struct mpi_buf *buf);
void vcodec_rockit_buf_unref(struct mpi_buf *buf);
struct dma_buf *vcodec_rockit_buf_get_dmabuf(struct mpi_buf *buf);
int vcodec_rockit_buf_get_paddr(struct mpi_buf *buf);
struct mpi_queue *vcodec_rockit_buf_queue_create(int cnt);
int vcodec_rockit_buf_queue_destroy( struct mpi_queue *queue);
int vcodec_rockit_buf_queue_push(struct mpi_queue *queue, struct mpi_buf *buf);
struct mpi_buf *vcodec_rockit_buf_queue_pop(struct mpi_queue *queue);
int vcodec_rockit_buf_queue_size(int chn);
struct mpi_buf *vcodec_rockit_dma_buf_import(struct dma_buf *dma_buf, struct mpp_frame_infos *info,
					     int chan_id);
int vcodec_rockit_get_buf_frm_info(struct mpi_buf *buf, struct mpp_frame_infos *info,
				   RK_S32 chan_id);
struct mpi_buf_pool *vcodec_rockit_buf_pool_create(size_t buf_size, unsigned int num_bufs);
int vcodec_rockit_buf_pool_destroy(struct mpi_buf_pool *pool);
struct mpi_buf *vcodec_rockit_buf_pool_request_buf(struct mpi_buf_pool *pool);
int vcodec_rockit_buf_pool_get_free_num(struct mpi_buf_pool *pool);
struct mpi_buf *vcodec_rockit_buf_alloc_with_name(size_t size, const char *name);
int vcodec_rockit_buf_get_struct_size(void);
/* end struct vcodec_mpibuf_fn */

#endif