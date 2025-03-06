// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 *
 *
 */

#ifndef __VEPU500_PP_H__
#define __VEPU500_PP_H__

#include <linux/types.h>

#include "kmpp_dmabuf.h"
#include "kmpp_allocator.h"

#include "kmpp_vsp_objs_impl.h"
#include "vepu_pp_common.h"

#define MAX_CHN_NUM  (8)

struct pp_buffer_t {
	osal_dmabuf *osal_buf;
	dma_addr_t iova;
};

struct pp_param_t {
	struct {
		u32 enc_done_en          : 1;
		u32 lkt_node_done_en     : 1;
		u32 sclr_done_en         : 1;
		u32 vslc_done_en         : 1;
		u32 vbsf_oflw_en         : 1;
		u32 vbuf_lens_en         : 1;
		u32 enc_err_en           : 1;
		u32 vsrc_err_en          : 1;
		u32 wdg_en               : 1;
		u32 lkt_err_int_en       : 1;
		u32 lkt_err_stop_en      : 1;
		u32 lkt_force_stop_en    : 1;
		u32 jslc_done_en         : 1;
		u32 jbsf_oflw_en         : 1;
		u32 jbuf_lens_en         : 1;
		u32 dvbm_err_en          : 1;
		u32 reserved             : 16;
	} int_en;

	/* 0x24 */
	u32 int_msk;

	/* 0x00000280 reg160 */
	u32 adr_src0;

	/* 0x00000284 reg161 */
	u32 adr_src1;

	/* 0x00000288 reg162 */
	u32 adr_src2;

	/* 0x000002a4 reg169 */
	u32 dspw_addr;

	/* 0x000002a8 reg170 */
	u32 dspr_addr;

	/* 0x00000300 reg192 */
	struct {
		u32 enc_stnd                : 2;
		u32 cur_frm_ref             : 1;
		u32 mei_stor                : 1;
		u32 bs_scp                  : 1;
		u32 reserved                : 3;
		u32 pic_qp                  : 6;
		u32 num_pic_tot_cur_hevc    : 5;
		u32 log2_ctu_num_hevc       : 5;
		u32 reserved1               : 3;
		u32 eslf_out_e_jpeg         : 1;
		u32 jpeg_slen_fifo          : 1;
		u32 eslf_out_e              : 1;
		u32 slen_fifo               : 1;
		u32 rec_fbc_dis             : 1;
	} enc_pic;

	/* 0x00000308 reg194 */
	struct {
		u32 frame_id        : 8;
		u32 frm_id_match    : 1;
		u32 reserved        : 3;
		u32 source_id       : 1;
		u32 src_id_match    : 1;
		u32 reserved1       : 2;
		u32 ch_id           : 2;
		u32 vrsp_rtn_en     : 1;
		u32 vinf_req_en     : 1;
		u32 reserved2       : 12;
	} enc_id;

	/* 0x00000310 reg196 */
	struct {
		u32 pic_wd8_m1    : 11;
		u32 reserved      : 5;
		u32 pic_hd8_m1    : 11;
		u32 reserved1     : 5;
	} enc_rsl;

	/* 0x00000314 reg197 */
	struct {
		u32 pic_wfill    : 6;
		u32 reserved     : 10;
		u32 pic_hfill    : 6;
		u32 reserved1    : 10;
	} src_fill;

	/* 0x00000318 reg198 */
	struct {
		u32 alpha_swap            : 1;
		u32 rbuv_swap             : 1;
		u32 src_cfmt              : 4;
		u32 src_rcne              : 1;
		u32 out_fmt               : 1;
		u32 src_range_trns_en     : 1;
		u32 src_range_trns_sel    : 1;
		u32 chroma_ds_mode        : 1;
		u32 reserved              : 21;
	} src_fmt;

	/* 0x00000330 reg204 */
	struct {
		u32 pic_ofst_x    : 14;
		u32 reserved      : 2;
		u32 pic_ofst_y    : 14;
		u32 reserved1     : 2;
	} pic_ofst;

	/* 0x00000334 reg205 */
	struct {
		u32 src_strd0    : 21;
		u32 reserved     : 11;
	} src_strd0;

	/* 0x00000338 reg206 */
	struct {
		u32 src_strd1    : 16;
		u32 reserved     : 16;
	} src_strd1;

	/* 0x0000033c reg207 */
	struct {
		u32 pp_corner_filter_strength      : 2;
		u32 reserved                       : 2;
		u32 pp_edge_filter_strength        : 2;
		u32 reserved1                      : 2;
		u32 pp_internal_filter_strength    : 2;
		u32 reserved2                      : 22;
	} src_flt_cfg;

	/* 0x00000370 reg220 */
	struct {
		u32 cime_srch_dwnh    : 4;
		u32 cime_srch_uph     : 4;
		u32 cime_srch_rgtw    : 4;
		u32 cime_srch_lftw    : 4;
		u32 dlt_frm_num       : 16;
	} me_rnge;

	/* 0x00000374 reg221 */
	struct {
		u32 srgn_max_num      : 7;
		u32 cime_dist_thre    : 13;
		u32 rme_srch_h        : 2;
		u32 rme_srch_v        : 2;
		u32 rme_dis           : 3;
		u32 reserved1         : 1;
		u32 fme_dis           : 3;
		u32 reserved2         : 1;
	} me_cfg;

	/* 0x00000378 reg222 */
	struct {
		u32 cime_zero_thre     : 13;
		u32 reserved           : 15;
		u32 fme_prefsu_en      : 2;
		u32 colmv_stor         : 1;
		u32 colmv_load         : 1;
	} me_cach;

	/* 0x000003bc reg239 */
	struct {
		u32 cbc_init_flg           : 1;
		u32 mvd_l1_zero_flg        : 1;
		u32 mrg_up_flg             : 1;
		u32 mrg_lft_flg            : 1;
		u32 reserved               : 1;
		u32 ref_pic_lst_mdf_l0     : 1;
		u32 num_refidx_l1_act      : 2;
		u32 num_refidx_l0_act      : 2;
		u32 num_refidx_act_ovrd    : 1;
		u32 sli_sao_chrm_flg       : 1;
		u32 sli_sao_luma_flg       : 1;
		u32 sli_tmprl_mvp_e        : 1;
		u32 pic_out_flg            : 1;
		u32 sli_type               : 2;
		u32 sli_rsrv_flg           : 7;
		u32 dpdnt_sli_seg_flg      : 1;
		u32 sli_pps_id             : 6;
		u32 no_out_pri_pic         : 1;
	} synt_sli0;

	/* 0x0000047c reg287 */
	struct {
		u32 reserved             : 31;
		u32 jpeg_stnd            : 1;
	} jpeg_base_cfg;

	/* 0x00000520 reg328 */
	u32 adr_md_vpp;

	/* 0x00000524 reg329 */
	u32 adr_od_vpp;

	/* 0x00000528 reg330 */
	u32 adr_ref_mdw;

	/* 0x0000052c reg331 */
	u32 adr_ref_mdr;

	/* 0x00000530 reg332 */
	struct {
		u32 sto_stride_md          : 8;
		u32 sto_stride_od          : 8;
		u32 cur_frm_en_md          : 1;
		u32 ref_frm_en_md          : 1;
		u32 switch_sad_md          : 2;
		u32 night_mode_en_md       : 1;
		u32 flycatkin_flt_en_md    : 1;
		u32 en_od                  : 1;
		u32 background_en_od       : 1;
		u32 sad_comp_en_od         : 1;
		u32 reserved               : 6;
		u32 vepu_pp_en             : 1;
	} vpp_base_cfg;

	/* 0x00000534 reg333 */
	struct {
		u32 thres_sad_md          : 12;
		u32 thres_move_md         : 3;
		u32 reserved              : 1;
		u32 thres_dust_move_md    : 4;
		u32 thres_dust_blk_md     : 3;
		u32 reserved1             : 1;
		u32 thres_dust_chng_md    : 8;
	} thd_md_vpp;

	/* 0x00000538 reg334 */
	struct {
		u32 thres_complex_od        : 12;
		u32 thres_complex_cnt_od    : 3;
		u32 thres_sad_od            : 14;
		u32 reserved                : 3;
	} thd_od_vpp;
};

struct pp_output_t {
	u32 od_out_pix_sum; /* 0x4070 */
};

struct pp_chn_info_t {
	KmppVspPpCfg cfg;
	KmppFrame frame;
	rk_u32 frm_cnt;
	rk_u32 frm_num;
	KmppDmaBuf in_buf;
	KmppDmaBuf md_buf;
	KmppDmaBuf od_buf;

	u32 chn; /* channel ID */
	u32 used;
	u32 width;
	u32 height;
	u32 max_width;
	u32 max_height;
	int md_en;
	int od_en;
	int flycatkin_en;
	int frm_accum_interval;
	int frm_accum_gop;
	int mdw_len;

	struct pp_buffer_t *buf_rfpw;
	struct pp_buffer_t *buf_rfpr;

	u8 *buf_rfmwr; /* for fly-catkin filtering */
	u8 *buf_rfmwr0;
	u8 *buf_rfmwr1;
	u8 *buf_rfmwr2; /* md output without fly-catkin filtering */

	struct pp_param_t param;
	struct pp_output_t output;

	const struct pp_srv_api_t *api;
	void *dev_srv; 	/* communicate with rk_vcodec.ko */
	void *device;	/* struct device* */
};

struct vepu_pp_ctx_t {
	struct pp_chn_info_t chn_info[MAX_CHN_NUM];
};

extern KmppVspApi vepu500_pp_api;

#endif
