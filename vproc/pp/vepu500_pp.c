// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 *
 *
 */

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>

#include "vepu500_pp.h"
#include "vepu_pp_api.h"
#include "vepu_pp_service_api.h"


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

	if (func->buf_alloc) {
		pp_buf->buf = func->buf_alloc(size);
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
	pp_free_buffer(info, info->buf_rfmwr);
	pp_free_buffer(info, info->buf_rfmrd);
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

	/* alloc buffer for md wr */
	buf_len = (64) * (h >> 5);
	info->buf_rfmwr = pp_malloc_buffer(info, buf_len);
	if (IS_ERR_OR_NULL(info->buf_rfmwr)) {
		pp_err("alloc buffer failed\n");
		ret = VEPU_PP_NOK;
		goto __return;
	}

	/* alloc buffer for md rd */
	buf_len = (64) * (h >> 5);
	info->buf_rfmrd = pp_malloc_buffer(info, buf_len);
	if (IS_ERR_OR_NULL(info->buf_rfmrd)) {
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

	return VEPU_PP_OK;
}
EXPORT_SYMBOL(vepu_pp_destroy_chn);

static void pp_set_src_addr(struct pp_chn_info_t *info, struct pp_com_cfg *cfg)
{
	struct pp_param_t *p = &info->param;
	u32 adr_src0, adr_src1, adr_src2;
	u32 width = info->width, height = info->height;
	struct vcodec_mpibuf_fn *func = get_vmpibuf_func();
	u32 stride_y, stride_c;

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
	p->vpp_base_cfg.ref_frm_en_md = p->vpp_base_cfg.cur_frm_en_md && (frm_cnt > interval);

	p->vpp_base_cfg.en_od = info->od_en && md_od_switch;
	p->vpp_base_cfg.background_en_od = (frm_cnt > 0);
	p->vpp_base_cfg.sad_comp_en_od = 1;
	p->vpp_base_cfg.vepu_pp_en = 1;

	/* cime cfg */
	p->me_rnge.cime_srch_dwnh = 15;
	p->me_rnge.cime_srch_uph  = 15;
	p->me_rnge.cime_srch_rgtw = 12;
	p->me_rnge.cime_srch_lftw = 12;
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
		p->vpp_base_cfg.night_mode_en_md = cfg->night_mode;
		p->vpp_base_cfg.flycatkin_flt_en_md = cfg->filter_switch;

		p->thd_md_vpp.thres_sad_md = cfg->thres_sad;
		p->thd_md_vpp.thres_move_md = cfg->thres_move;
		p->thd_md_vpp.thres_dust_move_md = cfg->thres_dust_move;
		p->thd_md_vpp.thres_dust_blk_md = cfg->thres_dust_blk;
		p->thd_md_vpp.thres_dust_chng_md = cfg->thres_dust_chng;
		p->vpp_base_cfg.sto_stride_md = 4;//PP_ALIGN(info->width, 32);
		if (info->frm_accum_gop % 2) {
			p->adr_ref_mdw = info->buf_rfmwr->iova;
			p->adr_ref_mdr = info->buf_rfmrd->iova;
		} else {
			p->adr_ref_mdr = info->buf_rfmwr->iova;
			p->adr_ref_mdw = info->buf_rfmrd->iova;
		}

		if (func->buf_get_paddr)
			p->adr_md_vpp = func->buf_get_paddr(cfg->mdw_buf);
	} break;
	case PP_CMD_SET_OD_CFG: {
		struct pp_od_cfg *cfg = (struct pp_od_cfg *)param;
		struct vcodec_mpibuf_fn *func = get_vmpibuf_func();

		p->vpp_base_cfg.sto_stride_od = 4;//PP_ALIGN(info->width, 32);
		p->thd_od_vpp.thres_complex_od = cfg->thres_complex;
		// TODO: may be need add to user config
		p->thd_od_vpp.thres_complex_cnt_od = 1;
		p->thd_od_vpp.thres_sad_od = 7;
		if (func->buf_get_paddr)
			p->adr_od_vpp = func->buf_get_paddr(cfg->odw_buf);
	} break;
	default: {
	}
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
	}

	if (cmd == PP_CMD_GET_OD_OUTPUT) {
		struct pp_od_out *out = (struct pp_od_out *)param;

		out->pix_sum = info->output.od_out_pix_sum;
	}

	pr_debug("vepu pp control cmd 0x%08x finished\n", cmd);

	return 0;
}
EXPORT_SYMBOL(vepu_pp_control);

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
