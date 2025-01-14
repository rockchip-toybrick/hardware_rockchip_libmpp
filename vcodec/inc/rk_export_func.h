// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *  Herman Chen <herman.chen@rock-chips.com>
 *
 */
#ifndef __RK_EXPORT_FUNC_H__
#define __RK_EXPORT_FUNC_H__

#include <rk_type.h>

struct dma_buf;
struct mpi_obj;

struct pp_md_cfg {
	struct mpi_buf *mdw_buf;
	int switch_sad;
	int thres_sad;
	int thres_move;
	int night_mode;
	int filter_switch;
	int thres_dust_move;
	int thres_dust_blk;
	int thres_dust_chng;
	int reserved[5];
};

struct pp_od_cfg {
	int is_background;
	int thres_complex;
	int thres_area_complex;
	struct mpi_buf *odw_buf;
	int thres_complex_cnt;
	int thres_sad;
	int reserved[5];
};

struct mpp_frame_infos {
	RK_U32  width;
	RK_U32  height;
	RK_U32  hor_stride;
	RK_U32  ver_stride;
	RK_U32  hor_stride_pixel;
	RK_U32  offset_x;
	RK_U32  offset_y;
	RK_U32  fmt;
	RK_U32  fd;
	RK_U64  pts;
	RK_S32  jpeg_chan_id;
	void    *osd_buf;
	RK_S32  mpi_buf_id;
	void    *jpg_combo_osd_buf;
	RK_U32  is_gray;
	RK_U32  is_full;
	RK_U32  phy_addr;
	RK_U64  dts;
	void    *pp_info;
	/* num of pskip need to gen after normal frame */
	RK_U32	pskip_num;
	union {
		RK_U32 val;
		struct {
			RK_U32  eos : 1;
			RK_U32  pskip : 1;
			RK_U32  idr_request : 1;
		};
	};
	/* pp combo */
	RK_S32 pp_chn;
	RK_S32 md_en;
	RK_S32 od_en;
	RK_S32 pp_frm_cnt;
	struct pp_md_cfg md_cfg;
	struct pp_od_cfg od_cfg;
	RK_U32 md_index;
	RK_U32 od_index;
	RK_U32 reserved[8];
};

struct vcodec_mpibuf_fn {
	struct mpi_buf *(*buf_alloc)(size_t size);
	void *(*buf_map)(struct mpi_buf *buf);
	int  (*buf_unmap)(struct mpi_buf *buf);
	void (*buf_ref)(struct mpi_buf *buf);
	void (*buf_unref)(struct mpi_buf *buf);
	struct dma_buf *(*buf_get_dmabuf)(struct mpi_buf *buf);
	int (*buf_get_paddr)(struct mpi_buf *buf);
	struct mpi_queue *(*buf_queue_create)(int cnt);
	int (*buf_queue_destroy)( struct mpi_queue *queue);
	int (*buf_queue_push)(struct mpi_queue *queue, struct mpi_buf *buf);
	struct mpi_buf *(*buf_queue_pop)(struct mpi_queue *queue);
	int (*buf_queue_size)(int chn);
	struct mpi_buf *(*dma_buf_import)(struct dma_buf *dma_buf, struct mpp_frame_infos *info,
					  int chan_id);
	int (*get_buf_frm_info)(struct mpi_buf *buf, struct mpp_frame_infos *info, RK_S32 chan_id);
	struct mpi_buf_pool *(*buf_pool_create)(size_t buf_size, unsigned int num_bufs);
	int (*buf_pool_destroy)(struct mpi_buf_pool *pool);
	struct mpi_buf *(*buf_pool_request_buf)(struct mpi_buf_pool *pool);
	int (*buf_pool_get_free_num)(struct mpi_buf_pool *pool);
	struct mpi_buf *(*buf_alloc_with_name)(size_t size, const char *name);
	int (*get_struct_size)(void);
};

struct vcodec_mppdev_svr_fn {
	struct mpp_session *(*chnl_open)(int client_type);
	int (*chnl_register)(struct mpp_session *session, void *fun, unsigned int chn_id);
	int (*chnl_release)(struct mpp_session *session);
	int (*chnl_add_req)(struct mpp_session *session,  void *reqs);
	unsigned int (*chnl_get_iova_addr)(struct mpp_session *session,  struct dma_buf *buf,
					   unsigned int offset);
	void (*chnl_release_iova_addr)(struct mpp_session *session,  struct dma_buf *buf);

	struct device *(*mpp_chnl_get_dev)(struct mpp_session *session);
	int (*chnl_run_task)(struct mpp_session *session, void *param);
	int (*chnl_check_running)(struct mpp_session *session);
};

#define VENC_MAX_PACK_INFO_NUM 8

struct venc_pack_info {
	RK_U32      flag;
	RK_U32      temporal_id;
	RK_U32      packet_offset;
	RK_U32      packet_len;
};

struct venc_packet {
	RK_S64               u64priv_data;  //map mpi_id
	RK_U64               u64packet_addr; //packet address in kernel space
	RK_U32               len;
	RK_U32               buf_size;

	RK_U64               u64pts;
	RK_U64               u64dts;
	RK_U32               flag;
	RK_U32               temporal_id;
	RK_U32               offset;
	RK_U32               data_num;
	struct venc_pack_info       packet[8];
};

extern struct vcodec_mppdev_svr_fn *get_mppdev_svr_ops(void);
extern RK_U32 mpp_srv_get_phy(struct dma_buf *buf);

extern int mpp_vcodec_clear_buf_resource(void);
extern int mpp_vcodec_run_task(RK_U32 chan_id, RK_S64 pts, RK_S64 dts);

#endif
