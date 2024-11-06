// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 *
 *
 */

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/dma-buf.h>
#include <linux/slab.h>
#include <linux/seq_file.h>

#include "vepu500_pp.h"
#include "vepu_pp_api.h"
#include "vepu_pp_service_api.h"

#define MDW_STRIDE  (64) /* 64 bytes, 512(64 * 8) blk8x8 */
#define pp_info(fmt, arg...)   //pr_info("PP: " fmt, ##arg)

static struct vepu_pp_ctx_t g_pp_ctx;
static struct vcodec_mpibuf_fn *g_mpi_buf_fn = NULL;

void register_vmpibuf_func_to_pp(struct vcodec_mpibuf_fn *mpibuf_fn)
{
	g_mpi_buf_fn = mpibuf_fn;
}
EXPORT_SYMBOL(register_vmpibuf_func_to_pp);

void unregister_vmpibuf_func_pp(void)
{
	g_mpi_buf_fn = NULL;
}
EXPORT_SYMBOL(unregister_vmpibuf_func_pp);

static struct vcodec_mpibuf_fn * get_vmpibuf_func(void)
{
	if (IS_ERR_OR_NULL(g_mpi_buf_fn)) {
		pr_err("%s failed\n", __FUNCTION__);
		return ERR_PTR(-ENOMEM);
	} else
		return g_mpi_buf_fn;
}

static struct pp_buffer_t * pp_malloc_buffer(struct pp_chn_info_t *info, u32 size)
{
	struct vcodec_mpibuf_fn *func = get_vmpibuf_func();
	struct pp_buffer_t *pp_buf = NULL;

	pp_buf = vmalloc(sizeof(*pp_buf));
	if (IS_ERR_OR_NULL(pp_buf)) {
		pp_err("failed\n");
		return ERR_PTR(-ENOMEM);
	}

	if (func->buf_alloc_with_name) {
		pp_buf->buf = func->buf_alloc_with_name(size, __func__);
		if (pp_buf->buf) {
			if (func->buf_get_dmabuf) {
				pp_buf->buf_dma = func->buf_get_dmabuf(pp_buf->buf);
				if (func->buf_get_paddr)
					pp_buf->iova = func->buf_get_paddr(pp_buf->buf);

				if (pp_buf->iova == -1)
					pp_buf->iova = info->api->get_address(info->dev_srv,
									      pp_buf->buf_dma, 0);
			}
		}
	}

	return pp_buf;
}

static void pp_free_buffer(struct pp_chn_info_t *info, struct pp_buffer_t *pp_buf)
{
	struct vcodec_mpibuf_fn *func = get_vmpibuf_func();

	if (!pp_buf)
		return;

	if (pp_buf->buf_dma)
		info->api->release_address(info->dev_srv, pp_buf->buf_dma);
	if (pp_buf->buf)
		func->buf_unref(pp_buf->buf);

	vfree(pp_buf);
	pp_buf = NULL;
}

static void pp_release_buffer(struct pp_chn_info_t *info)
{
	pp_free_buffer(info, info->buf_rfpw);
	pp_free_buffer(info, info->buf_rfpr);
}

static int pp_allocate_buffer(struct pp_chn_info_t *info)
{
	int w = PP_ALIGN(info->max_width, 32);
	int h = PP_ALIGN(info->max_height, 32);
	int buf_len = 0;
	enum PP_RET ret = VEPU_PP_OK;

	/* alloc buffer for down scale rw */
	buf_len = (w * h) >> 4;
	info->buf_rfpw = pp_malloc_buffer(info, buf_len);
	info->buf_rfpr = pp_malloc_buffer(info, buf_len);
	if (IS_ERR_OR_NULL(info->buf_rfpw) || IS_ERR_OR_NULL(info->buf_rfpr)) {
		pp_err("alloc buffer failed\n");
		ret = VEPU_PP_NOK;
		goto __return;
	}

__return:
	if (ret)
		pp_release_buffer(info);
	return ret;
}

int vepu_pp_create_chn(int chn, struct pp_chn_attr *attr)
{
	struct pp_chn_info_t *info = NULL;

	pr_info("%s %d\n", __FUNCTION__, chn);
	if (chn >= MAX_CHN_NUM) {
		pp_err("vepu pp create channel id %d error\n", chn);
		return VEPU_PP_NOK;
	}

	info = &g_pp_ctx.chn_info[chn];

	memset(info, 0, sizeof(*info));
	info->chn = chn;
	info->width = attr->width;
	info->height = attr->height;
	info->md_en = attr->md_en;
	info->od_en = attr->od_en;
	info->api = &pp_srv_api;

	if (attr->max_width > 0 && attr->max_width <= 4096 &&
	    attr->max_height > 0 && attr->max_height <= 4096) {
		info->max_width = attr->max_width;
		info->max_height = attr->max_height;
	} else {
		info->max_width = attr->width;
		info->max_height = attr->height;
	}

	info->dev_srv = vmalloc(info->api->ctx_size);
	if (info->dev_srv == NULL) {
		pp_err("vepu pp vmalloc failed\n");
		return VEPU_PP_NOK;
	} else
		memset(info->dev_srv, 0, info->api->ctx_size);

	info->api->init(info->dev_srv, MPP_DEVICE_RKVENC_PP);

	return pp_allocate_buffer(info);
}
EXPORT_SYMBOL(vepu_pp_create_chn);

int vepu_pp_destroy_chn(int chn)
{
	struct pp_chn_info_t *info = NULL;

	pr_info("%s %d\n", __FUNCTION__, chn);

	if (chn >= MAX_CHN_NUM) {
		pp_err("vepu pp destroy channel id %d error\n", chn);
		return VEPU_PP_NOK;
	}

	info = &g_pp_ctx.chn_info[chn];

	pp_release_buffer(info);

	info->api->deinit(info->dev_srv);

	if (info->dev_srv) {
		vfree(info->dev_srv);
		info->dev_srv = NULL;
	}

	if (info->buf_rfmwr) {
		kvfree(info->buf_rfmwr);
		info->buf_rfmwr = NULL;
	}

	return VEPU_PP_OK;
}
EXPORT_SYMBOL(vepu_pp_destroy_chn);

static void pp_set_src_addr(struct pp_chn_info_t *info, struct pp_com_cfg *cfg)
{
	struct pp_param_t *p = &info->param;
	u32 adr_src0 = 0, adr_src1 = 0, adr_src2 = 0;
	u32 width = info->width, height = info->height;
	struct vcodec_mpibuf_fn *func = get_vmpibuf_func();
	u32 stride_y = 0, stride_c = 0;

	if (func->buf_get_paddr)
		adr_src0 = func->buf_get_paddr(cfg->src_buf);

	switch (cfg->fmt) {
	case RKVENC_F_YCbCr_420_P: {
		adr_src1 = adr_src0 + width * height;
		adr_src2 = adr_src1 + width * height / 4;
		stride_y = width;
		stride_c = width / 2;
	} break;
	case RKVENC_F_YCbCr_420_SP: {
		adr_src1 = adr_src0 + width * height;
		adr_src2 = adr_src1;
		stride_y = width;
		stride_c = width;
	} break;
	default: {
	}
	}

	p->adr_src0 = adr_src0;
	p->adr_src1 = adr_src1;
	p->adr_src2 = adr_src2;
	p->src_strd0.src_strd0 = stride_y;
	p->src_strd1.src_strd1 = stride_c;
}

static void pp_set_common_addr(struct pp_chn_info_t *info, struct pp_com_cfg *cfg)
{
	struct pp_param_t *p = &info->param;

	//TODO: md_interval > 1
	if (cfg->frm_cnt % 2 == 0) {
		p->dspw_addr = info->buf_rfpw->iova;
		p->dspr_addr = info->buf_rfpr->iova;
	} else {
		p->dspw_addr = info->buf_rfpr->iova;
		p->dspr_addr = info->buf_rfpw->iova;
	}
}

static void pp_set_channel_info(struct pp_chn_info_t *info, struct pp_chn_attr *attr)
{
	info->width = attr->width;
	info->height = attr->height;
	info->md_en = attr->md_en;
	info->od_en = attr->od_en;
	info->frm_accum_gop = 0;
	info->frm_accum_interval = 0;
}

static void pp_set_common_cfg(struct pp_chn_info_t *info, struct pp_com_cfg *cfg)
{
	struct pp_param_t *p = &info->param;
	int frm_cnt = cfg->frm_cnt;
	int gop = cfg->gop ? cfg->gop : 30;
	int interval = cfg->md_interval ? cfg->md_interval : 1;
	int md_od_switch = (info->frm_accum_interval == 0);

	info->frm_num = frm_cnt;
	p->int_en.enc_done_en = 1;
	p->int_msk = 0;
	p->enc_pic.enc_stnd = 2;
	p->enc_pic.cur_frm_ref = 1;//TODO:
	p->enc_rsl.pic_wd8_m1 = (PP_ALIGN(info->width, 8) >> 3) - 1;
	p->enc_rsl.pic_hd8_m1 = (PP_ALIGN(info->height, 8) >> 3) - 1;

	p->src_fmt.src_cfmt = cfg->fmt;

	p->src_fill.pic_wfill = 0;
	p->src_fill.pic_hfill = 0;

	p->pic_ofst.pic_ofst_x = 0;
	p->pic_ofst.pic_ofst_y = 0;

	p->vpp_base_cfg.cur_frm_en_md = info->md_en && md_od_switch && (frm_cnt > 0);
	/*
	 * Fix bug:
	 * Disable the ref_frm_en_md to prevent the hw stuck.
	 * This will cause the flying catkin detection to fail.
	 */
	p->vpp_base_cfg.ref_frm_en_md = 0;

	p->vpp_base_cfg.en_od = info->od_en && md_od_switch;
	p->vpp_base_cfg.background_en_od = (frm_cnt > 0);
	p->vpp_base_cfg.sad_comp_en_od = 1;
	p->vpp_base_cfg.vepu_pp_en = 1;

	/* cime cfg */
	p->me_rnge.cime_srch_dwnh = 4;
	p->me_rnge.cime_srch_uph  = 4;
	p->me_rnge.cime_srch_rgtw = 4;
	p->me_rnge.cime_srch_lftw = 4;
	p->me_rnge.dlt_frm_num    = 1;

	p->me_cfg.srgn_max_num   = 54;
	p->me_cfg.cime_dist_thre = 1024;
	p->me_cfg.rme_srch_h     = 0;
	p->me_cfg.rme_srch_v     = 0;

	p->me_cach.cime_zero_thre = 64;
	p->me_cach.fme_prefsu_en  = 0;
	p->me_cach.colmv_load     = 0;
	p->me_cach.colmv_stor     = 0;

	p->synt_sli0.sli_type = frm_cnt > 0 ? 1 : 2;

	p->jpeg_base_cfg.jpeg_stnd = 0;

	pp_set_src_addr(info, cfg);
	pp_set_common_addr(info, cfg);

	info->frm_accum_interval++;
	if (info->frm_accum_interval == interval)
		info->frm_accum_interval = 0;

	info->frm_accum_gop++;
	if (info->frm_accum_gop == gop)
		info->frm_accum_gop = 0;
}

static void vepu_pp_set_param(struct pp_chn_info_t *info, enum pp_cmd cmd, void *param)
{
	struct pp_param_t *p = &info->param;

	switch (cmd) {
	case PP_CMD_SET_CHANNEL_INFO: {
		struct pp_chn_attr *cfg = (struct pp_chn_attr *)param;
		pp_set_channel_info(info, cfg);
	} break;
	case PP_CMD_SET_COMMON_CFG: {
		struct pp_com_cfg *cfg = (struct pp_com_cfg *)param;

		pp_set_common_cfg(info, cfg);
	} break;
	case PP_CMD_SET_MD_CFG: {
		struct pp_md_cfg *cfg = (struct pp_md_cfg *)param;
		struct vcodec_mpibuf_fn *func = get_vmpibuf_func();

		p->vpp_base_cfg.switch_sad_md = cfg->switch_sad;
		p->vpp_base_cfg.night_mode_en_md = 0;
		p->vpp_base_cfg.flycatkin_flt_en_md = 0;

		p->thd_md_vpp.thres_sad_md = cfg->thres_sad;
		p->thd_md_vpp.thres_move_md = cfg->thres_move;
		p->thd_md_vpp.thres_dust_move_md = cfg->thres_dust_move;
		p->thd_md_vpp.thres_dust_blk_md = cfg->thres_dust_blk;
		p->thd_md_vpp.thres_dust_chng_md = cfg->thres_dust_chng;
		p->vpp_base_cfg.sto_stride_md = 4;
		if (func->buf_get_paddr)
			p->adr_md_vpp = func->buf_get_paddr(cfg->mdw_buf);

		info->flycatkin_en = cfg->filter_switch || cfg->night_mode;
		info->mdw_buf = cfg->mdw_buf;
	} break;
	case PP_CMD_SET_OD_CFG: {
		struct pp_od_cfg *cfg = (struct pp_od_cfg *)param;
		struct vcodec_mpibuf_fn *func = get_vmpibuf_func();

		p->vpp_base_cfg.sto_stride_od = 4;
		p->thd_od_vpp.thres_complex_od = cfg->thres_complex;
		p->thd_od_vpp.thres_complex_cnt_od = cfg->thres_complex_cnt;
		p->thd_od_vpp.thres_sad_od = cfg->thres_sad;
		if (func->buf_get_paddr)
			p->adr_od_vpp = func->buf_get_paddr(cfg->odw_buf);
	} break;
	default: {
	}
	}
}

static int vepu_pp_flycatkin_filter_init_buf(struct pp_chn_info_t *info)
{
	int buf_len = PP_ALIGN(info->max_height, 32) / 32 * MDW_STRIDE * 3;

	info->buf_rfmwr = kvmalloc(buf_len, GFP_KERNEL);
	if (info->buf_rfmwr == NULL) {
		pp_err("vepu pp kvmalloc failed\n");
		return VEPU_PP_NOK;
	}

	info->buf_rfmwr0 = info->buf_rfmwr;
	info->buf_rfmwr1 = info->buf_rfmwr + buf_len / 3;
	info->buf_rfmwr2 = info->buf_rfmwr + buf_len / 3 * 2;
	info->mdw_len = buf_len / 3;

	return VEPU_PP_OK;
}

/* x,y: blk8x8 pos */
static int vepu_pp_get_blk8_move_flag(u8 *src, int x, int y)
{
	u8 *mdw_ptr = src + MDW_STRIDE * y + x / 8;

	return (*mdw_ptr >> (x % 8)) & 1;
}

static int vepu_pp_get_blk8_vld_flag(struct pp_chn_info_t *info, int x, int y)
{
	int b8_col = PP_ALIGN(info->width, 32) / 32;
	int b8_row = PP_ALIGN(info->height, 32) / 32;

	if (x < 0 || x >= b8_col || y < 0 || y >= b8_row)
		return 0;

	return 1;
}

static int vepu_pp_get_blk8_move_cnt(struct pp_chn_info_t *info, int x, int y, int *vld_cnt)
{
	struct vcodec_mpibuf_fn *func = get_vmpibuf_func();
	u8 *mdw_buf = func->buf_map(info->mdw_buf);
	int mv_cnt = 0, vld_num = 0;

	if (vepu_pp_get_blk8_vld_flag(info, x - 1, y - 1)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x - 1, y - 1);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x, y - 1)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x, y - 1);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x + 1, y - 1)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x + 1, y - 1);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x - 1, y)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x - 1, y);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x + 1, y)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x + 1, y);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x - 1, y + 1)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x - 1, y + 1);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x, y + 1)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x, y + 1);
	}

	if (vepu_pp_get_blk8_vld_flag(info, x + 1, y + 1)) {
		vld_num++;
		mv_cnt += vepu_pp_get_blk8_move_flag(mdw_buf, x + 1, y + 1);
	}

	*vld_cnt = vld_num;

	return mv_cnt;
}

/* src: downscaled 8x8 block
 * stride: height of blk8x8
 * x,y: blk8x8 pos
 * avg: average value of blk4x4
 */
static void vepu_pp_get_blk4_average(u8 *src, int stride, int x, int y, u8 *avg)
{
	u8 *src_b8 = src + stride * x * 8 + y * 64;
	u8 *src_b4 = NULL;
	u32 sum;
	int idx;

	for (idx = 0; idx < 4; idx++) {
		src_b4 = src_b8 + (idx % 2) * 4 + (idx / 2) * 4 * 8;
		sum = src_b4[0 + 0 * 8] + src_b4[1 + 0 * 8] + src_b4[2 + 0 * 8] + src_b4[3 + 0 * 8] +
		      src_b4[0 + 1 * 8] + src_b4[1 + 1 * 8] + src_b4[2 + 1 * 8] + src_b4[3 + 1 * 8] +
		      src_b4[0 + 2 * 8] + src_b4[1 + 2 * 8] + src_b4[2 + 2 * 8] + src_b4[3 + 2 * 8] +
		      src_b4[0 + 3 * 8] + src_b4[1 + 3 * 8] + src_b4[2 + 3 * 8] + src_b4[3 + 3 * 8];
		avg[idx] = (u8)(sum >> 4);
	}
}

static u8 vepu_pp_update_dust_flag(struct pp_chn_info_t *info, int x, int y)
{
	struct vcodec_mpibuf_fn *func = get_vmpibuf_func();
	u8 sum_luma_chg = 0;
	u8 cur_ave[4];
	u8 ref_ave[4];
	u8 idx;
	u8 *src, *ref;
	u8 *pre_move;
	int stride = PP_ALIGN(info->height, 32) / 4;

	if (info->frm_num % 2 == 0) {
		src = func->buf_map(info->buf_rfpw->buf);
		ref = func->buf_map(info->buf_rfpr->buf);
		pre_move = info->buf_rfmwr1;
	} else {
		src = func->buf_map(info->buf_rfpr->buf);
		ref = func->buf_map(info->buf_rfpw->buf);
		pre_move = info->buf_rfmwr0;
	}

	vepu_pp_get_blk4_average(src, stride, x, y, cur_ave);
	vepu_pp_get_blk4_average(ref, stride, x, y, ref_ave);

	pp_info("frame %d pos(%d, %d) cur_ave %d %d %d %d ref_ave %d %d %d %d\n",
		info->frm_num, x * 32, y * 32, cur_ave[0], cur_ave[1], cur_ave[2], cur_ave[3],
		ref_ave[0], ref_ave[1], ref_ave[2], ref_ave[3]);

	if (vepu_pp_get_blk8_move_flag(pre_move, x, y)) {
		for (idx = 0; idx < 4; idx++) {
			if (ref_ave[idx] > cur_ave[idx] &&
			    ref_ave[idx] < cur_ave[idx] + info->param.thd_md_vpp.thres_dust_chng_md)
				sum_luma_chg++;
		}
	} else {
		for (idx = 0; idx < 4; idx++) {
			if (cur_ave[idx] > ref_ave[idx] &&
			    (cur_ave[idx] < ref_ave[idx] + info->param.thd_md_vpp.thres_dust_chng_md))
				sum_luma_chg++;
		}
	}

	pp_info("frame %d pos(%d, %d) sum_luma_chg %d luma_chg %d\n",
		info->frm_num, x * 32, y * 32, sum_luma_chg, info->param.thd_md_vpp.thres_dust_chng_md);

	return (sum_luma_chg >= info->param.thd_md_vpp.thres_dust_blk_md);
}

static void vepu_pp_flycatkin_filter(struct pp_chn_info_t *info)
{
	if (!info->buf_rfmwr)
		vepu_pp_flycatkin_filter_init_buf(info);

	if (info->buf_rfmwr) {
		int b8_col = PP_ALIGN(info->width, 32) / 32;
		int b8_row = PP_ALIGN(info->height, 32) / 32;
		int c, r, k;
		int vld_cnt, dust_flg, move_sum;
		struct vcodec_mpibuf_fn *func = get_vmpibuf_func();
		u8 *mdw_buf = func->buf_map(info->mdw_buf);
		u8 *mdw_ptr = NULL, *mdw_flt;

		if (info->frm_num % 2 == 0)
			dma_buf_begin_cpu_access(info->buf_rfpw->buf_dma, DMA_FROM_DEVICE);
		else
			dma_buf_begin_cpu_access(info->buf_rfpr->buf_dma, DMA_FROM_DEVICE);

		dma_buf_begin_cpu_access(func->buf_get_dmabuf(info->mdw_buf), DMA_FROM_DEVICE);
		memcpy(info->buf_rfmwr2, mdw_buf, info->mdw_len);

		for (r = 0; r < b8_row; r++) {
			mdw_ptr = mdw_buf + MDW_STRIDE * r;
			mdw_flt = info->buf_rfmwr2 + MDW_STRIDE * r;

			for (c = 0; c < b8_col; c += 8) {
				for (k = 0; k < 8; k++) {
					dust_flg = 0;

					if (((c + k) * 32 < info->width) && (r * 32 < info->height) &&
					    (*mdw_ptr >> k) & 1) {
						if (vepu_pp_get_blk8_move_flag(info->buf_rfmwr0, c + k, r) &&
						    vepu_pp_get_blk8_move_flag(info->buf_rfmwr1, c + k, r))
							dust_flg = 0;
						else {
							move_sum = vepu_pp_get_blk8_move_cnt(info, c + k, r, &vld_cnt);

							if (move_sum <= info->param.thd_md_vpp.thres_dust_move_md * vld_cnt / 8)
								dust_flg = vepu_pp_update_dust_flag(info, c + k, r);
						}

						if (dust_flg) {
							*mdw_flt = *mdw_flt & (~(1 << k));
							pp_info("frame %d pos(%d, %d) fly-catkin block\n",
								info->frm_num, (c + k) * 32, r * 32);
						}
					}
				}
				mdw_ptr++;
				mdw_flt++;
			}
		}

		memcpy((info->frm_num % 2 == 0) ? info->buf_rfmwr0 : info->buf_rfmwr1,
		       mdw_buf, info->mdw_len);
		memcpy(mdw_buf, info->buf_rfmwr2, info->mdw_len);
	}
}

int vepu_pp_control(int chn, enum pp_cmd cmd, void *param)
{
	struct pp_chn_info_t *info = NULL;

	if (chn >= MAX_CHN_NUM) {
		pp_err("vepu pp control channel id %d error\n", chn);
		return VEPU_PP_NOK;
	}

	info = &g_pp_ctx.chn_info[chn];

	if (cmd == PP_CMD_SET_COMMON_CFG || cmd == PP_CMD_SET_MD_CFG ||
	    cmd == PP_CMD_SET_OD_CFG || cmd == PP_CMD_SET_SMEAR_CFG ||
	    cmd == PP_CMD_SET_WEIGHTP_CFG || cmd == PP_CMD_SET_CHANNEL_INFO)
		vepu_pp_set_param(info, cmd, param);

	if (cmd == PP_CMD_RUN_SYNC) {
		struct dev_reg_wr_t reg_wr;
		struct dev_reg_rd_t reg_rd;

		reg_wr.data_ptr = &info->param;
		reg_wr.size = sizeof(info->param);
		reg_wr.offset = 0;

		if (info->api->reg_wr)
			info->api->reg_wr(info->dev_srv, &reg_wr);

		reg_rd.data_ptr = &info->output;
		reg_rd.size = sizeof(info->output);
		reg_rd.offset = 0;

		if (info->api->reg_rd)
			info->api->reg_rd(info->dev_srv, &reg_rd);

		if (info->api->cmd_send)
			info->api->cmd_send(info->dev_srv);
		if (info->api->cmd_poll)
			info->api->cmd_poll(info->dev_srv);

		if (info->md_en && info->flycatkin_en && info->mdw_buf)
			vepu_pp_flycatkin_filter(info);
	}

	if (cmd == PP_CMD_GET_OD_OUTPUT) {
		struct pp_od_out *out = (struct pp_od_out *)param;

		out->pix_sum = info->output.od_out_pix_sum;
	}

	pr_debug("vepu pp control cmd 0x%08x finished\n", cmd);

	return 0;
}
EXPORT_SYMBOL(vepu_pp_control);

static void vepu_pp_show_chn_attr(struct seq_file *seq)
{
	struct pp_chn_info_t *info = NULL;
	int i;

	seq_puts(seq,
		 "\n-----------------------------------ivs channel attr---------------------------------------------\n");
	seq_printf(seq, "%8s|%8s|%8s|%12s|%12s|%8s|%8s\n",
		   "ID", "width", "height", "max_width", "max_height", "md_en", "od_en");
	for (i = 0; i < MAX_CHN_NUM; i++) {
		info = &g_pp_ctx.chn_info[i];
		if (info->buf_rfpw == NULL || info->buf_rfpr == NULL)
			continue;

		seq_printf(seq, "%8d|%8d|%8d|%12d|%12d|%8d|%8d\n",
			   info->chn, info->width, info->height, info->max_width, info->max_height,
			   info->md_en, info->od_en);
	}
}

static void vepu_pp_show_com_cfg(struct seq_file *seq)
{
	struct pp_chn_info_t *info = NULL;
	struct pp_param_t *p = NULL;
	int i;

	seq_puts(seq,
		 "\n-----------------------------------ivs common cfg-----------------------------------------------\n");
	seq_printf(seq, "%8s|%8s|%15s|%15s\n",
		   "ID", "fmt", "frm_acc_intval", "frm_acc_gop");
	for (i = 0; i < MAX_CHN_NUM; i++) {
		info = &g_pp_ctx.chn_info[i];
		if (info->buf_rfpw == NULL || info->buf_rfpr == NULL)
			continue;
		p = &info->param;
		seq_printf(seq, "%8d|%8d|%15d|%15d\n",
			   info->chn, p->src_fmt.src_cfmt,
			   info->frm_accum_interval, info->frm_accum_gop);
	}
}

static void vepu_pp_show_md_cfg(struct seq_file *seq)
{
	struct pp_chn_info_t *info = NULL;
	struct pp_param_t *p = NULL;
	int i;

	seq_puts(seq,
		 "\n-----------------------------------ivs md cfg---------------------------------------------------\n");
	seq_printf(seq, "%8s|%10s|%10s|%10s|%10s|%10s|%15s|%15s|%15s\n",
		   "ID", "switch_sad", "thres_sad", "thres_move", "night_mode",
		   "filter_sw", "thres_dust_mv", "thres_dust_blk", "thres_dust_chng");
	for (i = 0; i < MAX_CHN_NUM; i++) {
		info = &g_pp_ctx.chn_info[i];
		if (info->buf_rfpw == NULL || info->buf_rfpr == NULL)
			continue;
		p = &info->param;
		seq_printf(seq, "%8d|%10d|%10d|%10d|%10d|%10d|%15d|%15d|%15d\n",
			   info->chn, p->vpp_base_cfg.switch_sad_md, p->thd_md_vpp.thres_sad_md,
			   p->thd_md_vpp.thres_move_md, p->vpp_base_cfg.night_mode_en_md,
			   info->flycatkin_en, p->thd_md_vpp.thres_dust_move_md,
			   p->thd_md_vpp.thres_dust_blk_md, p->thd_md_vpp.thres_dust_chng_md);
	}
}

static void vepu_pp_show_od_cfg(struct seq_file *seq)
{
	struct pp_chn_info_t *info = NULL;
	struct pp_param_t *p = NULL;
	int i;

	seq_puts(seq,
		 "\n-----------------------------------ivs od cfg---------------------------------------------------\n");
	seq_printf(seq, "%8s|%15s|%18s|%10s\n",
		   "ID", "thres_complex", "thres_complex_cnt", "thres_sad");
	for (i = 0; i < MAX_CHN_NUM; i++) {
		info = &g_pp_ctx.chn_info[i];
		if (info->buf_rfpw == NULL || info->buf_rfpr == NULL)
			continue;
		p = &info->param;
		seq_printf(seq, "%8d|%15d|%18d|%10d\n",
			   info->chn, p->thd_od_vpp.thres_complex_od,
			   p->thd_od_vpp.thres_complex_cnt_od, p->thd_od_vpp.thres_sad_od);
	}
}

void vepu_show_pp_info(struct seq_file *seq)
{
	vepu_pp_show_chn_attr(seq);
	vepu_pp_show_com_cfg(seq);
	vepu_pp_show_md_cfg(seq);
	vepu_pp_show_od_cfg(seq);
}

#if 0
#ifndef BUILD_ONE_KO
static int __init vepu_pp_init(void)
{
	pr_info("vepu_pp init\n");
	return 0;
}

static void __exit vepu_pp_exit(void)
{
	pr_info("vepu_pp exit\n");
}

module_init(vepu_pp_init);
module_exit(vepu_pp_exit);

MODULE_LICENSE("GPL");
#endif
#endif
