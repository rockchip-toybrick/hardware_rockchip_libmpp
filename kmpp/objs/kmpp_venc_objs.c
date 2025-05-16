/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_venc_objs"

#include "kmpp_log.h"

#include "kmpp_obj.h"
#include "kmpp_venc_objs_impl.h"

static rk_s32 kmpp_venc_init_cfg_impl_init(void *entry, KmppObj obj, osal_fs_dev *file, const rk_u8 *caller)
{
    if (entry) {
        KmppVencInitCfgImpl *impl = (KmppVencInitCfgImpl*)entry;

        impl->type = MPP_CTX_BUTT;
        impl->coding = MPP_VIDEO_CodingUnused;
        impl->chan_id = -1;
        impl->online = 0;
        impl->buf_size = 0;
        impl->max_strm_cnt = 0;
        impl->shared_buf_en = 0;
        impl->smart_en = 0;
        impl->max_width = 0;
        impl->max_height = 0;
        impl->max_lt_cnt = 0;
        impl->qpmap_en = 0;
        impl->chan_dup = 0;
        impl->tmvp_enable = 0;
        impl->only_smartp = 0;
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_init_cfg_impl_dump(void *entry)
{
    KmppVencInitCfgImpl *impl = (KmppVencInitCfgImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("type             %d\n", impl->type);
    kmpp_logi("coding           %d\n", impl->coding);
    kmpp_logi("chan_id          %d\n", impl->chan_id);
    kmpp_logi("online           %d\n", impl->online);
    kmpp_logi("buf_size         %d\n", impl->buf_size);
    kmpp_logi("max_strm_cnt     %d\n", impl->max_strm_cnt);
    kmpp_logi("shared_buf_en    %d\n", impl->shared_buf_en);
    kmpp_logi("smart_en         %d\n", impl->smart_en);
    kmpp_logi("max_width        %d\n", impl->max_width);
    kmpp_logi("max_height       %d\n", impl->max_height);
    kmpp_logi("max_lt_cnt       %d\n", impl->max_lt_cnt);
    kmpp_logi("qpmap_en         %d\n", impl->qpmap_en);
    kmpp_logi("chan_dup         %d\n", impl->chan_dup);
    kmpp_logi("tmvp_enable      %d\n", impl->tmvp_enable);
    kmpp_logi("only_smartp      %d\n", impl->only_smartp);

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_venc_init_cfg
#define KMPP_OBJ_INTF_TYPE          KmppVencInitCfg
#define KMPP_OBJ_IMPL_TYPE          KmppVencInitCfgImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_VENC_INIT_CFG_ENTRY_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_venc_init_cfg_impl_init
#define KMPP_OBJ_FUNC_DUMP          kmpp_venc_init_cfg_impl_dump
#include "kmpp_obj_helper.h"

/* ------------------------ kmpp notify infos ------------------------- */
static rk_s32 kmpp_venc_notify_impl_init(void *entry, KmppObj obj, osal_fs_dev *file, const rk_u8 *caller)
{
    if (entry) {
        KmppVencNtfyImpl *impl = (KmppVencNtfyImpl*)entry;

        impl->chan_id = -1;

        if (!impl->venc_syms)
            kmpp_syms_get(&impl->venc_syms, "venc");

        if (impl->venc_syms && !impl->ntfy_sym)
            kmpp_sym_get(&impl->ntfy_sym, impl->venc_syms, "venc_ntfy");
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_notify_impl_deinit(void *entry, const rk_u8 *caller)
{
    if (entry) {
        KmppVencNtfyImpl *impl = (KmppVencNtfyImpl*)entry;

        impl->chan_id = -1;

        if (impl->ntfy_sym) {
            kmpp_sym_put(impl->ntfy_sym);
            impl->ntfy_sym = NULL;
        }

        if (impl->venc_syms) {
            kmpp_syms_put(impl->venc_syms);
            impl->venc_syms = NULL;
        }
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_notify_impl_dump(void *entry)
{
    KmppVencNtfyImpl *impl = (KmppVencNtfyImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("chan_id      %d\n",      impl->chan_id);
    kmpp_logi("cmd          %d\n",      impl->cmd);
    kmpp_logi("type         %d\n",      impl->drop_type);
    kmpp_logi("pipe_id      %d\n",      impl->pipe_id);
    kmpp_logi("frame_id     %d\n",      impl->frame_id);
    kmpp_logi("frame        %p\n",      impl->frame);
    kmpp_logi("is_intra     %d\n",      impl->is_intra);
    kmpp_logi("luma_pix_sum_od %d\n",   impl->luma_pix_sum_od);
    kmpp_logi("md_index     %d\n",      impl->md_index);
    kmpp_logi("od_index     %d\n",      impl->od_index);

    return rk_ok;
}

rk_s32 kmpp_venc_notify(KmppVencNtfy ntfy)
{
    rk_s32 ret = rk_ok;
    KmppVencNtfyImpl *impl = kmpp_obj_to_entry(ntfy);

    if (!impl->ntfy_sym) {
        kmpp_err_f("the kmpp venc notify symbol is NULL\n");
        return rk_nok;
    }

    if (kmpp_sym_run(impl->ntfy_sym, ntfy, &ret))
        return rk_nok;

    return ret;
}

#define KMPP_OBJ_NAME               kmpp_venc_ntfy
#define KMPP_OBJ_INTF_TYPE          KmppVencNtfy
#define KMPP_OBJ_IMPL_TYPE          KmppVencNtfyImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_NOTIFY_CFG_ENTRY_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_venc_notify_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_venc_notify_impl_deinit
#define KMPP_OBJ_FUNC_DUMP          kmpp_venc_notify_impl_dump
#define KMPP_OBJ_SHARE_DISABLE
#include "kmpp_obj_helper.h"

/* ------------------------ kmpp venc static config ------------------------- */
#include "mpp_enc_cfg.h"

static rk_s32 kmpp_venc_st_cfg_impl_init(void *entry, KmppObj obj, osal_fs_dev *file, const rk_u8 *caller)
{
    if (entry) {
        MppEncCfgSet *cfg = (MppEncCfgSet *)entry;

        cfg->rc.max_reenc_times = 1;

        cfg->prep.color = MPP_FRAME_SPC_UNSPECIFIED;
        cfg->prep.colorprim = MPP_FRAME_PRI_UNSPECIFIED;
        cfg->prep.colortrc = MPP_FRAME_TRC_UNSPECIFIED;
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_st_cfg_impl_deinit(void *entry, const rk_u8 *caller)
{
    if (entry) {
        MppEncCfgSet *cfg = (MppEncCfgSet *)entry;

        osal_memset(cfg, 0, sizeof(MppEncCfgSet));
    }

    return rk_ok;
}

static rk_s32 kmpp_venc_st_cfg_impl_dump(void *entry)
{
    MppEncCfgSet *impl = (MppEncCfgSet*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("rc mode      %d\n",      impl->rc.rc_mode);
    kmpp_logi("bps target   %d\n",      impl->rc.bps_target);
    kmpp_logi("bps max      %d\n",      impl->rc.bps_max);
    kmpp_logi("bps min      %d\n",      impl->rc.bps_min);

    return rk_ok;
}

#define KMPP_VENC_ST_CFG_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    CFG_DEF_START() \
    STRUCT_START(base) \
    /*    prefix  ElemType    base type   trie name               update flag type    element address */ \
    ENTRY(prefix, s32,  rk_s32,     low_delay,              ELEM_FLAG_START,    base, low_delay) \
    STRUCT_END(base) \
    STRUCT_START(rc) \
    ENTRY(prefix, s32,  rk_s32,     mode,                   ELEM_FLAG_UPDATE,   rc, rc_mode) \
    ENTRY(prefix, s32,  rk_s32,     bps_target,             ELEM_FLAG_UPDATE,   rc, bps_target) \
    ENTRY(prefix, s32,  rk_s32,     bps_max,                ELEM_FLAG_HOLD,     rc, bps_max) \
    ENTRY(prefix, s32,  rk_s32,     bps_min,                ELEM_FLAG_HOLD,     rc, bps_min) \
    ENTRY(prefix, s32,  rk_s32,     fps_in_flex,            ELEM_FLAG_UPDATE,   rc, fps_in_flex) \
    ENTRY(prefix, s32,  rk_s32,     fps_in_num,             ELEM_FLAG_HOLD,     rc, fps_in_num) \
    ENTRY(prefix, s32,  rk_s32,     fps_in_denorm,          ELEM_FLAG_HOLD,     rc, fps_in_denom) \
    ALIAS(prefix, s32,  rk_s32,     fps_in_denom,           ELEM_FLAG_HOLD,     rc, fps_in_denom) \
    ENTRY(prefix, s32,  rk_s32,     fps_out_flex,           ELEM_FLAG_UPDATE,   rc, fps_out_flex) \
    ENTRY(prefix, s32,  rk_s32,     fps_out_num,            ELEM_FLAG_HOLD,     rc, fps_out_num) \
    ENTRY(prefix, s32,  rk_s32,     fps_out_denorm,         ELEM_FLAG_HOLD,     rc, fps_out_denom) \
    ALIAS(prefix, s32,  rk_s32,     fps_out_denom,          ELEM_FLAG_HOLD,     rc, fps_out_denom) \
    ENTRY(prefix, s32,  rk_s32,     fps_chg_no_idr,         ELEM_FLAG_HOLD,     rc, fps_chg_no_idr) \
    ENTRY(prefix, s32,  rk_s32,     gop,                    ELEM_FLAG_UPDATE,   rc, gop) \
    STRCT(prefix, shm,  KmppShmPtr, ref_cfg,                ELEM_FLAG_UPDATE,   rc, ref_cfg) \
    ENTRY(prefix, u32,  rk_u32,     max_reenc_times,        ELEM_FLAG_UPDATE,   rc, max_reenc_times) \
    ENTRY(prefix, u32,  rk_u32,     priority,               ELEM_FLAG_UPDATE,   rc, rc_priority) \
    ENTRY(prefix, u32,  rk_u32,     drop_mode,              ELEM_FLAG_UPDATE,   rc, drop_mode) \
    ENTRY(prefix, u32,  rk_u32,     drop_thd,               ELEM_FLAG_HOLD,     rc, drop_threshold) \
    ENTRY(prefix, u32,  rk_u32,     drop_gap,               ELEM_FLAG_HOLD,     rc, drop_gap) \
    ENTRY(prefix, s32,  rk_s32,     max_i_prop,             ELEM_FLAG_UPDATE,   rc, max_i_prop) \
    ENTRY(prefix, s32,  rk_s32,     min_i_prop,             ELEM_FLAG_HOLD,     rc, min_i_prop) \
    ENTRY(prefix, s32,  rk_s32,     init_ip_ratio,          ELEM_FLAG_UPDATE,   rc, init_ip_ratio) \
    ENTRY(prefix, u32,  rk_u32,     super_mode,             ELEM_FLAG_UPDATE,   rc, super_mode) \
    ENTRY(prefix, u32,  rk_u32,     super_i_thd,            ELEM_FLAG_HOLD,     rc, super_i_thd) \
    ENTRY(prefix, u32,  rk_u32,     super_p_thd,            ELEM_FLAG_HOLD,     rc, super_p_thd) \
    ENTRY(prefix, u32,  rk_u32,     debreath_en,            ELEM_FLAG_UPDATE,   rc, debreath_en) \
    ENTRY(prefix, u32,  rk_u32,     debreath_strength,      ELEM_FLAG_HOLD,     rc, debre_strength) \
    ENTRY(prefix, s32,  rk_s32,     qp_init,                (ELEM_FLAG_UPDATE|ELEM_FLAG_RECORD_0),  rc, qp_init) \
    ENTRY(prefix, s32,  rk_s32,     qp_min,                 (ELEM_FLAG_UPDATE|ELEM_FLAG_RECORD_1),  rc, qp_min) \
    ENTRY(prefix, s32,  rk_s32,     qp_max,                 (ELEM_FLAG_HOLD),   rc, qp_max) \
    ENTRY(prefix, s32,  rk_s32,     qp_min_i,               (ELEM_FLAG_UPDATE|ELEM_FLAG_RECORD_2),  rc, qp_min_i) \
    ENTRY(prefix, s32,  rk_s32,     qp_max_i,               (ELEM_FLAG_HOLD),   rc, qp_max_i) \
    ENTRY(prefix, s32,  rk_s32,     qp_step,                (ELEM_FLAG_UPDATE|ELEM_FLAG_RECORD_3),  rc, qp_max_step) \
    ENTRY(prefix, s32,  rk_s32,     qp_ip,                  (ELEM_FLAG_UPDATE|ELEM_FLAG_RECORD_4),  rc, qp_delta_ip) \
    ENTRY(prefix, s32,  rk_s32,     qp_vi,                  ELEM_FLAG_UPDATE,   rc, qp_delta_vi) \
    ENTRY(prefix, s32,  rk_s32,     hier_qp_en,             ELEM_FLAG_UPDATE,   rc, hier_qp_en) \
    STRCT(prefix, st,   void *,     hier_qp_delta,          ELEM_FLAG_HOLD,     rc, hier_qp_delta); \
    STRCT(prefix, st,   void *,     hier_frame_num,         ELEM_FLAG_HOLD,     rc, hier_frame_num); \
    ENTRY(prefix, s32,  rk_s32,     stats_time,             ELEM_FLAG_UPDATE,   rc, stats_time) \
    ENTRY(prefix, u32,  rk_u32,     refresh_en,             ELEM_FLAG_UPDATE,   rc, refresh_en) \
    ENTRY(prefix, u32,  rk_u32,     refresh_mode,           ELEM_FLAG_HOLD,     rc, refresh_mode) \
    ENTRY(prefix, u32,  rk_u32,     refresh_num,            ELEM_FLAG_HOLD,     rc, refresh_num) \
    ENTRY(prefix, s32,  rk_s32,     fqp_min_i,              ELEM_FLAG_UPDATE,   rc, fqp_min_i) \
    ENTRY(prefix, s32,  rk_s32,     fqp_min_p,              ELEM_FLAG_HOLD,     rc, fqp_min_p) \
    ENTRY(prefix, s32,  rk_s32,     fqp_max_i,              ELEM_FLAG_HOLD,     rc, fqp_max_i) \
    ENTRY(prefix, s32,  rk_s32,     fqp_max_p,              ELEM_FLAG_HOLD,     rc, fqp_max_p) \
    ENTRY(prefix, s32,  rk_s32,     mt_st_swth_frm_qp,      ELEM_FLAG_HOLD,     rc, mt_st_swth_frm_qp) \
    STRUCT_END(rc) \
    STRUCT_START(prep); \
    ENTRY(prefix, s32,  rk_s32,     width,                  ELEM_FLAG_UPDATE,   prep, width); \
    ENTRY(prefix, s32,  rk_s32,     height,                 ELEM_FLAG_HOLD,     prep, height); \
    ENTRY(prefix, s32,  rk_s32,     max_width,              ELEM_FLAG_HOLD,     prep, max_width); \
    ENTRY(prefix, s32,  rk_s32,     max_height,             ELEM_FLAG_HOLD,     prep, max_height); \
    ENTRY(prefix, s32,  rk_s32,     hor_stride,             ELEM_FLAG_HOLD,     prep, hor_stride); \
    ENTRY(prefix, s32,  rk_s32,     ver_stride,             ELEM_FLAG_HOLD,     prep, ver_stride); \
    ENTRY(prefix, s32,  rk_s32,     format,                 ELEM_FLAG_UPDATE,   prep, format); \
    ENTRY(prefix, s32,  rk_s32,     format_out,             ELEM_FLAG_HOLD,     prep, format_out); \
    ENTRY(prefix, s32,  rk_s32,     chroma_ds_mode,         ELEM_FLAG_HOLD,     prep, chroma_ds_mode); \
    ENTRY(prefix, s32,  rk_s32,     fix_chroma_en,          ELEM_FLAG_HOLD,     prep, fix_chroma_en); \
    ENTRY(prefix, s32,  rk_s32,     fix_chroma_u,           ELEM_FLAG_HOLD,     prep, fix_chroma_u); \
    ENTRY(prefix, s32,  rk_s32,     fix_chroma_v,           ELEM_FLAG_HOLD,     prep, fix_chroma_v); \
    ENTRY(prefix, s32,  rk_s32,     colorspace,             ELEM_FLAG_UPDATE,   prep, color); \
    ENTRY(prefix, s32,  rk_s32,     colorprim,              ELEM_FLAG_HOLD,     prep, colorprim); \
    ENTRY(prefix, s32,  rk_s32,     colortrc,               ELEM_FLAG_UPDATE,   prep, colortrc); \
    ENTRY(prefix, s32,  rk_s32,     colorrange,             ELEM_FLAG_UPDATE,   prep, range); \
    ALIAS(prefix, s32,  rk_s32,     range,                  ELEM_FLAG_HOLD,     prep, range); \
    ENTRY(prefix, s32,  rk_s32,     range_out,              ELEM_FLAG_HOLD,     prep, range_out); \
    ENTRY(prefix, s32,  rk_s32,     rotation,               ELEM_FLAG_UPDATE,   prep, rotation_ext); \
    ENTRY(prefix, s32,  rk_s32,     mirroring,              ELEM_FLAG_UPDATE,   prep, mirroring_ext); \
    ENTRY(prefix, s32,  rk_s32,     flip,                   ELEM_FLAG_UPDATE,   prep, flip); \
    STRUCT_END(prep); \
    STRUCT_START(codec); \
    ENTRY(prefix, s32,  rk_s32,     type,                   ELEM_FLAG_UPDATE,   codec, coding); \
    STRUCT_END(codec); \
    STRUCT_START(h264); \
    /* h264 config */ \
    ENTRY(prefix, s32,  rk_s32,     stream_type,            ELEM_FLAG_UPDATE,   codec, h264, stream_type); \
    ENTRY(prefix, s32,  rk_s32,     profile,                ELEM_FLAG_UPDATE,   codec, h264, profile); \
    ENTRY(prefix, s32,  rk_s32,     level,                  ELEM_FLAG_HOLD,     codec, h264, level); \
    ENTRY(prefix, u32,  rk_u32,     poc_type,               ELEM_FLAG_UPDATE,   codec, h264, poc_type); \
    ENTRY(prefix, u32,  rk_u32,     log2_max_poc_lsb,       ELEM_FLAG_UPDATE,   codec, h264, log2_max_poc_lsb); \
    ENTRY(prefix, u32,  rk_u32,     log2_max_frm_num,       ELEM_FLAG_UPDATE,   codec, h264, log2_max_frame_num); \
    ENTRY(prefix, u32,  rk_u32,     gaps_not_allowed,       ELEM_FLAG_UPDATE,   codec, h264, gaps_not_allowed); \
    ENTRY(prefix, s32,  rk_s32,     cabac_en,               ELEM_FLAG_UPDATE,   codec, h264, entropy_coding_mode_ex); \
    ENTRY(prefix, s32,  rk_s32,     cabac_idc,              ELEM_FLAG_HOLD,     codec, h264, cabac_init_idc_ex); \
    ENTRY(prefix, s32,  rk_s32,     trans8x8,               ELEM_FLAG_UPDATE,   codec, h264, transform8x8_mode_ex); \
    ENTRY(prefix, s32,  rk_s32,     const_intra,            ELEM_FLAG_UPDATE,   codec, h264, constrained_intra_pred_mode); \
    ENTRY(prefix, s32,  rk_s32,     scaling_list,           ELEM_FLAG_UPDATE,   codec, h264, scaling_list_mode); \
    ENTRY(prefix, s32,  rk_s32,     cb_qp_offset,           ELEM_FLAG_UPDATE,   codec, h264, chroma_cb_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     cr_qp_offset,           ELEM_FLAG_HOLD,     codec, h264, chroma_cr_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     dblk_disable,           ELEM_FLAG_UPDATE,   codec, h264, deblock_disable); \
    ENTRY(prefix, s32,  rk_s32,     dblk_alpha,             ELEM_FLAG_HOLD,     codec, h264, deblock_offset_alpha); \
    ENTRY(prefix, s32,  rk_s32,     dblk_beta,              ELEM_FLAG_HOLD,     codec, h264, deblock_offset_beta); \
    ALIAS(prefix, s32,  rk_s32,     qp_init,                ELEM_FLAG_REPLAY_0, rc, qp_init); \
    ALIAS(prefix, s32,  rk_s32,     qp_min,                 ELEM_FLAG_REPLAY_1, rc, qp_min); \
    ALIAS(prefix, s32,  rk_s32,     qp_max,                 ELEM_FLAG_REPLAY_1, rc, qp_max); \
    ALIAS(prefix, s32,  rk_s32,     qp_min_i,               ELEM_FLAG_REPLAY_2, rc, qp_min_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_max_i,               ELEM_FLAG_REPLAY_2, rc, qp_max_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_step,                ELEM_FLAG_REPLAY_3, rc, qp_max_step); \
    ALIAS(prefix, s32,  rk_s32,     qp_delta_ip,            ELEM_FLAG_REPLAY_4, rc, qp_delta_ip); \
    ENTRY(prefix, s32,  rk_s32,     max_tid,                ELEM_FLAG_UPDATE,   codec, h264, max_tid); \
    ENTRY(prefix, s32,  rk_s32,     max_ltr,                ELEM_FLAG_UPDATE,   codec, h264, max_ltr_frames); \
    ENTRY(prefix, s32,  rk_s32,     prefix_mode,            ELEM_FLAG_UPDATE,   codec, h264, prefix_mode); \
    ENTRY(prefix, s32,  rk_s32,     base_layer_pid,         ELEM_FLAG_UPDATE,   codec, h264, base_layer_pid); \
    ENTRY(prefix, u32,  rk_u32,     constraint_set,         ELEM_FLAG_UPDATE,   codec, h264, constraint_set); \
    STRUCT_END(h264); \
    /* h265 config*/ \
    STRUCT_START(h265); \
    ENTRY(prefix, s32,  rk_s32,     profile,                ELEM_FLAG_UPDATE,   codec, h265, profile); \
    ENTRY(prefix, s32,  rk_s32,     tier,                   ELEM_FLAG_HOLD,     codec, h265, tier); \
    ENTRY(prefix, s32,  rk_s32,     level,                  ELEM_FLAG_HOLD,     codec, h265, level); \
    ENTRY(prefix, u32,  rk_u32,     scaling_list,           ELEM_FLAG_UPDATE,   codec, h265, trans_cfg, scaling_list_mode); \
    ENTRY(prefix, s32,  rk_s32,     cb_qp_offset,           ELEM_FLAG_HOLD,     codec, h265, trans_cfg, cb_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     cr_qp_offset,           ELEM_FLAG_HOLD,     codec, h265, trans_cfg, cr_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     diff_cu_qp_delta_depth, ELEM_FLAG_HOLD,     codec, h265, trans_cfg, diff_cu_qp_delta_depth); \
    ENTRY(prefix, u32,  rk_u32,     dblk_disable,           ELEM_FLAG_UPDATE,   codec, h265, dblk_cfg, slice_deblocking_filter_disabled_flag); \
    ENTRY(prefix, s32,  rk_s32,     dblk_alpha,             ELEM_FLAG_HOLD,     codec, h265, dblk_cfg, slice_beta_offset_div2); \
    ENTRY(prefix, s32,  rk_s32,     dblk_beta,              ELEM_FLAG_HOLD,     codec, h265, dblk_cfg, slice_tc_offset_div2); \
    ALIAS(prefix, s32,  rk_s32,     qp_init,                ELEM_FLAG_REPLAY_0, rc, qp_init); \
    ALIAS(prefix, s32,  rk_s32,     qp_min,                 ELEM_FLAG_REPLAY_1, rc, qp_min); \
    ALIAS(prefix, s32,  rk_s32,     qp_max,                 ELEM_FLAG_REPLAY_1, rc, qp_max); \
    ALIAS(prefix, s32,  rk_s32,     qp_min_i,               ELEM_FLAG_REPLAY_2, rc, qp_min_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_max_i,               ELEM_FLAG_REPLAY_2, rc, qp_max_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_step,                ELEM_FLAG_REPLAY_3, rc, qp_max_step); \
    ALIAS(prefix, s32,  rk_s32,     qp_delta_ip,            ELEM_FLAG_REPLAY_4, rc, qp_delta_ip); \
    ENTRY(prefix, s32,  rk_s32,     sao_luma_disable,       ELEM_FLAG_UPDATE,   codec, h265, sao_cfg, slice_sao_luma_disable); \
    ENTRY(prefix, s32,  rk_s32,     sao_chroma_disable,     ELEM_FLAG_HOLD,     codec, h265, sao_cfg, slice_sao_chroma_disable); \
    ENTRY(prefix, s32,  rk_s32,     sao_bit_ratio,          ELEM_FLAG_HOLD,     codec, h265, sao_cfg, sao_bit_ratio); \
    ENTRY(prefix, u32,  rk_u32,     lpf_acs_sli_en,         ELEM_FLAG_UPDATE,   codec, h265, lpf_acs_sli_en); \
    ENTRY(prefix, u32,  rk_u32,     lpf_acs_tile_disable,   ELEM_FLAG_UPDATE,   codec, h265, lpf_acs_tile_disable); \
    ENTRY(prefix, s32,  rk_s32,     auto_tile,              ELEM_FLAG_UPDATE,   codec, h265, auto_tile); \
    ENTRY(prefix, s32,  rk_s32,     max_tid,                ELEM_FLAG_UPDATE,   codec, h265, max_tid); \
    ENTRY(prefix, s32,  rk_s32,     max_ltr,                ELEM_FLAG_UPDATE,   codec, h265, max_ltr_frames); \
    ENTRY(prefix, s32,  rk_s32,     base_layer_pid,         ELEM_FLAG_UPDATE,   codec, h265, base_layer_pid); \
    ENTRY(prefix, s32,  rk_s32,     const_intra,            ELEM_FLAG_UPDATE,   codec, h265, const_intra_pred); \
    ENTRY(prefix, s32,  rk_s32,     lcu_size,               ELEM_FLAG_UPDATE,   codec, h265, max_cu_size); \
    STRUCT_END(h265); \
    /* vp8 config */ \
    STRUCT_START(vp8); \
    ALIAS(prefix, s32,  rk_s32,     qp_init,                ELEM_FLAG_REPLAY_0, rc, qp_init); \
    ALIAS(prefix, s32,  rk_s32,     qp_min,                 ELEM_FLAG_REPLAY_1, rc, qp_min); \
    ALIAS(prefix, s32,  rk_s32,     qp_max,                 ELEM_FLAG_REPLAY_1, rc, qp_max); \
    ALIAS(prefix, s32,  rk_s32,     qp_min_i,               ELEM_FLAG_REPLAY_2, rc, qp_min_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_max_i,               ELEM_FLAG_REPLAY_2, rc, qp_max_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_step,                ELEM_FLAG_REPLAY_3, rc, qp_max_step); \
    ALIAS(prefix, s32,  rk_s32,     qp_delta_ip,            ELEM_FLAG_REPLAY_4, rc, qp_delta_ip); \
    ENTRY(prefix, s32,  rk_s32,     disable_ivf,            ELEM_FLAG_UPDATE,   codec, vp8, disable_ivf); \
    STRUCT_END(vp8); \
    /* jpeg config */ \
    STRUCT_START(jpeg); \
    ENTRY(prefix, s32,  rk_s32,     quant,                  ELEM_FLAG_UPDATE,   codec, jpeg, quant); \
    ENTRY(prefix, kptr, void *,     qtable_y,               ELEM_FLAG_UPDATE,   codec, jpeg, qtable_y); \
    ENTRY(prefix, kptr, void *,     qtable_u,               ELEM_FLAG_HOLD,     codec, jpeg, qtable_u); \
    ENTRY(prefix, kptr, void *,     qtable_v,               ELEM_FLAG_HOLD,     codec, jpeg, qtable_v); \
    ENTRY(prefix, s32,  rk_s32,     q_factor,               ELEM_FLAG_UPDATE,   codec, jpeg, q_factor); \
    ENTRY(prefix, s32,  rk_s32,     qf_max,                 ELEM_FLAG_HOLD,     codec, jpeg, qf_max); \
    ENTRY(prefix, s32,  rk_s32,     qf_min,                 ELEM_FLAG_HOLD,     codec, jpeg, qf_min); \
    STRUCT_END(jpeg); \
    /* split config */ \
    STRUCT_START(split); \
    ENTRY(prefix, u32,  rk_u32,     mode,                   ELEM_FLAG_UPDATE,   split, split_mode) \
    ENTRY(prefix, u32,  rk_u32,     arg,                    ELEM_FLAG_UPDATE,   split, split_arg) \
    ENTRY(prefix, u32,  rk_u32,     out,                    ELEM_FLAG_UPDATE,   split, split_out) \
    STRUCT_END(split); \
    /* hardware detail config */ \
    STRUCT_START(hw); \
    ENTRY(prefix, s32,  rk_s32,     qp_row,                 ELEM_FLAG_UPDATE,   hw, qp_delta_row) \
    ENTRY(prefix, s32,  rk_s32,     qp_row_i,               ELEM_FLAG_UPDATE,   hw, qp_delta_row_i) \
    STRCT(prefix, st,   void *,     aq_thrd_i,              ELEM_FLAG_UPDATE,   hw, aq_thrd_i) \
    STRCT(prefix, st,   void *,     aq_thrd_p,              ELEM_FLAG_UPDATE,   hw, aq_thrd_p) \
    STRCT(prefix, st,   void *,     aq_step_i,              ELEM_FLAG_UPDATE,   hw, aq_step_i) \
    STRCT(prefix, st,   void *,     aq_step_p,              ELEM_FLAG_UPDATE,   hw, aq_step_p) \
    ENTRY(prefix, s32,  rk_s32,     mb_rc_disable,          ELEM_FLAG_UPDATE,   hw, mb_rc_disable) \
    STRCT(prefix, st,   void *,     aq_rnge_arr,            ELEM_FLAG_UPDATE,   hw, aq_rnge_arr) \
    STRCT(prefix, st,   void *,     mode_bias,              ELEM_FLAG_UPDATE,   hw, mode_bias) \
    ENTRY(prefix, s32,  rk_s32,     skip_bias_en,           ELEM_FLAG_UPDATE,   hw, skip_bias_en) \
    ENTRY(prefix, s32,  rk_s32,     skip_sad,               ELEM_FLAG_HOLD,     hw, skip_sad) \
    ENTRY(prefix, s32,  rk_s32,     skip_bias,              ELEM_FLAG_HOLD,     hw, skip_bias) \
    ENTRY(prefix, s32,  rk_s32,     qbias_i,                ELEM_FLAG_UPDATE,   hw, qbias_i) \
    ENTRY(prefix, s32,  rk_s32,     qbias_p,                ELEM_FLAG_UPDATE,   hw, qbias_p) \
    ENTRY(prefix, s32,  rk_s32,     qbias_en,               ELEM_FLAG_UPDATE,   hw, qbias_en) \
    STRCT(prefix, st,   void *,     qbias_arr,              ELEM_FLAG_UPDATE,   hw, qbias_arr) \
    ENTRY(prefix, s32,  rk_s32,     flt_str_i,              ELEM_FLAG_UPDATE,   hw, flt_str_i) \
    ENTRY(prefix, s32,  rk_s32,     flt_str_p,              ELEM_FLAG_HOLD,     hw, flt_str_p) \
    STRUCT_END(hw); \
    /* quality fine tuning config */ \
    STRUCT_START(tune); \
    ENTRY(prefix, s32,  rk_s32,     scene_mode,             ELEM_FLAG_UPDATE,   tune, scene_mode); \
    ENTRY(prefix, s32,  rk_s32,     deblur_en,              ELEM_FLAG_UPDATE,   tune, deblur_en); \
    ENTRY(prefix, s32,  rk_s32,     deblur_str,             ELEM_FLAG_UPDATE,   tune, deblur_str); \
    ENTRY(prefix, s32,  rk_s32,     anti_flicker_str,       ELEM_FLAG_UPDATE,   tune, anti_flicker_str); \
    ENTRY(prefix, s32,  rk_s32,     lambda_idx_i,           ELEM_FLAG_UPDATE,   tune, lambda_idx_i); \
    ENTRY(prefix, s32,  rk_s32,     lambda_idx_p,           ELEM_FLAG_UPDATE,   tune, lambda_idx_p); \
    ENTRY(prefix, s32,  rk_s32,     atr_str_i,              ELEM_FLAG_UPDATE,   tune, atr_str_i); \
    ENTRY(prefix, s32,  rk_s32,     atr_str_p,              ELEM_FLAG_UPDATE,   tune, atr_str_p); \
    ENTRY(prefix, s32,  rk_s32,     atl_str,                ELEM_FLAG_UPDATE,   tune, atl_str); \
    ENTRY(prefix, s32,  rk_s32,     sao_str_i,              ELEM_FLAG_UPDATE,   tune, sao_str_i); \
    ENTRY(prefix, s32,  rk_s32,     sao_str_p,              ELEM_FLAG_UPDATE,   tune, sao_str_p); \
    ENTRY(prefix, s32,  rk_s32,     rc_container,           ELEM_FLAG_UPDATE,   tune, rc_container); \
    ENTRY(prefix, s32,  rk_s32,     vmaf_opt,               ELEM_FLAG_UPDATE,   tune, vmaf_opt); \
    ENTRY(prefix, s32,  rk_s32,     motion_static_switch_enable, ELEM_FLAG_UPDATE, tune, motion_static_switch_enable); \
    ENTRY(prefix, s32,  rk_s32,     atf_str,                ELEM_FLAG_UPDATE,   tune, atf_str); \
    ENTRY(prefix, s32,  rk_s32,     lgt_chg_lvl,            ELEM_FLAG_UPDATE,   tune, lgt_chg_lvl); \
    ENTRY(prefix, s32,  rk_s32,     static_frm_num,         ELEM_FLAG_UPDATE,   tune, static_frm_num); \
    ENTRY(prefix, s32,  rk_s32,     madp16_th,              ELEM_FLAG_UPDATE,   tune, madp16_th); \
    ENTRY(prefix, s32,  rk_s32,     skip16_wgt,             ELEM_FLAG_UPDATE,   tune, skip16_wgt); \
    ENTRY(prefix, s32,  rk_s32,     skip32_wgt,             ELEM_FLAG_UPDATE,   tune, skip32_wgt); \
    ENTRY(prefix, s32,  rk_s32,     speed,                  ELEM_FLAG_UPDATE,   tune, speed); \
    STRUCT_END(tune); \
    CFG_DEF_END()

#define KMPP_OBJ_NAME               kmpp_venc_st_cfg
#define KMPP_OBJ_INTF_TYPE          KmppVencStCfg
#define KMPP_OBJ_IMPL_TYPE          MppEncCfgSet
#define KMPP_OBJ_FUNC_INIT          kmpp_venc_st_cfg_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_venc_st_cfg_impl_deinit
#define KMPP_OBJ_FUNC_DUMP          kmpp_venc_st_cfg_impl_dump
#define KMPP_OBJ_ENTRY_TABLE        KMPP_VENC_ST_CFG_TABLE
#define KMPP_OBJ_ACCESS_DISABLE
#define KMPP_OBJ_HIERARCHY_ENABLE
#include "kmpp_obj_helper.h"
