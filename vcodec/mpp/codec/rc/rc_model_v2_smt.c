/*
 * Copyright 2016 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License,  Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef USE_SMART_RC
#define MODULE_TAG "rc_model_v2_smt"
#include <linux/string.h>
#include <linux/seq_file.h>
#include <linux/math64.h>

#include "mpp_mem.h"
#include "mpp_maths.h"
#include "mpp_2str.h"
#include "mpp_rc.h"
#include "rc_debug.h"
#include "rc_ctx.h"
#include "rc_model_v2.h"
#include "kmpp_frame.h"

#define LOW_QP 34
#define LOW_LOW_QP 35
#define LIGHT_STAT_LEN     (8)

typedef struct RcModelV2SmtCtx_t {
	RcCfg           usr_cfg;
	RK_U32          frame_type;
	RK_U32          last_frame_type;
	RK_U32          first_frm_flg;
	RK_S64          frm_num;
	RK_S32          qp_min;
	RK_S32          qp_max;
	MppEncGopMode   gop_mode;
	RK_S64          acc_intra_count;
	RK_S64          acc_inter_count;
	RK_S32          last_fps_bits;
	RK_S32          pre_gop_left_bit;
	MppData         *qp_p;
	MppDataV2       *motion_level;
	MppDataV2       *complex_level;
	MppDataV2       *stat_bits;
	MppDataV2       *stat_luma_ave;
	MppDataV2       *lgt_chg_flg; /* light change flag */
	MppPIDCtx       pid_fps;
	RK_S32          bits_target_lr;  //bits_target_low_rate
	RK_S32          bits_target_hr;  //bits_target_high_rate
	RK_S32          bits_per_ilr;    //bits_per_intra_low_rate
	RK_S32          bits_per_plr;    //bits_per_inter_low_rate
	RK_S32          bits_per_ihr;    //bits_per_intra_high_rate
	RK_S32          bits_per_phr;    //bits_per_inter_high_rate
	RK_S32          pre_diff_bit_lr; //pre_diff_bit_low_rate
	RK_S32          pre_diff_bit_hr; //pre_diff_bit_high_rate
	RK_S32          igop;
	MppPIDCtx       pid_ilr;         //pid_intra_low_rate
	MppPIDCtx       pid_ihr;         //pid_intra_high_rate
	MppPIDCtx       pid_alr;         //pid_allframes_low_rate
	MppPIDCtx       pid_ahr;         //pid_allframes_high_rate
	MppDataV2       *pid_plr;
	MppDataV2       *pid_phr;
	RK_S32          qp_out;
	RK_S32          qp_prev_out;
	RK_S32          pre_real_bit_i; /* real bit of last intra frame */
	RK_S32          pre_qp_i; /* qp of last intra frame */
	RK_S32          gop_qp_sum;
	RK_S32          gop_frm_cnt;
	RK_S32          pre_iblk4_prop;
	RK_S32          reenc_cnt;
	RK_U32          drop_cnt;
	RK_S32          on_drop;
	RK_S32          on_pskip;
	RK_S32          ptz_keep_cnt;
	RK_S32          qp_add;
	RK_S32          cur_lgt_chg_flg;
} RcModelV2SmtCtx;

MPP_RET bits_model_smt_deinit(RcModelV2SmtCtx * ctx)
{
	rc_dbg_func("enter %p\n", ctx);

	if (ctx->qp_p) {
		mpp_data_deinit(ctx->qp_p);
		ctx->qp_p = NULL;
	}

	if (ctx->motion_level != NULL) {
		mpp_data_deinit_v2(ctx->motion_level);
		ctx->motion_level = NULL;
	}

	if (ctx->complex_level != NULL) {
		mpp_data_deinit_v2(ctx->complex_level);
		ctx->complex_level = NULL;
	}

	if (ctx->stat_bits != NULL) {
		mpp_data_deinit_v2(ctx->stat_bits);
		ctx->stat_bits = NULL;
	}

	if (ctx->pid_plr != NULL) {
		mpp_data_deinit_v2(ctx->pid_plr);
		ctx->pid_plr = NULL;
	}

	if (ctx->pid_phr != NULL) {
		mpp_data_deinit_v2(ctx->pid_phr);
		ctx->pid_phr = NULL;
	}

	if (ctx->stat_luma_ave != NULL) {
		mpp_data_deinit_v2(ctx->stat_luma_ave);
		ctx->stat_luma_ave = NULL;
	}

	if (ctx->lgt_chg_flg) {
		mpp_data_deinit_v2(ctx->lgt_chg_flg);
		ctx->lgt_chg_flg = NULL;
	}

	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

MPP_RET bits_model_smt_init(RcModelV2SmtCtx * ctx)
{
	RK_S32 gop_len = ctx->usr_cfg.igop;
	RcFpsCfg *fps = &ctx->usr_cfg.fps;
	RK_S32 mad_len = 10;
	RK_S32 avg_lr = 0;
	RK_S32 avg_hr = 0;
	RK_S32 bit_ratio[5] = {7, 8, 9, 10, 11};
	RK_S32 nfps = fps->fps_out_num / fps->fps_out_denom;
	RK_S32 win_len = mpp_clip(MPP_MAX3(gop_len, nfps, 10), 1, nfps);
	RK_S32 stat_len = fps->fps_out_num * ctx->usr_cfg.stats_time / fps->fps_out_denom;
	stat_len = stat_len ? stat_len : (fps->fps_out_num * 8 / fps->fps_out_denom);

	rc_dbg_func("enter %p\n", ctx);
	ctx->frm_num = 0;
	ctx->first_frm_flg = 1;
	ctx->gop_frm_cnt = 0;
	ctx->gop_qp_sum = 0;

	// smt
	ctx->igop = gop_len;
	ctx->qp_min = 10;
	ctx->qp_max = 51;

	bits_model_smt_deinit(ctx);

	mpp_data_init_v2(&ctx->motion_level, mad_len, 0);
	if (ctx->motion_level == NULL) {
		mpp_err("motion_level init fail");
		return MPP_ERR_MALLOC;
	}

	mpp_data_init_v2(&ctx->complex_level, mad_len, 0);
	if (ctx->complex_level == NULL) {
		mpp_err("complex_level init fail");
		return MPP_ERR_MALLOC;
	}

	mpp_data_init_v2(&ctx->stat_bits, stat_len, ctx->bits_per_phr);
	if (ctx->stat_bits == NULL) {
		mpp_err("stat_bits init fail");
		return MPP_ERR_MALLOC;
	}

	mpp_data_init_v2(&ctx->pid_plr, stat_len, 0);
	if (ctx->pid_plr == NULL) {
		mpp_err("pid_plr init fail");
		return MPP_ERR_MALLOC;
	}

	mpp_data_init_v2(&ctx->pid_phr, stat_len, 0);
	if (ctx->pid_phr == NULL) {
		mpp_err("pid_phr init fail");
		return MPP_ERR_MALLOC;
	}

	{
		RK_S32 qp_stat_len = nfps < 15 ? 4 * nfps : (nfps < 25 ? 3 * nfps : 2 * nfps);

		mpp_data_init(&ctx->qp_p, mpp_clip(MPP_MAX(gop_len, qp_stat_len), 20, 50));
		if (ctx->qp_p == NULL) {
			mpp_err("qp_p init fail");
			return MPP_ERR_MALLOC;
		}
	}

	/* 3 frames for luma average */
	mpp_data_init_v2(&ctx->stat_luma_ave, 3, 0);
	if (ctx->stat_luma_ave == NULL) {
		mpp_err("stat_luma_ave init fail");
		return MPP_ERR_MALLOC;
	}

	mpp_data_init_v2(&ctx->lgt_chg_flg, LIGHT_STAT_LEN, 0);
	if  (!ctx->lgt_chg_flg) {
		mpp_err("lgt_chg_flg init fail");
		return MPP_ERR_MALLOC;
	}

	mpp_pid_reset(&ctx->pid_fps);
	mpp_pid_reset(&ctx->pid_ilr);
	mpp_pid_reset(&ctx->pid_ihr);
	mpp_pid_reset(&ctx->pid_alr);
	mpp_pid_reset(&ctx->pid_ahr);

	mpp_pid_set_param(&ctx->pid_fps, 4, 6, 0, 90, win_len);
	mpp_pid_set_param(&ctx->pid_ilr, 4, 6, 0, 100, win_len);
	mpp_pid_set_param(&ctx->pid_ihr, 4, 6, 0, 100, win_len);
	mpp_pid_set_param(&ctx->pid_alr, 4, 6, 0, 100, gop_len);
	mpp_pid_set_param(&ctx->pid_ahr, 4, 6, 0, 100, gop_len);

	avg_lr = axb_div_c(ctx->usr_cfg.bps_min, fps->fps_out_denom, fps->fps_out_num);
	avg_hr = axb_div_c(ctx->usr_cfg.bps_max, fps->fps_out_denom, fps->fps_out_num);
	ctx->acc_intra_count = 0;
	ctx->acc_inter_count = 0;
	ctx->last_fps_bits = 0;
	rc_dbg_rc("frame %lld avg_lr %d avg_hr %d win_len %d gop_len %d stat_len %d\n",
		  ctx->frm_num, avg_lr, avg_hr, win_len, gop_len, stat_len);

	if (gop_len == 0) {
		ctx->gop_mode = MPP_GOP_ALL_INTER;
		ctx->bits_per_plr = avg_lr;
		ctx->bits_per_ilr = avg_lr * 10;
		ctx->bits_per_phr = avg_hr;
		ctx->bits_per_ihr = avg_hr * 10;
	} else if (gop_len == 1) {
		ctx->gop_mode = MPP_GOP_ALL_INTRA;
		ctx->bits_per_plr = 0;
		ctx->bits_per_ilr = avg_lr;
		ctx->bits_per_phr = 0;
		ctx->bits_per_ihr = avg_hr;
		/* disable debreath on all intra case */
		if (ctx->usr_cfg.debreath_cfg.enable)
			ctx->usr_cfg.debreath_cfg.enable = 0;
	} else if (gop_len < win_len) {
		ctx->gop_mode = MPP_GOP_SMALL;
		ctx->bits_per_plr = avg_lr >> 1;
		ctx->bits_per_ilr = ctx->bits_per_plr * (gop_len + 1);
		ctx->bits_per_phr = avg_hr >> 1;
		ctx->bits_per_ihr = ctx->bits_per_phr * (gop_len + 1);
	} else {
		RK_S32 g = gop_len;
		RK_S32 idx = g <= 50 ? 0 : (g <= 100 ? 1 : (g <= 200 ? 2 : (g <= 300 ? 3 : 4)));

		ctx->gop_mode = MPP_GOP_LARGE;
		ctx->bits_per_ilr = avg_lr * bit_ratio[idx] / 2;
		ctx->bits_per_ihr = avg_hr * bit_ratio[idx] / 2;
		ctx->bits_per_plr = avg_lr - ctx->bits_per_ilr / (nfps - 1);
		ctx->bits_per_phr = avg_hr - ctx->bits_per_ihr / (nfps - 1);
	}

	rc_dbg_rc("frame %lld bits_per_ilr %d bits_per_plr %d "
		  "bits_per_ihr %d bits_per_phr %d\n",
		  ctx->frm_num, ctx->bits_per_ilr, ctx->bits_per_plr,
		  ctx->bits_per_ihr, ctx->bits_per_phr);

	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

MPP_RET bits_model_update_smt(RcModelV2SmtCtx * ctx, RK_S32 real_bit)
{
	RcFpsCfg *fps = &ctx->usr_cfg.fps;
	RK_S32 bps_target_tmp = 0;
	RK_S32 mod = 0;

	rc_dbg_func("enter %p\n", ctx);

	mpp_data_update_v2(ctx->stat_bits, real_bit);
	ctx->pre_diff_bit_lr = ctx->bits_target_lr - real_bit;
	ctx->pre_diff_bit_hr = ctx->bits_target_hr - real_bit;

	if (ctx->frame_type == INTRA_FRAME) {
		ctx->acc_intra_count++;
		mpp_pid_update(&ctx->pid_ilr, real_bit - ctx->bits_target_lr, 1);
		mpp_pid_update(&ctx->pid_ihr, real_bit - ctx->bits_target_hr, 1);
	} else {
		ctx->acc_inter_count++;
		mpp_data_update_v2(ctx->pid_plr, real_bit - ctx->bits_target_lr);
		mpp_data_update_v2(ctx->pid_phr, real_bit - ctx->bits_target_hr);
	}
	mpp_pid_update(&ctx->pid_alr, real_bit - ctx->bits_target_lr, 1);
	mpp_pid_update(&ctx->pid_ahr, real_bit - ctx->bits_target_hr, 1);

	ctx->last_fps_bits += real_bit;
	/* new fps start */
	mod = ctx->acc_intra_count + ctx->acc_inter_count;
	mod = mod % (fps->fps_out_num / fps->fps_out_denom);
	if (0 == mod) {
		bps_target_tmp = (ctx->usr_cfg.bps_min + ctx->usr_cfg.bps_max) >> 1;
		if (bps_target_tmp * 3 > (ctx->last_fps_bits * 2))
			mpp_pid_update(&ctx->pid_fps, bps_target_tmp - ctx->last_fps_bits, 0);
		else {
			bps_target_tmp = ctx->usr_cfg.bps_min * 4 / 10 + ctx->usr_cfg.bps_max * 6 / 10;
			mpp_pid_update(&ctx->pid_fps, bps_target_tmp - ctx->last_fps_bits, 0);
		}
		ctx->last_fps_bits = 0;
	}

	/* new frame start */
	ctx->qp_prev_out = ctx->qp_out;

	rc_dbg_func("leave %p\n", ctx);

	return MPP_OK;
}

MPP_RET rc_model_v2_smt_h265_init(void *ctx, RcCfg * cfg)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;

	rc_dbg_func("enter %p\n", ctx);

	memcpy(&p->usr_cfg, cfg, sizeof(RcCfg));
	bits_model_smt_init(p);

	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

MPP_RET rc_model_v2_smt_h264_init(void *ctx, RcCfg * cfg)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;

	rc_dbg_func("enter %p\n", ctx);

	memcpy(&p->usr_cfg, cfg, sizeof(RcCfg));
	bits_model_smt_init(p);

	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

MPP_RET rc_model_v2_smt_deinit(void *ctx)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	rc_dbg_func("enter %p\n", ctx);
	bits_model_smt_deinit(p);
	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

static void set_coef(void *ctx, RK_S32 *coef, RK_S32 val)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	RK_S32 cplx_lvl_0 = mpp_data_get_pre_val_v2(p->complex_level, 0);
	RK_S32 cplx_lvl_1 = mpp_data_get_pre_val_v2(p->complex_level, 1);
	RK_S32 cplx_lvl_sum = mpp_data_sum_v2(p->complex_level);

	if (cplx_lvl_sum == 0)
		*coef = val + 0;
	else if (cplx_lvl_sum == 1) {
		if (cplx_lvl_0 == 0)
			*coef = val + 10;
		else
			*coef = val + 25;
	} else if (cplx_lvl_sum == 2) {
		if (cplx_lvl_0 == 0)
			*coef = val + 25;
		else
			*coef = val + 35;
	} else if (cplx_lvl_sum == 3) {
		if (cplx_lvl_0 == 0)
			*coef = val + 35;
		else
			*coef = val + 51;
	} else if (cplx_lvl_sum >= 4 && cplx_lvl_sum <= 6) {
		if (cplx_lvl_0 == 0) {
			if (cplx_lvl_1 == 0)
				*coef = val + 35;
			else
				*coef = val + 51;
		} else
			*coef = val + 64;
	} else if (cplx_lvl_sum >= 7 && cplx_lvl_sum <= 9) {
		if (cplx_lvl_0 == 0) {
			if (cplx_lvl_1 == 0)
				*coef = val + 64;
			else
				*coef = val + 72;
		} else
			*coef = val + 72;
	} else
		*coef = val + 80;
}

static RK_U32 mb_num[9] = {
	0, 200, 700, 1200, 2000, 4000, 8000, 16000, 20000
};

static RK_U32 tab_bit[9] = {
	3780, 3570, 3150, 2940, 2730, 3780, 2100, 1680, 2100
};

static RK_U8 qscale2qp[96] = {
	15,  15,  15,  15,  15,  16, 18, 20, 21, 22, 23,
	24,  25,  25,  26,  27,  28, 28, 29, 29, 30, 30,
	30,  31,  31,  32,  32,  33, 33, 33, 34, 34, 34,
	34,  35,  35,  35,  36,  36, 36, 36, 36, 37, 37,
	37,  37,  38,  38,  38,  38, 38, 39, 39, 39, 39,
	39,  39,  40,  40,  40,  40, 41, 41, 41, 41, 41,
	41,  41,  42,  42,  42,  42, 42, 42, 42, 42, 43,
	43,  43,  43,  43,  43,  43, 43, 44, 44, 44, 44,
	44,  44,  44,  44,  45,  45, 45, 45,
};

static RK_U8 inter_pqp0[52] = {
	1,  1,  1,  1,  1,  2,  3,  4,
	5,  6,  7,  8,  9,  10,  11,  12,
	13,  14,  15,  17,  18,  19,  20,  21,
	21,  21,  22,  23,  24,  25,  26,  26,
	27,  28,  28,  29,  29,  29,  30,  31,
	31,  32,  32,  33,  33,  34,  35,  35,
	35,  36,  36,  36
};

static RK_U8 inter_pqp1[52] = {
	1,  1,  2,  3,  4,  5,  6,  7,
	8,  9,  10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 20, 21, 22,
	23, 24, 25, 26, 26, 27, 28, 29,
	29, 30, 31, 32, 33, 34, 35, 36,
	37, 38, 39, 40, 40, 41, 42, 42,
	42, 43, 43, 44
};

static RK_U8 intra_pqp0[3][52] = {
	{
		1,  1,  1,  2,  3,  4,  5,  6,
		7,  8,  9,  10, 11, 12, 13, 14,
		15, 16, 17, 18, 19, 20, 21, 22,
		23, 23, 24, 25, 26, 27, 27, 28,
		28, 29, 30, 31, 32, 32, 33, 34,
		34, 34, 35, 35, 36, 36, 36, 36,
		37, 37, 37, 38
	},

	{
		1,  1,  1,  2,  3,  4,  5,  6,
		7,  8,  9,  10, 11, 12, 13, 14,
		15, 16, 17, 17, 18, 18, 19, 19,
		20, 21, 22, 23, 24, 25, 26, 27,
		28, 29, 30, 31, 32, 32, 33, 34,
		34, 34, 35, 35, 36, 36, 36, 36,
		37, 37, 37, 38
	},

	{
		1,  1,  1,  2,  3,  4,  5,  6,
		7,  8,  9,  10, 11, 12, 13, 14,
		14, 15, 15, 16, 16, 17, 17, 18,
		16, 16, 16, 17, 18, 19, 20, 21,
		23, 24, 26, 28, 30, 31, 32, 33,
		34, 34, 35, 35, 36, 36, 36, 36,
		37, 37, 37, 38
	},
};

static RK_U8 intra_pqp1[52] = {
	2,  3,  4,  5,  6,  7,  8,  9,
	10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 22, 23, 24, 25, 26,
	27, 28, 29, 30, 31, 32, 33, 34,
	35, 36, 37, 38, 39, 40, 41, 42,
	43, 44, 45, 46, 47, 48, 49, 50,
	51, 51, 51, 51
};

static RK_S32 cal_smt_first_i_start_qp(RK_S32 target_bit, RK_U32 total_mb)
{
	RK_S32 cnt = 0;
	RK_S32 index;
	RK_S32 i;

	for (i = 0; i < 8; i++) {
		if (mb_num[i] > total_mb)
			break;
		cnt++;
	}

	index = (total_mb * tab_bit[cnt] - 350) / target_bit; // qscale
	index = mpp_clip(index, 4, 95);

	return qscale2qp[index];
}

static MPP_RET calc_smt_debreath_qp(RcModelV2SmtCtx * ctx)
{
	RK_S32 fm_qp_sum = 0;
	RK_S32 new_fm_qp = 0;
	RcDebreathCfg *debreath_cfg = &ctx->usr_cfg.debreath_cfg;
	RK_S32 dealt_qp = 0;
	RK_S32 gop_qp_sum = ctx->gop_qp_sum;
	RK_S32 gop_frm_cnt = ctx->gop_frm_cnt;
	static RK_S8 intra_qp_map[8] = {
		0, 0, 1, 1, 2, 2, 2, 2,
	};
	RK_U8 idx2 = MPP_MIN(ctx->pre_iblk4_prop >> 5, (RK_S32)sizeof(intra_qp_map) - 1);

	static RK_S8 strength_map[36] = {
		0, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4,
		5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8,
		9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 12
	};

	rc_dbg_func("enter %p\n", ctx);

	fm_qp_sum = MPP_MIN(gop_qp_sum / gop_frm_cnt, (RK_S32)sizeof(strength_map) - 1);

	rc_dbg_qp("i qp_out %d, qp_start_sum = %d, intra_lv4_prop %d",
		  ctx->qp_out, fm_qp_sum, ctx->pre_iblk4_prop);

	dealt_qp = strength_map[debreath_cfg->strength] - intra_qp_map[idx2];
	if (fm_qp_sum > dealt_qp)
		new_fm_qp = fm_qp_sum - dealt_qp;
	else
		new_fm_qp = fm_qp_sum;

	ctx->qp_out = mpp_clip(new_fm_qp, ctx->usr_cfg.min_i_quality, ctx->usr_cfg.max_i_quality);
	ctx->gop_frm_cnt = 0;
	ctx->gop_qp_sum = 0;
	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

static MPP_RET smt_start_prepare(void *ctx, EncRcTask *task, RK_S32 *fm_min_iqp,
				 RK_S32 *fm_min_pqp, RK_S32 *fm_max_iqp, RK_S32 *fm_max_pqp)
{
	VepuPpInfo *ppinfo = NULL;
	EncFrmStatus *frm = &task->frm;
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	EncRcTaskInfo *info = &task->info;
	RcFpsCfg *fps = &p->usr_cfg.fps;
	RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
	RK_S32 b_min = p->usr_cfg.bps_min;
	RK_S32 b_max = p->usr_cfg.bps_max;

	if (ppinfo) {
		RK_S32 wp_en = ppinfo->wp_out_par_y & 0x1;
		RK_S32 wp_weight = (ppinfo->wp_out_par_y >> 4) & 0x1FF;
		RK_S32 wp_offset = (ppinfo->wp_out_par_y >> 16) & 0xFF;
		if (wp_en) {
			if (abs(wp_weight) > 1)
				*fm_min_pqp = 33;
			else if (abs(wp_weight) > 0)
				*fm_min_pqp = 32;
			else
				*fm_min_pqp = 30;

			if (abs(wp_offset) > 1)
				*fm_min_pqp = 33;
			else if (abs(wp_offset) > 0)
				*fm_min_pqp = 32;
			else
				*fm_min_pqp = 30;

			if ((abs(wp_weight) > 0 || abs(wp_offset) > 0) && *fm_max_pqp > 37)
				*fm_max_pqp = 37;
		}
	}

	if (0 == mpp_data_sum_v2(p->motion_level) && p->usr_cfg.motion_static_switch_enable) {
		*fm_min_iqp = p->usr_cfg.mt_st_swth_frm_qp;
		*fm_min_pqp = p->usr_cfg.mt_st_swth_frm_qp;
	}

	if (MPP_ENC_SCENE_MODE_IPC_PTZ == info->scene_mode) {
		if (mpp_data_sum_v2(p->complex_level) <= 15) {
			if (*fm_max_iqp > 34)
				*fm_max_iqp = 34;
			if (*fm_max_pqp > 37)
				*fm_max_pqp = 37;
		} else {
			if (*fm_max_iqp > 35)
				*fm_max_iqp = 35;
			if (*fm_max_pqp > 38)
				*fm_max_pqp = 38;
		}
	}

	p->frame_type = frm->is_intra ? INTRA_FRAME :
			(frm->ref_mode == REF_TO_PREV_INTRA) ? INTER_VI_FRAME : INTER_P_FRAME;

	if (p->ptz_keep_cnt || (MPP_ENC_SCENE_MODE_IPC == info->scene_mode &&
				MPP_ENC_SCENE_MODE_IPC_PTZ == info->last_scene_mode)) {
		if (p->frame_type != INTRA_FRAME) {
			RK_S32 fqp = 28;
			*fm_max_iqp = *fm_max_pqp = fqp;
			*fm_min_iqp = *fm_min_pqp = fqp;
			p->ptz_keep_cnt++;
		} else
			p->ptz_keep_cnt = 0;
	}

	switch (p->gop_mode) {
	case MPP_GOP_ALL_INTER: {
		if (p->frame_type == INTRA_FRAME) {
			p->bits_target_lr = p->bits_per_ilr;
			p->bits_target_hr = p->bits_per_ihr;
		} else {
			p->bits_target_lr = p->bits_per_plr - mpp_data_mean_v2(p->pid_plr);
			p->bits_target_hr = p->bits_per_phr - mpp_data_mean_v2(p->pid_phr);
		}
	}
	break;
	case MPP_GOP_ALL_INTRA: {
		p->bits_target_lr = p->bits_per_ilr - mpp_pid_calc(&p->pid_ilr);
		p->bits_target_hr = p->bits_per_ihr - mpp_pid_calc(&p->pid_ihr);
	}
	break;
	default: {
		if (p->frame_type == INTRA_FRAME) {
			RK_S32 diff_bit = mpp_pid_calc(&p->pid_fps);

			p->pre_gop_left_bit = p->pid_fps.i - diff_bit;
			mpp_pid_reset(&p->pid_fps);

			if (p->acc_intra_count) {
				p->bits_target_lr = (p->bits_per_ilr + diff_bit);
				p->bits_target_hr = (p->bits_per_ihr + diff_bit);
			} else {
				p->bits_target_lr = p->bits_per_ilr - mpp_pid_calc(&p->pid_ilr);
				p->bits_target_hr = p->bits_per_ihr - mpp_pid_calc(&p->pid_ihr);
			}

			rc_dbg_rc("frame %lld pre_gop_left_bit %d, diff_bit %d, pid %d %d\n",
				  p->frm_num, p->pre_gop_left_bit, diff_bit,
				  mpp_pid_calc(&p->pid_ilr), mpp_pid_calc(&p->pid_ihr));
		} else {
			if (p->last_frame_type == INTRA_FRAME) {
				RK_S32 bits_prev_i = p->pre_real_bit_i;

				p->bits_per_plr = ((RK_S32)div64_s64((RK_S64)b_min * p->igop, fps_out) -
									bits_prev_i + p->pre_gop_left_bit) / (p->igop - 1);
				p->bits_target_lr = p->bits_per_plr;
				p->bits_per_phr = ((RK_S32)div64_s64((RK_S64)b_max * p->igop, fps_out) -
									bits_prev_i + p->pre_gop_left_bit) / (p->igop - 1);
				p->bits_target_hr = p->bits_per_phr;
				rc_dbg_rc("frame %lld b_min %d b_max %d bits_prev_i %d\n",
					  p->frm_num, b_min, b_max, bits_prev_i);
			} else {
				RK_S32 diff_bit_lr = mpp_data_mean_v2(p->pid_plr);
				RK_S32 diff_bit_hr = mpp_data_mean_v2(p->pid_phr);
				RK_S32 lr = axb_div_c(b_min, fps->fps_out_denom, fps->fps_out_num);
				RK_S32 hr = axb_div_c(b_max, fps->fps_out_denom, fps->fps_out_num);

				p->bits_target_lr = MPP_MIN(p->bits_per_plr - diff_bit_lr, 2 * lr);
				p->bits_target_hr = MPP_MIN(p->bits_per_phr - diff_bit_hr, 2 * hr);
				rc_dbg_rc("frame %lld diff_bit %d %d lr %d hr %d\n",
					  p->frm_num, diff_bit_lr, diff_bit_hr, lr, hr);
			}
		}
	}
	break;
	}

	info->bit_max = MPP_MAX((p->bits_target_lr + p->bits_target_hr) / 2, 100);

	rc_dbg_rc("frame %lld bits_tgt_lower %d bits_tgt_upper %d bits_tgt_ave %d prev_frm_qp %d",
		  p->frm_num, p->bits_target_lr, p->bits_target_hr, info->bit_max, p->qp_out);

	return MPP_OK;
}

static RK_S32 smt_calc_coef(void *ctx)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	RK_S32 coef = 1024;
	RK_S32 coef2 = 512;
	RK_S32 md_lvl_sum = mpp_data_sum_v2(p->motion_level);
	RK_S32 md_lvl_0 = mpp_data_get_pre_val_v2(p->motion_level, 0);
	RK_S32 md_lvl_1 = mpp_data_get_pre_val_v2(p->motion_level, 1);

	if (md_lvl_sum < 100)
		set_coef(ctx, &coef, 0);
	else if (md_lvl_sum < 200) {
		if (md_lvl_0 < 100)
			set_coef(ctx, &coef, 102);
		else
			set_coef(ctx, &coef, 154);
	} else if (md_lvl_sum < 300) {
		if (md_lvl_0 < 100)
			set_coef(ctx, &coef, 154);
		else if (md_lvl_0 == 100) {
			if (md_lvl_1  < 100)
				set_coef(ctx, &coef, 205);
			else if (md_lvl_1 == 100)
				set_coef(ctx, &coef, 256);
			else
				set_coef(ctx, &coef, 307);
		} else
			set_coef(ctx, &coef, 307);
	} else if (md_lvl_sum < 600) {
		if (md_lvl_0 < 100) {
			if (md_lvl_1  < 100)
				set_coef(ctx, &coef, 307);
			else if (md_lvl_1 == 100)
				set_coef(ctx, &coef, 358);
			else
				set_coef(ctx, &coef, 410);
		} else if (md_lvl_0 == 100) {
			if (md_lvl_1  < 100)
				set_coef(ctx, &coef, 358);
			else if (md_lvl_1 == 100)
				set_coef(ctx, &coef, 410);
			else
				set_coef(ctx, &coef, 461);
		} else
			set_coef(ctx, &coef, 461);
	} else if (md_lvl_sum < 900) {
		if (md_lvl_0 < 100) {
			if (md_lvl_1 < 100)
				set_coef(ctx, &coef, 410);
			else if (md_lvl_1 == 100)
				set_coef(ctx, &coef, 461);
			else
				set_coef(ctx, &coef, 512);
		} else if (md_lvl_0 == 100) {
			if (md_lvl_1 < 100)
				set_coef(ctx, &coef, 512);
			else if (md_lvl_1 == 100)
				set_coef(ctx, &coef, 563);
			else
				set_coef(ctx, &coef, 614);
		} else
			set_coef(ctx, &coef, 614);
	} else if (md_lvl_sum < 1500)
		set_coef(ctx, &coef, 666);
	else if (md_lvl_sum < 1900)
		set_coef(ctx, &coef, 768);
	else
		set_coef(ctx, &coef, 900);

	if (coef > 1024)
		coef = 1024;

	if (coef >= 900)
		coef2 = 1024;
	else if (coef >= 307)	// 0.7~0.3 --> 1.0~0.5
		coef2 = 512 + (coef - 307) * (1024 - 512) / (717 - 307);
	else	// 0.3~0.0 --> 0.5~0.0
		coef2 = 0 + coef * (512 - 0) / (307 - 0);
	if (coef2 >= 1024)
		coef2 = 1024;

	return coef2;
}

/* bit_target_use: average of bits_target_lr and bits_target_hr */
static RK_S32 derive_iframe_qp_by_bitrate(RcModelV2SmtCtx *p, RK_S32 bit_target_use)
{
	RcFpsCfg *fps = &p->usr_cfg.fps;
	RK_S32 avg_bps = (p->usr_cfg.bps_min + p->usr_cfg.bps_max) / 2;
	RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
	RK_S32 avg_pqp = mpp_data_avg(p->qp_p, -1, 1, 1);
	RK_S32 avg_qp = mpp_clip(avg_pqp, p->qp_min, p->qp_max);
	RK_S32 prev_iqp = p->pre_qp_i;
	RK_S32 prev_pqp = p->qp_prev_out;
	RK_S32 pre_bits_i = p->pre_real_bit_i;
	RK_S32 qp_out_i = 0;

	if (bit_target_use <= pre_bits_i) {
		qp_out_i = (bit_target_use * 5 < pre_bits_i) ? prev_iqp + 3 :
			   (bit_target_use * 2 < pre_bits_i) ? prev_iqp + 2 :
			   (bit_target_use * 3 < pre_bits_i * 2) ? prev_iqp + 1 : prev_iqp;
	} else {
		qp_out_i = (pre_bits_i * 3 < bit_target_use) ? prev_iqp - 3 :
			   (pre_bits_i * 2 < bit_target_use) ? prev_iqp - 2 :
			   (pre_bits_i * 3 < bit_target_use * 2) ? prev_iqp - 1 : prev_iqp;
	}
	rc_dbg_rc("frame %lld bit_target_use %d pre_bits_i %d prev_iqp %d qp_out_i %d\n",
		  p->frm_num, bit_target_use, pre_bits_i, prev_iqp, qp_out_i);

	//FIX: may be invalid(2025.01.06)
	if (!p->reenc_cnt && p->usr_cfg.debreath_cfg.enable)
		calc_smt_debreath_qp(p);

	qp_out_i = mpp_clip(qp_out_i, inter_pqp0[avg_qp], inter_pqp1[avg_qp]);
	qp_out_i = mpp_clip(qp_out_i, inter_pqp0[prev_pqp], inter_pqp1[prev_pqp]);
	if (qp_out_i > 27)
		qp_out_i = mpp_clip(qp_out_i, intra_pqp0[0][prev_iqp], intra_pqp1[prev_iqp]);
	else if (qp_out_i > 22)
		qp_out_i = mpp_clip(qp_out_i, intra_pqp0[1][prev_iqp], intra_pqp1[prev_iqp]);
	else
		qp_out_i = mpp_clip(qp_out_i, intra_pqp0[2][prev_iqp], intra_pqp1[prev_iqp]);

	rc_dbg_rc("frame %lld qp_out_i %d avg_qp %d prev_pqp %d prev_iqp %d qp_out_i %d\n",
		  p->frm_num, qp_out_i, avg_qp, prev_pqp, prev_iqp, qp_out_i);

	if (p->pre_gop_left_bit < 0) {
		if (abs(p->pre_gop_left_bit) * 5 > avg_bps * (p->igop / fps_out))
			qp_out_i = mpp_clip(qp_out_i, 20, 51);
		else if (abs(p->pre_gop_left_bit) * 20 > avg_bps * (p->igop / fps_out))
			qp_out_i = mpp_clip(qp_out_i, 15, 51);

		rc_dbg_rc("frame %lld pre_gop_left_bit %d avg_bps %d qp_out_i %d\n",
			  p->frm_num, p->pre_gop_left_bit, avg_bps, qp_out_i);
	}

	return qp_out_i;
}

static RK_S32 derive_pframe_qp_by_bitrate(RcModelV2SmtCtx *p)
{
	RcFpsCfg *fps = &p->usr_cfg.fps;
	RK_S32 avg_bps = (p->usr_cfg.bps_min + p->usr_cfg.bps_max) / 2;
	RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
	RK_S32 bits_target_use = 0;
	RK_S32 pre_diff_bit_use = 0;
	RK_S32 coef = smt_calc_coef(p);
	RK_S32 m_tbr = p->bits_target_hr - p->bits_target_lr;
	RK_S32 m_dbr = p->pre_diff_bit_hr - p->pre_diff_bit_lr;
	RK_S32 diff_bit = (p->pid_alr.i + p->pid_ahr.i) >> 1;
	RK_S32 prev_pqp = p->qp_prev_out;
	RK_S32 qp_out = p->qp_out;
	RK_S32 qp_add = 0, qp_minus = 0;

	bits_target_use = ((RK_S64)m_tbr * coef + (RK_S64)p->bits_target_lr * 1024) >> 10;
	pre_diff_bit_use = ((RK_S64)m_dbr * coef + (RK_S64)p->pre_diff_bit_lr * 1024) >> 10;

	if (bits_target_use < 100)
		bits_target_use = 100;

	rc_dbg_rc("frame %lld bits_target_use %d m_tbr %d coef %d bits_target_lr %d\n"
		  "pre_diff_bit_use %d m_dbr %d  pre_diff_bit_lr %d "
		  "bits_target_hr %d pre_diff_bit_hr %d qp_out_0 %d\n",
		  p->frm_num, bits_target_use, m_tbr, coef, p->bits_target_lr,
		  pre_diff_bit_use, m_dbr, p->pre_diff_bit_lr,
		  p->bits_target_hr, p->pre_diff_bit_hr, qp_out);

	if (abs(pre_diff_bit_use) * 100 <= bits_target_use * 3)
		qp_out = prev_pqp - 1;
	else if (pre_diff_bit_use * 100 > bits_target_use * 3) {
		if (pre_diff_bit_use >= bits_target_use)
			qp_out = qp_out >= 30 ? prev_pqp - 4 : prev_pqp - 3;
		else if (pre_diff_bit_use * 4 >= bits_target_use * 1)
			qp_out = qp_out >= 30 ? prev_pqp - 3 : prev_pqp - 2;
		else if (pre_diff_bit_use * 10 > bits_target_use * 1)
			qp_out = prev_pqp - 2;
		else
			qp_out = prev_pqp - 1;
	} else {
		RK_S32 qp_add_tmp = (prev_pqp >= 36) ? 0 : 1;
		pre_diff_bit_use = abs(pre_diff_bit_use);
		qp_out = (pre_diff_bit_use >= 2 * bits_target_use) ? prev_pqp + 2 + qp_add_tmp :
			 (pre_diff_bit_use * 3 >= bits_target_use * 2) ? prev_pqp + 1 + qp_add_tmp :
			 (pre_diff_bit_use * 5 >  bits_target_use) ? prev_pqp + 1 : prev_pqp;
	}
	rc_dbg_rc("frame %lld prev_pqp %d qp_out_1 %d\n", p->frm_num, prev_pqp, qp_out);

	qp_out = mpp_clip(qp_out, p->qp_min, p->qp_max);
	if (qp_out > LOW_QP) {
		pre_diff_bit_use = ((RK_S64)m_dbr * coef + (RK_S64)p->pre_diff_bit_lr * 1024) >> 10;
		bits_target_use = avg_bps / fps_out;
		bits_target_use = -bits_target_use / 5;
		coef += pre_diff_bit_use <= 2 * bits_target_use ? 205 :
			((pre_diff_bit_use <= bits_target_use) ? 102 : 51);

		if (coef >= 1024 || qp_out > LOW_LOW_QP)
			coef = 1024;
		rc_dbg_rc("frame %lld pre_diff_bit_use %d bits_target_use %d coef %d\n",
			  p->frm_num, pre_diff_bit_use, bits_target_use, coef);

		pre_diff_bit_use = ((RK_S64)m_dbr * coef + (RK_S64)p->pre_diff_bit_lr * 1024) >> 10;
		bits_target_use = ((RK_S64)m_tbr * coef + (RK_S64)p->bits_target_lr * 1024) >> 10;
		if (bits_target_use < 100)
			bits_target_use = 100;

		if (abs(pre_diff_bit_use) * 100 <= bits_target_use * 3)
			qp_out = prev_pqp;
		else if (pre_diff_bit_use * 100 > bits_target_use * 3) {
			if (pre_diff_bit_use >= bits_target_use)
				qp_out = qp_out >= 30 ? prev_pqp - 3 : prev_pqp - 2;
			else if (pre_diff_bit_use * 4 >= bits_target_use * 1)
				qp_out = qp_out >= 30 ? prev_pqp - 2 : prev_pqp - 1;
			else if (pre_diff_bit_use * 10 > bits_target_use * 1)
				qp_out = prev_pqp - 1;
			else
				qp_out = prev_pqp;
		} else {
			pre_diff_bit_use = abs(pre_diff_bit_use);
			qp_out = prev_pqp + (pre_diff_bit_use * 3 >= bits_target_use * 2 ? 1 : 0);
		}
		rc_dbg_rc("frame %lld pre_diff_bit_use %d bits_target_use %d prev_pqp %d qp_out_2 %d\n",
			  p->frm_num, pre_diff_bit_use, bits_target_use, prev_pqp, qp_out);
	}

	qp_out = mpp_clip(qp_out, p->qp_min, p->qp_max);
	qp_add = qp_out > 36 ? 1 : (qp_out > 33 ? 2 : (qp_out > 30 ? 3 : 4));
	qp_minus = qp_out > 40 ? 4 : (qp_out > 36 ? 3 : (qp_out > 33 ? 2 : 1));
	qp_out = mpp_clip(qp_out, prev_pqp - qp_minus, prev_pqp + qp_add);
	rc_dbg_rc("frame %lld qp_out_3 %d qp_add %d qp_minus %d\n",
		  p->frm_num, qp_out, qp_add, qp_minus);

	if (diff_bit > 0) {
		if (avg_bps * 5 > avg_bps) //FIXME: avg_bps is typo error?(2025.01.06)
			qp_out = mpp_clip(qp_out, 25, 51);
		else if (avg_bps * 20 > avg_bps)
			qp_out = mpp_clip(qp_out, 21, 51);
		rc_dbg_rc("frame %lld avg_bps %d qp_out_4 %d\n", p->frm_num, avg_bps, qp_out);
	}

	return qp_out;
}

static RK_S32 revise_qp_by_complexity(RcModelV2SmtCtx *p, RK_S32 fm_min_iqp,
				      RK_S32 fm_min_pqp, RK_S32 fm_max_iqp, RK_S32 fm_max_pqp)
{
	RK_S32 md_lvl_sum = mpp_data_sum_v2(p->motion_level);
	RK_S32 md_lvl_0 = mpp_data_get_pre_val_v2(p->motion_level, 0);
	RK_S32 cplx_lvl_sum = mpp_data_sum_v2(p->complex_level);
	RK_S32 qp_add = 0, qp_add_p = 0;
	RK_S32 qp_final = p->qp_out;

	qp_add = 4;
	qp_add_p = 4;
	if (md_lvl_sum >= 700 || md_lvl_0 == 200) {
		qp_add = 6;
		qp_add_p = 5;
	} else if (md_lvl_sum >= 400 || md_lvl_0 == 100) {
		qp_add = 5;
		qp_add_p = 4;
	}

	if (cplx_lvl_sum >= 12) {
		qp_add++;
		qp_add_p++;
	}

	if (p->ptz_keep_cnt)
		qp_add = 0;

	rc_dbg_rc("frame %lld md_lvl_sum %d md_lvl_0 %d cplx_lvl_sum %d "
		  "qp_add_cplx %d qp_add_p %d qp_final_0 %d\n",
		  p->frm_num, md_lvl_sum, md_lvl_0, cplx_lvl_sum,
		  qp_add, qp_add_p, qp_final);

	if (p->frame_type == INTRA_FRAME) {
		qp_final = mpp_clip(qp_final, fm_min_iqp + qp_add, fm_max_iqp);
		qp_final = (fm_max_iqp == fm_min_iqp) ? fm_max_iqp : qp_final;
	} else {
		qp_final = mpp_clip(qp_final, fm_min_pqp + qp_add_p, fm_max_pqp);
		qp_final = (fm_max_pqp == fm_min_pqp) ? fm_max_pqp : qp_final;
	}

	if (p->frame_type == INTER_VI_FRAME) {
		qp_final -= 1;
		qp_final = mpp_clip(qp_final, fm_min_pqp + qp_add - 1, fm_max_pqp);
		qp_final = (fm_max_pqp == fm_min_pqp) ? fm_max_pqp : qp_final;
	}

	qp_final = mpp_clip(qp_final, p->qp_min, p->qp_max);
	rc_dbg_rc("frame %lld frm_type %d frm_qp %d:%d:%d:%d blk_qp %d:%d qp_final_1 %d\n",
		  p->frm_num, p->frame_type, fm_min_iqp, fm_max_iqp,
		  fm_min_pqp, fm_max_pqp, p->qp_min, p->qp_max, qp_final);

	return qp_final;
}

static RK_S32 revise_qp_by_light(RcModelV2SmtCtx *ctx)
{
	RK_S32 level = ctx->usr_cfg.lgt_chg_lvl;
	RK_S32 pre_val_0 = mpp_data_get_pre_val_v2(ctx->stat_luma_ave, 0);
	RK_S32 pre_val_1 = mpp_data_get_pre_val_v2(ctx->stat_luma_ave, 1);
	RK_S32 luma_diff = abs(pre_val_0 - pre_val_1);
	RK_S32 qp_in = ctx->qp_out;
	RK_S32 qp_ave = qp_in;
	RK_S32 chg_flag = 0, chg_num = 0;

	rc_dbg_rc("frame %lld luma_diff %d pre_val_0 %d pre_val_1 %d qp_in %d\n",
		  ctx->frm_num, luma_diff, pre_val_0, pre_val_1, qp_in);

	chg_flag = (luma_diff > 3 && level == 3) ||
		   (luma_diff > 5 && level == 2) || (luma_diff > 8 && level == 1);

	mpp_data_update_v2(ctx->lgt_chg_flg, chg_flag);
	chg_num = (ctx->frame_type == INTRA_FRAME) ? 0 : mpp_data_sum_v2(ctx->lgt_chg_flg);

	if (chg_num) {
		RK_S32 qp_pre_ave = mpp_data_avg(ctx->qp_p, chg_num, 1, 1);
		qp_ave = (qp_pre_ave * chg_num + qp_in * (LIGHT_STAT_LEN - chg_num)) / LIGHT_STAT_LEN;
		rc_dbg_rc("frame %lld chg_num %d qp_pre_ave %d qp_in %d qp_ave %d\n",
			  ctx->frm_num, chg_num, qp_pre_ave, qp_in, qp_ave);
	}

	ctx->cur_lgt_chg_flg = chg_num;

	return qp_ave;
}

MPP_RET rc_model_v2_smt_start(void *ctx, EncRcTask *task)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	EncFrmStatus *frm = &task->frm;
	EncRcTaskInfo *info = &task->info;
	RcFpsCfg *fps = &p->usr_cfg.fps;
	RK_S32 fps_out = fps->fps_out_num / fps->fps_out_denom;
	RK_S32 avg_pqp = 0;
	RK_S32 fm_min_iqp = p->usr_cfg.fm_lv_min_i_quality;
	RK_S32 fm_min_pqp = p->usr_cfg.fm_lv_min_quality;
	RK_S32 fm_max_iqp = p->usr_cfg.fm_lv_max_i_quality;
	RK_S32 fm_max_pqp = p->usr_cfg.fm_lv_max_quality;

	if (frm->reencode)
		return MPP_OK;

	smt_start_prepare(ctx, task, &fm_min_iqp, &fm_min_pqp, &fm_max_iqp, &fm_max_pqp);

	if (p->frm_num == 0) {
		RK_S32 mb_w = MPP_ALIGN(p->usr_cfg.width, 16) / 16;
		RK_S32 mb_h = MPP_ALIGN(p->usr_cfg.height, 16) / 16;
		RK_S32 ratio = mpp_clip(fps_out  / 10 + 1, 1, 3);
		RK_S32 qp_out_f0 = 0;
		if (p->usr_cfg.init_quality < 0) {
			qp_out_f0 = cal_smt_first_i_start_qp(p->bits_target_hr * ratio, mb_w * mb_h);
			if (fm_min_iqp > 31)
				qp_out_f0 = mpp_clip(qp_out_f0, fm_min_iqp, p->qp_max);
			else
				qp_out_f0 = mpp_clip(qp_out_f0, 31, p->qp_max);
		} else
			qp_out_f0 = p->usr_cfg.init_quality;
		p->qp_out = qp_out_f0;

		rc_dbg_rc("first frame init_quality %d bits_tgt_upper %d "
			  "cu16_w %d cu16_h %d ratio %d qp_init %d\n",
			  p->usr_cfg.init_quality, p->bits_target_hr,
			  mb_w, mb_h, ratio, p->qp_out);
	} else if (p->frame_type == INTRA_FRAME) {
		// if (p->frm_num > 0)
		p->qp_out = derive_iframe_qp_by_bitrate(p, info->bit_max);
	} else {
		if (p->last_frame_type == INTRA_FRAME) {
			RK_S32 prev_qp = p->qp_prev_out;
			p->qp_out = prev_qp + (prev_qp < 33 ? 3 : (prev_qp < 35 ? 2 : 1));
		} else
			p->qp_out = derive_pframe_qp_by_bitrate(p);
	}

	p->qp_out = revise_qp_by_complexity(p, fm_min_iqp, fm_min_pqp, fm_max_iqp, fm_max_pqp);

	avg_pqp = mpp_data_avg(p->qp_p, -1, 1, 1);
	info->complex_scene = 0;
	if (p->frame_type == INTER_P_FRAME && avg_pqp >= fm_max_pqp - 1 &&
	    p->qp_out == fm_max_pqp && p->qp_prev_out == fm_max_pqp) {
		info->complex_scene = 1;
		if (p->frame_type == INTRA_FRAME)
			info->complex_scene &= (fm_max_iqp != fm_min_iqp);
		else
			info->complex_scene &= (fm_max_pqp != fm_min_pqp);
	}

	if (p->frm_num >= 2 && p->usr_cfg.lgt_chg_lvl)
		p->qp_out = revise_qp_by_light(p);

	info->lgt_chg_enable = p->cur_lgt_chg_flg;
	info->quality_target = p->qp_out;
	info->quality_max = p->usr_cfg.max_quality;
	info->quality_min = p->usr_cfg.min_quality;
	info->bit_target = p->bits_target_hr;

	rc_dbg_rc("frame %lld quality [%d : %d : %d] complex_scene %d\n",
		  p->frm_num, info->quality_min, info->quality_target,
		  info->quality_max, info->complex_scene);

	p->reenc_cnt = 0;

	if (p->usr_cfg.super_cfg.super_mode) {
		RK_U32 super_thd = p->frame_type == INTRA_FRAME ?
				   p->usr_cfg.super_cfg.super_i_thd :
				   p->usr_cfg.super_cfg.super_p_thd;

		if (p->usr_cfg.super_cfg.rc_priority == MPP_ENC_RC_BY_FRM_SIZE_FIRST) {
			if (info->bit_target > super_thd)
				info->bit_target = super_thd;
		}
	}
	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

MPP_RET check_super_frame_smt(RcModelV2SmtCtx *ctx, EncRcTaskInfo *cfg)
{
	MPP_RET ret = MPP_OK;
	RK_S32 frame_type = ctx->frame_type;
	RK_U32 bits_thr = 0;
	RcCfg *usr_cfg = &ctx->usr_cfg;

	rc_dbg_func("enter %p\n", ctx);
	if (usr_cfg->super_cfg.super_mode) {
		bits_thr = usr_cfg->super_cfg.super_p_thd;
		if (frame_type == INTRA_FRAME)
			bits_thr = usr_cfg->super_cfg.super_i_thd;

		if ((RK_U32)cfg->bit_real >= bits_thr) {
			if (usr_cfg->super_cfg.super_mode == MPP_ENC_RC_SUPER_FRM_DROP) {
				rc_dbg_rc("super frame drop current frame");
				usr_cfg->drop_mode = MPP_ENC_RC_DROP_FRM_NORMAL;
				usr_cfg->drop_gap  = 0;
			}
			rc_dbg_rc("bit real %d >= bit thd %d\n", cfg->bit_real, bits_thr);
			ret = MPP_NOK;
		}
	}
	rc_dbg_func("leave %p\n", ctx);
	return ret;
}

MPP_RET check_re_enc_smt(RcModelV2SmtCtx *ctx, EncRcTaskInfo *cfg)
{
	RcCfg *usr_cfg = &ctx->usr_cfg;
	RK_S32 frame_type = ctx->frame_type;
	RK_S32 bit_thr = 0;
	RK_S32 stat_time = usr_cfg->stats_time;
	RK_S32 last_ins_bps = mpp_data_sum_v2(ctx->stat_bits) / stat_time;
	RK_S32 ins_bps = (last_ins_bps * stat_time - mpp_data_get_pre_val_v2(ctx->stat_bits, -1)
			  + cfg->bit_real) / stat_time;
	RK_S32 bps;
	RK_S32 ret = MPP_OK;

	rc_dbg_func("enter %p\n", ctx);
	rc_dbg_rc("reenc check target_bps %d last_ins_bps %d ins_bps %d",
		  usr_cfg->bps_target, last_ins_bps, ins_bps);

	if (ctx->reenc_cnt >= usr_cfg->max_reencode_times)
		return MPP_OK;

	if (ctx->drop_cnt >= usr_cfg->max_reencode_times)
		return MPP_OK;

	if (check_super_frame_smt(ctx, cfg))
		return MPP_NOK;

	if (usr_cfg->debreath_cfg.enable && !ctx->first_frm_flg)
		return MPP_OK;

	rc_dbg_drop("drop mode %d frame_type %d\n", usr_cfg->drop_mode, frame_type);
	if (usr_cfg->drop_mode && frame_type == INTER_P_FRAME) {
		bit_thr = (RK_S32)(usr_cfg->bps_max * (100 + usr_cfg->drop_thd) / 100);
		rc_dbg_drop("drop mode %d check max_bps %d bit_thr %d ins_bps %d",
			    usr_cfg->drop_mode, usr_cfg->bps_target, bit_thr, ins_bps);
		return (ins_bps > bit_thr) ? MPP_NOK : MPP_OK;
	}

	switch (frame_type) {
	case INTRA_FRAME:
		bit_thr = 3 * cfg->bit_max / 2;
		break;
	case INTER_P_FRAME:
		bit_thr = 3 * cfg->bit_max;
		break;
	default:
		break;
	}

	if (cfg->bit_real > bit_thr) {
		bps = usr_cfg->bps_max;
		if ((bps - (bps >> 3) < ins_bps) && (bps / 20  < ins_bps - last_ins_bps))
			ret =  MPP_NOK;
	}

	rc_dbg_func("leave %p ret %d\n", ctx, ret);
	return ret;
}

MPP_RET rc_model_v2_smt_check_reenc(void *ctx, EncRcTask *task)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *)ctx;
	EncRcTaskInfo *cfg = (EncRcTaskInfo *)&task->info;
	EncFrmStatus *frm = &task->frm;
	RcCfg *usr_cfg = &p->usr_cfg;

	rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);

	frm->reencode = 0;

	if ((usr_cfg->mode == RC_FIXQP) ||
	    (task->force.force_flag & ENC_RC_FORCE_QP) ||
	    p->on_drop || p->on_pskip)
		return MPP_OK;

	if (check_re_enc_smt(p, cfg)) {
		MppEncRcDropFrmMode drop_mode = usr_cfg->drop_mode;
		RK_S32 bits_thr = p->frame_type == INTRA_FRAME ? usr_cfg->super_cfg.super_i_thd :
				  usr_cfg->super_cfg.super_p_thd;

		if (usr_cfg->drop_gap && p->drop_cnt >= usr_cfg->drop_gap)
			drop_mode = MPP_ENC_RC_DROP_FRM_DISABLED;

		rc_dbg_drop("reenc drop_mode %d drop_cnt %d\n", drop_mode, p->drop_cnt);

		switch (drop_mode) {
		case MPP_ENC_RC_DROP_FRM_NORMAL : {
			if (cfg->bit_real > bits_thr * 2)
				p->qp_add += 3;
			else if (cfg->bit_real > bits_thr * 3 / 2)
				p->qp_add += 2;
			else if (cfg->bit_real > bits_thr)
				p->qp_add += 1;

			frm->drop = 1;
			frm->reencode = 1;
			p->on_drop = 1;
			p->drop_cnt++;
			rc_dbg_drop("drop\n");
		} break;
		case MPP_ENC_RC_DROP_FRM_PSKIP : {
			frm->force_pskip = 1;
			frm->reencode = 1;
			p->on_pskip = 1;
			p->drop_cnt++;
			rc_dbg_drop("force_pskip\n");
		} break;
		case MPP_ENC_RC_DROP_FRM_DISABLED :
		default : {
			if (cfg->bit_real > bits_thr * 2)
				p->qp_add = 3;
			else if (cfg->bit_real > bits_thr * 3 / 2)
				p->qp_add = 2;
			else if (cfg->bit_real > bits_thr)
				p->qp_add = 1;

			if (cfg->quality_target + p->qp_add <= cfg->quality_max) {
				p->reenc_cnt++;
				frm->reencode = 1;
			}
			p->drop_cnt = 0;
			rc_dbg_drop("drop disable\n");
		} break;
		}
	} else {
		p->qp_add = 0;
		p->drop_cnt = 0;
		p->on_drop = 0;
	}

	return MPP_OK;
}

MPP_RET rc_model_v2_smt_end(void *ctx, EncRcTask * task)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	EncRcTaskInfo *cfg = (EncRcTaskInfo *) & task->info;
	RK_S32 bit_real = cfg->bit_real;

	rc_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);
	if (p->ptz_keep_cnt) {
		bit_real /= 3;
		if (p->ptz_keep_cnt > 0)
			p->ptz_keep_cnt = 0;
	}

	if (p->cur_lgt_chg_flg && !p->ptz_keep_cnt) {
		RK_S32 chg_lvl = p->usr_cfg.lgt_chg_lvl;
		bit_real = (chg_lvl == 3) ? bit_real * 1 / 3 :
			   (chg_lvl == 2) ? bit_real * 2 / 3 : bit_real;
		p->cur_lgt_chg_flg = 0;
	}

	rc_dbg_rc("motion_level %u complex_level %u dsp_y %d\n",
		  cfg->motion_level, cfg->complex_level, cfg->dsp_luma_ave);

	mpp_data_update_v2(p->motion_level, cfg->motion_level);
	mpp_data_update_v2(p->complex_level, cfg->complex_level);
	mpp_data_update_v2(p->stat_luma_ave, cfg->dsp_luma_ave);
	p->first_frm_flg = 0;

	if (p->frame_type == INTER_P_FRAME || p->gop_mode == MPP_GOP_ALL_INTRA)
		mpp_data_update(p->qp_p, p->qp_out);
	else {
		p->pre_qp_i = p->qp_out;
		p->pre_real_bit_i = bit_real;
	}

	rc_dbg_rc("frame %lld update real_bit %d motion_level %u, complex_level %u\n\n",
		  p->frm_num, bit_real, cfg->motion_level, cfg->complex_level);
	bits_model_update_smt(p, bit_real);
	p->qp_prev_out = p->qp_out;
	p->last_frame_type = p->frame_type;
	p->pre_iblk4_prop = cfg->iblk4_prop;
	p->gop_frm_cnt++;
	p->frm_num++;
	p->gop_qp_sum += p->qp_out;

	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

MPP_RET rc_model_v2_smt_hal_start(void *ctx, EncRcTask * task)
{
	EncRcTaskInfo *cfg = (EncRcTaskInfo *)&task->info;
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *)ctx;

	p->on_drop = 0;
	cfg->quality_target += p->qp_add;
	cfg->quality_target = mpp_clip(cfg->quality_target, cfg->quality_min, cfg->quality_max);
	p->qp_out = cfg->quality_target;

	rc_dbg_rc("qp [%d %d %d] qp_add_reenc %d\n", cfg->quality_min,
		  cfg->quality_target, cfg->quality_max, p->qp_add);
	rc_dbg_func("smt_hal_start enter ctx %p task %p\n", ctx, task);
	return MPP_OK;
}

MPP_RET rc_model_v2_smt_hal_end(void *ctx, EncRcTask * task)
{
	rc_dbg_func("smt_hal_end enter ctx %p task %p\n", ctx, task);
	rc_dbg_func("leave %p\n", ctx);
	return MPP_OK;
}

void rc_model_v2_smt_proc_show(void *seq_file, void *ctx, RK_S32 chl_id)
{
	RcModelV2SmtCtx *p = (RcModelV2SmtCtx *) ctx;
	RcCfg *usr_cfg = &p->usr_cfg;
	RK_U32 target_bps = usr_cfg->bps_max;
	struct seq_file *seq  = (struct seq_file *)seq_file;

	if (usr_cfg->mode == RC_CBR)
		target_bps = usr_cfg->bps_target;
	seq_puts(seq,
		 "\n--------RC base param----------------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%7s|%8s|%6s|%6s|%8s|%13s|%13s|%5s|%5s \n",
		   "ChnId", "Gop", "StatTm", "ViFr", "TrgFr",
		   "RcMode", "MinBr(kbps)", "MaxBr(kbps)", "IQp", "PQp");

	if (usr_cfg->mode == RC_FIXQP) {
		seq_printf(seq, "%7d|%7u|%8u|%6u|%6u|%8s|%13s|%13s|%5u|%5u \n",
			   chl_id, usr_cfg->igop, usr_cfg->stats_time,
			   usr_cfg->fps.fps_in_num / usr_cfg->fps.fps_in_denom,
			   usr_cfg->fps.fps_out_num / usr_cfg->fps.fps_out_denom,
			   strof_rc_mode(usr_cfg->mode), "N/A", "N/A",
			   usr_cfg->init_quality, usr_cfg->init_quality);
	} else {
		seq_printf(seq, "%7d|%7u|%8u|%6u|%6u|%8s|%13u|%13u|%5s|%5s \n",
			   chl_id, usr_cfg->igop, usr_cfg->stats_time,
			   usr_cfg->fps.fps_in_num / usr_cfg->fps.fps_in_denom,
			   usr_cfg->fps.fps_out_num / usr_cfg->fps.fps_out_denom,
			   "smart", usr_cfg->bps_min / 1000, usr_cfg->bps_max / 1000, "N/A", "N/A");
	}

	seq_puts(seq,
		 "\n--------RC run comm param 1----------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%8s|%12s|%12s|%10s \n",
		   "ChnId", "bLost", "LostThr", "LostFrmStr", "EncGap");
	seq_printf(seq, "%7d|%8s|%12u|%12d|%10u \n",
		   chl_id, strof_drop(usr_cfg->drop_mode), usr_cfg->drop_thd, p->drop_cnt, usr_cfg->drop_gap);

	seq_puts(seq,
		 "\n--------RC run comm param 2----------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%12s|%12s|%12s|%12s \n",
		   "ChnId", "SprFrmMod", "SprIFrm", "SprPFrm", "RCPriority");
	seq_printf(seq, "%7d|%12s|%12u|%12u|%12u \n",
		   chl_id, strof_suprmode(usr_cfg->super_cfg.super_mode), usr_cfg->super_cfg.super_i_thd,
		   usr_cfg->super_cfg.super_p_thd, usr_cfg->super_cfg.rc_priority);

	seq_puts(seq,
		 "\n--------RC gop mode attr-------------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%10s|%10s|%12s|%10s \n",
		   "ChnId", "GopMode", "IpQpDelta", "BgInterval", "ViQpDelta");
	if (usr_cfg->gop_mode == SMART_P) {
		seq_printf(seq, "%7d|%10s|%10d|%12u|%10d\n",
			   chl_id, strof_gop_mode(usr_cfg->gop_mode), usr_cfg->i_quality_delta,
			   usr_cfg->vgop, usr_cfg->i_quality_delta);
	} else {
		seq_printf(seq, "%7d|%10s|%10d|%12s|%10s\n",
			   chl_id, strof_gop_mode(usr_cfg->gop_mode), usr_cfg->i_quality_delta,
			   "N/A", "N/A");
	}

	switch (usr_cfg->mode) {
	case RC_CBR: {
	} break;
	case RC_SMT:
	case RC_VBR: {
		seq_puts(seq,
			 "\n--------RC run smart common param----------------------------------------------------------------\n");
		seq_printf(seq, "%7s|%8s|%8s|%8s|%8s|%10s|%10s|%10s|%10s|%15s\n",
			   "ChnId", "MaxQp", "MinQp", "MaxIQp", "MinIQp",
			   "FrmMaxQp", "FrmMinQp", "FrmMaxIQp", "FrmMinIQp",
			   "MaxReEncTimes");

		seq_printf(seq, "%7d|%8u|%8u|%8u|%8u|%10d|%10d|%10d|%10d|%15d\n", chl_id,
			   usr_cfg->max_quality, usr_cfg->min_quality, usr_cfg->max_i_quality,
			   usr_cfg->min_i_quality, usr_cfg->fm_lv_max_quality,
			   usr_cfg->fm_lv_min_quality, usr_cfg->fm_lv_max_i_quality,
			   usr_cfg->fm_lv_min_i_quality, usr_cfg->max_reencode_times);


	} break;
	case RC_AVBR: {
	} break;
	default: {
		;
	} break;
	}

	seq_puts(seq,
		 "\n--------RC HierarchicalQp INFO-------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%10s|%12s|%12s|%12s|%12s|%12s|%12s|%12s|%12s\n",
		   "ChnId", "bEnable",
		   "FrameNum[0]", "FrameNum[1]",
		   "FrameNum[2]", "FrameNum[3]",
		   "QpDelta[0]", "QpDelta[1]",
		   "QpDelta[2]", "QpDelta[3]");
	seq_printf(seq, "%7d|%10s|%12d|%12d|%12d|%12d|%12d|%12d|%12d|%12d\n",
		   chl_id, strof_bool(usr_cfg->hier_qp_cfg.hier_qp_en),
		   usr_cfg->hier_qp_cfg.hier_frame_num[0], usr_cfg->hier_qp_cfg.hier_frame_num[1],
		   usr_cfg->hier_qp_cfg.hier_frame_num[2], usr_cfg->hier_qp_cfg.hier_frame_num[3],
		   usr_cfg->hier_qp_cfg.hier_qp_delta[0], usr_cfg->hier_qp_cfg.hier_qp_delta[1],
		   usr_cfg->hier_qp_cfg.hier_qp_delta[2], usr_cfg->hier_qp_cfg.hier_qp_delta[3]);

	seq_puts(seq,
		 "\n--------RC debreath_effect info------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%10s|%10s|%18s\n", "ChnId", "bEnable", "Strength0", "DeBrthEfctCnt");
	if (usr_cfg->debreath_cfg.enable)
		seq_printf(seq, "%7d|%10s|%10d|%18u\n",
			   chl_id, strof_bool(usr_cfg->debreath_cfg.enable),
			   usr_cfg->debreath_cfg.strength, 0);
	else
		seq_printf(seq, "%7d|%10s|%10s|%18u\n", chl_id,
			   strof_bool(usr_cfg->debreath_cfg.enable), "N/A", 0);

	seq_puts(seq,
		 "\n--------RC run smart info1-----------------------------------------------------------------------------\n");
	seq_printf(seq, "%7s|%12s|%10s|%10s|%15s|%15s|%12s\n",
		   "ChnId", "RealBt(kb)", "IPRatio", "StartQp", "md_switch_en", "md_switch_qp", "scene_mode");
	seq_printf(seq, "%7d|%12u|%10d|%10u|%15d|%15d|%12d\n",	 chl_id, p->last_fps_bits / 1000,
		   usr_cfg->init_ip_ratio, usr_cfg->init_quality, usr_cfg->motion_static_switch_enable,
		   usr_cfg->mt_st_swth_frm_qp, usr_cfg->scene_mode);
}

const RcImplApi smt_h264e = {
	"smart",
	MPP_VIDEO_CodingAVC,
	sizeof(RcModelV2SmtCtx),
	rc_model_v2_smt_h264_init,
	rc_model_v2_smt_deinit,
	NULL,
	rc_model_v2_smt_check_reenc,
	rc_model_v2_smt_start,
	rc_model_v2_smt_end,
	rc_model_v2_smt_hal_start,
	rc_model_v2_smt_hal_end,
	rc_model_v2_smt_proc_show,
};

const RcImplApi smt_h265e = {
	"smart",
	MPP_VIDEO_CodingHEVC,
	sizeof(RcModelV2SmtCtx),
	rc_model_v2_smt_h265_init,
	rc_model_v2_smt_deinit,
	NULL,
	rc_model_v2_smt_check_reenc,
	rc_model_v2_smt_start,
	rc_model_v2_smt_end,
	rc_model_v2_smt_hal_start,
	rc_model_v2_smt_hal_end,
	rc_model_v2_smt_proc_show,
};
#endif
