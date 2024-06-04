// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd
 *
 *
 */
#define MODULE_TAG  "vepu500_common"

#include <linux/string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_maths.h"
#include "jpege_syntax.h"
#include "vepu500_common.h"
#include "hal_enc_task.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"

MPP_RET vepu500_set_roi(Vepu500RoiCfg *roi_reg_base, MppEncROICfg * roi, RK_S32 w, RK_S32 h)
{
	MppEncROIRegion *region = roi->regions;
	Vepu500RoiCfg  *roi_cfg = (Vepu500RoiCfg *)roi_reg_base;
	Vepu500RoiRegion *reg_regions = &roi_cfg->regions[0];
	MPP_RET ret = MPP_NOK;
	RK_S32 i = 0;

	memset(reg_regions, 0, sizeof(Vepu500RoiRegion) * 8);
	if (NULL == roi_cfg || NULL == roi) {
		mpp_err_f("invalid buf %p roi %p\n", roi_cfg, roi);
		goto DONE;
	}

	if (roi->number > VEPU500_MAX_ROI_NUM) {
		mpp_err_f("invalid region number %d\n", roi->number);
		goto DONE;
	}

	/* check region config */
	ret = MPP_OK;
	for (i = 0; i < (RK_S32) roi->number; i++, region++) {
		if (region->x + region->w > w || region->y + region->h > h)
			ret = MPP_NOK;

		if (region->intra > 1 || region->qp_area_idx >= VEPU500_MAX_ROI_NUM ||
		    region->area_map_en > 1 || region->abs_qp_en > 1)
			ret = MPP_NOK;

		if ((region->abs_qp_en && region->quality > 51) ||
		    (!region->abs_qp_en && (region->quality > 51 || region->quality < -51)))
			ret = MPP_NOK;

		if (ret) {
			mpp_err_f("region %d invalid param:\n", i);
			mpp_err_f("position [%d:%d:%d:%d] vs [%d:%d]\n",
				  region->x, region->y, region->w, region->h, w, h);
			mpp_err_f("force intra %d qp area index %d\n",
				  region->intra, region->qp_area_idx);
			mpp_err_f("abs qp mode %d value %d\n",
				  region->abs_qp_en, region->quality);
			goto DONE;
		}

		reg_regions->roi_pos_lt.roi_lt_x = MPP_ALIGN(region->x, 16) >> 4;
		reg_regions->roi_pos_lt.roi_lt_y = MPP_ALIGN(region->y, 16) >> 4;
		reg_regions->roi_pos_rb.roi_rb_x = MPP_ALIGN(region->x + region->w, 16) >> 4;
		reg_regions->roi_pos_rb.roi_rb_y = MPP_ALIGN(region->y + region->h, 16) >> 4;
		reg_regions->roi_base.roi_qp_value = region->quality;
		reg_regions->roi_base.roi_qp_adj_mode = region->abs_qp_en;
		reg_regions->roi_base.roi_en = 1;
		reg_regions->roi_base.roi_pri = 0x1f;
		if (region->intra) {
			reg_regions->roi_mdc.roi_mdc_intra16 = 1;
			reg_regions->roi_mdc.roi0_mdc_intra32_hevc = 1;
		}
		reg_regions++;
	}

DONE:
	return ret;
}

static MPP_RET vepu500_jpeg_set_uv_offset(Vepu500JpegFrame *regs, JpegeSyntax * syn,
					  VepuFmt input_fmt, HalEncTask * task)
{
	RK_U32 hor_stride = mpp_frame_get_hor_stride(task->frame) ?
			    mpp_frame_get_hor_stride(task->frame) :
			    mpp_frame_get_width(task->frame);
	RK_U32 ver_stride = mpp_frame_get_ver_stride(task->frame) ?
			    mpp_frame_get_ver_stride(task->frame) :
			    mpp_frame_get_height(task->frame);
	RK_U32 frame_size = hor_stride * ver_stride;
	RK_U32 u_offset = 0, v_offset = 0;
	MPP_RET ret = MPP_OK;

	if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
		u_offset = mpp_frame_get_fbc_offset(task->frame);
		v_offset = 0;
		mpp_log("fbc case u_offset = %d", u_offset);
	} else {
		switch (input_fmt) {
		case VEPU541_FMT_YUV420P: {
			u_offset = frame_size;
			v_offset = frame_size * 5 / 4;
		}
		break;
		case VEPU541_FMT_YUV420SP:
		case VEPU541_FMT_YUV422SP: {
			u_offset = frame_size;
			v_offset = frame_size;
		}
		break;
		case VEPU541_FMT_YUV422P: {
			u_offset = frame_size;
			v_offset = frame_size * 3 / 2;
		}
		break;
		case VEPU541_FMT_YUYV422:
		case VEPU541_FMT_UYVY422: {
			u_offset = 0;
			v_offset = 0;
		}
		break;
		case VEPU541_FMT_BGR565:
		case VEPU541_FMT_BGR888:
		case VEPU541_FMT_BGRA8888: {
			u_offset = 0;
			v_offset = 0;
		}
		break;
		default: {
			mpp_err("unknown color space: %d\n", input_fmt);
			u_offset = frame_size;
			v_offset = frame_size * 5 / 4;
		}
		}
	}

	/* input cb addr */
	if (u_offset)
		regs->adr_src1 += u_offset;
	/* input cr addr */
	if (v_offset)
		regs->adr_src2 += v_offset;

	return ret;
}

MPP_RET vepu500_set_jpeg_reg(Vepu500JpegCfg * cfg)
{
	HalEncTask *task = (HalEncTask *) cfg->enc_task;
	JpegeSyntax *syn = (JpegeSyntax *) task->syntax.data;
	Vepu500JpegFrame *regs = (Vepu500JpegFrame *)cfg->jpeg_reg_base;
	VepuFmtCfg *fmt = (VepuFmtCfg *) cfg->input_fmt;
	RK_S32 stridey = 0;
	RK_S32 stridec = 0;
	MppFrame frame = task->frame;
	RK_U32 width = syn->width;
	RK_U32 height = syn->height;
	RK_U32 slice_en = (mpp_frame_get_height(frame) < height) && syn->restart_ri;
	RK_U32 pkt_len = mpp_packet_get_length(task->packet);

	if (slice_en) {
		width = mpp_frame_get_width(frame);
		height = mpp_frame_get_height(frame);
		/* if not first marker, do not write header */
		if (cfg->rst_marker) {
			task->length = 0;
			mpp_packet_set_length(task->packet, 0);
		}
	}

	if (!cfg->online) {
		regs->adr_src0 = mpp_dev_get_iova_address(cfg->dev, task->input, 264);
		regs->adr_src1 = regs->adr_src0;
		regs->adr_src2 = regs->adr_src0;
		vepu500_jpeg_set_uv_offset(regs, syn, (VepuFmt)fmt->format, task);
	}

	if (!task->output->cir_flag) {
		if (task->output->buf) {
			RK_U32 out_addr = mpp_dev_get_iova_address(cfg->dev, task->output->buf, 257);

			regs->adr_bsbb = out_addr + task->output->start_offset;
		} else
			regs->adr_bsbb = task->output->mpi_buf_id + task->output->start_offset;

		regs->adr_bsbt = regs->adr_bsbb + task->output->size - 1;
		regs->adr_bsbr = regs->adr_bsbb;
		regs->adr_bsbs = regs->adr_bsbb + pkt_len;
	} else {
		RK_U32 size = mpp_buffer_get_size(task->output->buf);
		RK_U32 start_off = (task->output->start_offset + pkt_len) % size;

		regs->adr_bsbb = mpp_dev_get_iova_address(cfg->dev, task->output->buf, 257);
		regs->adr_bsbs = regs->adr_bsbb + start_off;
		regs->adr_bsbr = regs->adr_bsbb + task->output->r_pos;
		regs->adr_bsbt = regs->adr_bsbb + size;
	}

	regs->enc_rsl.pic_wd8_m1 = MPP_ALIGN(width, 16) / 8 - 1;
	regs->src_fill.pic_wfill = MPP_ALIGN(width, 16) - width;
	regs->enc_rsl.pic_hd8_m1 = MPP_ALIGN(height, 16) / 8 - 1;
	regs->src_fill.pic_hfill = MPP_ALIGN(height, 16) - height;

	regs->src_fmt.src_cfmt = fmt->format;
	regs->src_fmt.alpha_swap = fmt->alpha_swap;
	regs->src_fmt.rbuv_swap = fmt->rbuv_swap;
	regs->src_fmt.src_range_trns_en = 0;
	regs->src_fmt.src_range_trns_sel = 0;
	regs->src_fmt.chroma_ds_mode = 0;

	regs->src_proc.src_mirr = syn->mirroring > 0;
	regs->src_proc.src_rot = syn->rotation;

	if (syn->hor_stride)
		stridey = syn->hor_stride;

	else {
		if (regs->src_fmt.src_cfmt == VEPU541_FMT_BGRA8888)
			stridey = syn->width * 4;
		else if (regs->src_fmt.src_cfmt == VEPU541_FMT_BGR888)
			stridey = syn->width * 3;
		else if (regs->src_fmt.src_cfmt == VEPU541_FMT_BGR565 ||
			 regs->src_fmt.src_cfmt == VEPU541_FMT_YUYV422
			 || regs->src_fmt.src_cfmt ==
			 VEPU541_FMT_UYVY422)
			stridey = syn->width * 2;
	}

	stridec = (regs->src_fmt.src_cfmt == VEPU541_FMT_YUV422SP ||
		   regs->src_fmt.src_cfmt == VEPU541_FMT_YUV420SP) ?
		  stridey : stridey / 2;

	if (regs->src_fmt.src_cfmt < VEPU541_FMT_ARGB1555) {
		regs->src_udfy.csc_wgt_r2y = 77;
		regs->src_udfy.csc_wgt_g2y = 150;
		regs->src_udfy.csc_wgt_b2y = 29;

		regs->src_udfu.csc_wgt_r2u = -43;
		regs->src_udfu.csc_wgt_g2u = -85;
		regs->src_udfu.csc_wgt_b2u = 128;

		regs->src_udfv.csc_wgt_r2v = 128;
		regs->src_udfv.csc_wgt_g2v = -107;
		regs->src_udfv.csc_wgt_b2v = -21;

		regs->src_udfo.csc_ofst_y = 0;
		regs->src_udfo.csc_ofst_u = 128;
		regs->src_udfo.csc_ofst_v = 128;
	}
	regs->src_strd0.src_strd0 = stridey;
	regs->src_strd1.src_strd1 = stridec;
	regs->pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
	regs->pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);

	regs->src_flt_cfg.pp_corner_filter_strength = 0;
	regs->src_flt_cfg.pp_edge_filter_strength = 0;
	regs->src_flt_cfg.pp_internal_filter_strength = 0;

	regs->y_cfg.bias_y = 0;
	regs->u_cfg.bias_u = 0;
	regs->v_cfg.bias_v = 0;

	regs->base_cfg.jpeg_ri = syn->restart_ri;
	regs->base_cfg.jpeg_out_mode = 0;
	regs->base_cfg.jpeg_start_rst_m = cfg->rst_marker & 0x7;
	if (slice_en) {
		if (mpp_frame_get_eos(frame)) {
			regs->base_cfg.jpeg_pic_last_ecs = 1;
			cfg->rst_marker = 0;
		} else {
			regs->base_cfg.jpeg_pic_last_ecs = 0;
			cfg->rst_marker++;
		}
	} else
		regs->base_cfg.jpeg_pic_last_ecs = 1;

	regs->base_cfg.jpeg_stnd = 1;	//enable

	regs->uvc_cfg.uvc_partition0_len = 0;
	regs->uvc_cfg.uvc_partition_len = 0;
	regs->uvc_cfg.uvc_skip_len = 0;

	return MPP_OK;
}
