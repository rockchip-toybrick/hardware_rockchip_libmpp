/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#if defined(_WIN32)
#include "vld.h"
#endif

#define MODULE_TAG "mpi_enc_slt_test"
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <math.h>
#include "rk_mpi.h"
#include "rk_venc_kcfg.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_soc.h"

#include "utils.h"
#include "mpi_enc_utils.h"
#include "camera_source.h"
#include "mpp_enc_roi_utils.h"
#include "mpi_enc_slt_utils.h"
#include "mpp_rc_api.h"

static RK_S32 qbias_arr_hevc[18] = {
    3, 6, 13, 171, 171, 171, 171,
    3, 6, 13, 171, 171, 220, 171, 85, 85, 85, 85
};

static RK_S32 qbias_arr_avc[18] = {
    3, 6, 13, 683, 683, 683, 683,
    3, 6, 13, 683, 683, 683, 683, 341, 341, 341, 341
};

static RK_S32 aq_rnge_arr[10] = {
    5, 5, 10, 12, 12,
    5, 5, 10, 12, 12
};

static RK_S32 aq_thd_smart[16] = {
    1,  3,  3,  3,  3,  3,  5,  5,
    8,  8,  8, 15, 15, 20, 25, 28
};

static RK_S32 aq_step_smart[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    0,  1,  2,  3,  4,  6,  8, 10
};

typedef struct {
    // base flow context
    MppCtx ctx;
    MppApi *mpi;
    RK_S32 chn;

    // global flow control flag
    RK_U32 frm_eos;
    RK_U32 pkt_eos;
    RK_U32 frm_pkt_cnt;
    RK_S32 frame_num;
    RK_S32 frame_count;
    RK_S32 frm_step;

    RK_U64 stream_size;
    /* end of encoding flag when set quit the loop */
    volatile RK_U32 loop_end;

    // src and dst
    FILE *fp_input;
    FILE *fp_output;
    FILE *fp_verify;
    FILE *fp_slt_gold;
    FILE *fp_slt_result;

    /* encoder config set */
    MppEncCfg       cfg;
    MppEncPrepCfg   prep_cfg;
    MppEncRcCfg     rc_cfg;
    MppEncCodecCfg  codec_cfg;
    MppEncSliceSplit split_cfg;
    MppEncOSDPltCfg osd_plt_cfg;
    MppEncOSDPlt    osd_plt;
    MppEncOSDData   osd_data;
    RoiRegionCfg    roi_region;
    MppEncROICfg    roi_cfg;

    // input / output
    MppBufferGroup buf_grp;
    MppBuffer frm_buf;
    MppBuffer pkt_buf;
    MppBuffer md_info;
    MppEncSeiMode sei_mode;
    MppEncHeaderMode header_mode;

    // paramter for resource malloc
    RK_U32 width;
    RK_U32 height;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    MppFrameFormat fmt;
    MppCodingType type;
    RK_S32 loop_times;
    CamSource *cam_ctx;
    MppEncRoiCtx roi_ctx;

    MppVencKcfg init_kcfg;

    // resources
    size_t header_size;
    size_t frame_size;
    size_t mdinfo_size;
    /* NOTE: packet buffer may overflow */
    size_t packet_size;

    RK_U32 osd_enable;
    RK_U32 osd_mode;
    RK_U32 split_mode;
    RK_U32 split_arg;
    RK_U32 split_out;

    RK_U32 user_data_enable;
    RK_U32 roi_enable;

    // rate control runtime parameter
    RK_S32 fps_in_flex;
    RK_S32 fps_in_den;
    RK_S32 fps_in_num;
    RK_S32 fps_out_flex;
    RK_S32 fps_out_den;
    RK_S32 fps_out_num;
    RK_S32 bps;
    RK_S32 bps_max;
    RK_S32 bps_min;
    RK_S32 rc_mode;
    RK_S32 gop_mode;
    RK_S32 gop_len;
    RK_S32 vi_len;
    RK_S32 scene_mode;
    RK_S32 deblur_en;

    RK_S32 cu_qp_delta_depth;
    RK_S32 anti_flicker_str;
    RK_S32 atr_str_i;
    RK_S32 atr_str_p;
    RK_S32 atl_str;
    RK_S32 sao_str_i;
    RK_S32 sao_str_p;
    RK_S64 first_frm;
    RK_S64 first_pkt;
    RK_U32 hw_status;
    RK_U32 total_hw_time;
} MpiEncTestData;

/* For each instance thread return value */
typedef struct {
    float           frame_rate;
    RK_U64          bit_rate;
    RK_S64          elapsed_time;
    RK_S32          frame_count;
    RK_S64          stream_size;
    RK_S64          delay;
    RK_U32          verify;
    RK_U32          hw_status;
    RK_U32          total_hw_time;
    RK_FLOAT        hw_fps;
} MpiEncMultiCtxRet;

typedef struct {
    MpiEncTestArgs      *cmd;       // pointer to global command line info
    const char          *name;
    RK_S32              chn;

    pthread_t           thd;        // thread for for each instance
    MpiEncTestData      ctx;        // context of encoder
    MpiEncMultiCtxRet   ret;        // return of encoder
} MpiEncMultiCtxInfo;

static RK_S32 aq_thd[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 25
};

static RK_S32 aq_step_i_ipc[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    5,  7,  7,  8,
};

static RK_S32 aq_step_p_ipc[16] = {
    -8, -7, -6, -5,
    -4, -2, -1, -1,
    0,  2,  3,  4,
    6,  8,  9,  10,
};

static RK_S32 get_mdinfo_size(MpiEncTestData *p, MppCodingType type)
{
    RockchipSocType soc_type = mpp_get_soc_type();
    RK_S32 md_size;
    RK_U32 w = p->hor_stride, h = p->ver_stride;

    if (soc_type == ROCKCHIP_SOC_RV1126B) {
        md_size = (MPP_VIDEO_CodingHEVC == type) ?
                  (MPP_ALIGN(w, 32) >> 5) * (MPP_ALIGN(h, 32) >> 5) * 20 :
                  (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 16) >> 4) * 16;
    } else if (soc_type == ROCKCHIP_SOC_RK3588) {
        md_size = (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 64) >> 6) * 32;
    } else {
        md_size = (MPP_VIDEO_CodingHEVC == type) ?
                  (MPP_ALIGN(w, 32) >> 5) * (MPP_ALIGN(h, 32) >> 5) * 16 :
                  (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 16) >> 4) * 16;
    }

    return md_size;
}

static MPP_RET kmpp_cfg_init(MpiEncMultiCtxInfo *info)
{
    MppVencKcfg init_kcfg = NULL;
    MpiEncTestData *p = &info->ctx;
    MPP_RET ret = MPP_NOK;

    mpp_venc_kcfg_init(&init_kcfg, MPP_VENC_KCFG_TYPE_INIT);
    if (!init_kcfg) {
        mpp_err_f("kmpp_venc_init_cfg_init failed\n");
        return ret;
    }

    p->init_kcfg = init_kcfg;

    mpp_venc_kcfg_set_u32(init_kcfg, "type", MPP_CTX_ENC);
    mpp_venc_kcfg_set_u32(init_kcfg, "coding", p->type);
    mpp_venc_kcfg_set_s32(init_kcfg, "chan_id", 0);
    mpp_venc_kcfg_set_s32(init_kcfg, "online", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "buf_size", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_strm_cnt", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "shared_buf_en", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "smart_en", p->rc_mode == MPP_ENC_RC_MODE_SMTRC);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_width", p->width);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_height", p->height);
    mpp_venc_kcfg_set_u32(init_kcfg, "max_lt_cnt", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "qpmap_en", p->deblur_en);
    mpp_venc_kcfg_set_u32(init_kcfg, "chan_dup", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "tmvp_enable", 0);
    mpp_venc_kcfg_set_u32(init_kcfg, "only_smartp", 0);

    ret = p->mpi->control(p->ctx, MPP_SET_VENC_INIT_KCFG, init_kcfg);
    if (ret)
        mpp_err_f("mpi control set kmpp enc cfg failed ret %d\n", ret);

    return ret;
}

MPP_RET test_ctx_init(MpiEncMultiCtxInfo *info)
{
    MpiEncTestArgs *cmd = info->cmd;
    MpiEncTestData *p = &info->ctx;
    MPP_RET ret = MPP_OK;

    // get paramter from cmd
    p->width        = cmd->width;
    p->height       = cmd->height;
    p->hor_stride   = (cmd->hor_stride) ? (cmd->hor_stride) :
                      (MPP_ALIGN(cmd->width, 16));
    p->ver_stride   = (cmd->ver_stride) ? (cmd->ver_stride) :
                      (MPP_ALIGN(cmd->height, 16));
    p->fmt          = cmd->format;
    p->type         = cmd->type;
    p->bps          = cmd->bps_target;
    p->bps_min      = cmd->bps_min;
    p->bps_max      = cmd->bps_max;
    p->rc_mode      = cmd->rc_mode;
    p->frame_num    = cmd->frame_num;
    if (cmd->type == MPP_VIDEO_CodingMJPEG && p->frame_num == 0) {
        mpp_log("jpege default encode only one frame. Use -n [num] for rc case\n");
        p->frame_num = 1;
    }

    p->frm_step     = cmd->frm_step;
    p->gop_mode     = cmd->gop_mode;
    p->gop_len      = cmd->gop_len;
    p->vi_len       = cmd->vi_len;
    p->fps_in_flex  = cmd->fps_in_flex;
    p->fps_in_den   = cmd->fps_in_den;
    p->fps_in_num   = cmd->fps_in_num;
    p->fps_out_flex = cmd->fps_out_flex;
    p->fps_out_den  = cmd->fps_out_den;
    p->fps_out_num  = cmd->fps_out_num;
    p->scene_mode   = cmd->scene_mode;
    p->deblur_en    = cmd->deblur_en;
    p->cu_qp_delta_depth = cmd->cu_qp_delta_depth;
    p->anti_flicker_str = cmd->anti_flicker_str;
    p->atr_str_i = cmd->atr_str_i;
    p->atr_str_p = cmd->atr_str_p;
    p->atl_str = cmd->atl_str;
    p->sao_str_i = cmd->sao_str_i;
    p->sao_str_p = cmd->sao_str_p;
    p->mdinfo_size  = get_mdinfo_size(p, cmd->type);

    if (cmd->file_input) {
        if (!strncmp(cmd->file_input, "/dev/video", 10)) {
            mpp_log("open camera device");
            p->cam_ctx = camera_source_init(cmd->file_input, 4, p->width, p->height, p->fmt);
            mpp_log("new framecap ok");
            if (p->cam_ctx == NULL)
                mpp_err("open %s fail", cmd->file_input);
        } else {
            p->fp_input = fopen(cmd->file_input, "rb");
            if (NULL == p->fp_input) {
                mpp_err("Cpu create default source image for test\n");
            }
        }
    }

    if (cmd->file_output) {
        p->fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == p->fp_output) {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            ret = MPP_ERR_OPEN_FILE;
        }
    }

    if (cmd->file_slt) {
        p->fp_verify = fopen(cmd->file_slt, "wt");
        if (!p->fp_verify)
            mpp_err("failed to open verify file %s\n", cmd->file_slt);

        p->fp_slt_gold = fopen(cmd->file_slt_gold, "rt");
        if (!p->fp_slt_gold)
            mpp_err("failed to open slt gold file.\n");
    } else {
        p->fp_verify = NULL;
        p->fp_slt_gold = NULL;
        p->fp_slt_result = NULL;
    }

    // update resource parameter
    switch (p->fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY :
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP : {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 2;
    } break;
    case MPP_FMT_YUV400:
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 :
    case MPP_FMT_RGB888 :
    case MPP_FMT_BGR888 :
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64);
    } break;

    default: {
        p->frame_size = MPP_ALIGN(p->hor_stride, 64) * MPP_ALIGN(p->ver_stride, 64) * 4;
    } break;
    }

    if (MPP_FRAME_FMT_IS_FBC(p->fmt)) {
        if ((p->fmt & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
            p->header_size = MPP_ALIGN(MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16, SZ_4K);
        else
            p->header_size = MPP_ALIGN(p->width, 16) * MPP_ALIGN(p->height, 16) / 16;
    } else {
        p->header_size = 0;
    }

    return ret;
}

MPP_RET test_ctx_deinit(MpiEncTestData *p)
{
    if (p) {
        if (p->cam_ctx) {
            camera_source_deinit(p->cam_ctx);
            p->cam_ctx = NULL;
        }
        if (p->fp_input) {
            fclose(p->fp_input);
            p->fp_input = NULL;
        }
        if (p->fp_output) {
            fclose(p->fp_output);
            p->fp_output = NULL;
        }
        if (p->fp_verify) {
            fclose(p->fp_verify);
            p->fp_verify = NULL;
        }
        if (p->fp_slt_gold) {
            fclose(p->fp_slt_gold);
            p->fp_slt_gold = NULL;
        }
        if (p->fp_slt_result) {
            fclose(p->fp_slt_result);
            p->fp_slt_result = NULL;
        }
    }
    return MPP_OK;
}

MPP_RET test_mpp_enc_cfg_setup(MpiEncMultiCtxInfo *info)
{
    MpiEncTestArgs *cmd = info->cmd;
    MpiEncTestData *p = &info->ctx;
    MppApi *mpi = p->mpi;
    MppCtx ctx = p->ctx;
    MppEncCfg cfg = p->cfg;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret;
    RK_U32 rotation;
    RK_U32 mirroring;
    RK_U32 flip;
    RK_U32 gop_mode = p->gop_mode;
    MppEncRefCfg ref = NULL;

    /* setup default parameter */
    if (p->fps_in_den == 0)
        p->fps_in_den = 1;
    if (p->fps_in_num == 0)
        p->fps_in_num = 30;
    if (p->fps_out_den == 0)
        p->fps_out_den = 1;
    if (p->fps_out_num == 0)
        p->fps_out_num = 30;

    if (!p->bps)
        p->bps = p->width * p->height / 8 * (p->fps_out_num / p->fps_out_den);

    /* setup preprocess parameters */
    mpp_enc_cfg_set_s32(cfg, "prep:width", p->width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", p->height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", p->hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", p->ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", p->fmt);
    mpp_enc_cfg_set_s32(cfg, "prep:range", MPP_FRAME_RANGE_JPEG);

    mpp_env_get_u32("mirroring", &mirroring, 0);
    mpp_env_get_u32("rotation", &rotation, 0);
    mpp_env_get_u32("flip", &flip, 0);

    mpp_enc_cfg_set_s32(cfg, "prep:mirroring", mirroring);
    mpp_enc_cfg_set_s32(cfg, "prep:rotation", rotation);
    mpp_enc_cfg_set_s32(cfg, "prep:flip", flip);

    /* setup rate control parameters */
    mpp_enc_cfg_set_s32(cfg, "rc:mode", p->rc_mode);
    mpp_enc_cfg_set_u32(cfg, "rc:max_reenc_times", 0);
    mpp_enc_cfg_set_u32(cfg, "rc:super_mode", 0);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", p->fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", p->fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denom", p->fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", p->fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", p->fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denom", p->fps_out_den);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", p->bps);
    switch (p->rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR :
    case MPP_ENC_RC_MODE_AVBR : {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 1 / 16);
    } break;
    default : {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", p->bps_max ? p->bps_max : p->bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", p->bps_min ? p->bps_min : p->bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    switch (p->type) {
    case MPP_VIDEO_CodingAVC :
    case MPP_VIDEO_CodingHEVC : {
        switch (p->rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            RK_S32 fix_qp = cmd->qp_init;

            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", fix_qp);
        } break;
        case MPP_ENC_RC_MODE_CBR :
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR :
        case MPP_ENC_RC_MODE_SMTRC : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", cmd->qp_init ? cmd->qp_init : -1);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", cmd->qp_max ? cmd->qp_max : 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", cmd->qp_min ? cmd->qp_min : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", cmd->qp_max_i ? cmd->qp_max_i : 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", cmd->qp_min_i ? cmd->qp_min_i : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", cmd->fqp_min_i ? cmd->fqp_min_i : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", cmd->fqp_max_i ? cmd->fqp_max_i : 45);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", cmd->fqp_min_p ? cmd->fqp_min_p : 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", cmd->fqp_max_p ? cmd->fqp_max_p : 45);
        } break;
        default : {
            mpp_err_f("unsupport encoder rc mode %d\n", p->rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8 : {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", cmd->qp_init ? cmd->qp_init : 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  cmd->qp_max ? cmd->qp_max : 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  cmd->qp_min ? cmd->qp_min : 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", cmd->qp_max_i ? cmd->qp_max_i : 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", cmd->qp_min_i ? cmd->qp_min_i : 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", cmd->qp_init ? cmd->qp_init : 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", cmd->qp_max ? cmd->qp_max : 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", cmd->qp_min ? cmd->qp_min : 1);
    } break;
    default : {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", p->type);
    switch (p->type) {
    case MPP_VIDEO_CodingAVC : {
        RK_U32 constraint_set;

        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        mpp_env_get_u32("constraint_set", &constraint_set, 0);
        if (constraint_set & 0x3f0000)
            mpp_enc_cfg_set_s32(cfg, "h264:constraint_set", constraint_set);
    } break;
    case MPP_VIDEO_CodingHEVC : {
        mpp_enc_cfg_set_s32(cfg, "h265:diff_cu_qp_delta_depth", p->cu_qp_delta_depth);
    } break;
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 : {
    } break;
    default : {
        mpp_err_f("unsupport encoder coding type %d\n", p->type);
    } break;
    }

    p->split_mode = 0;
    p->split_arg = 0;
    p->split_out = 0;

    mpp_env_get_u32("split_mode", &p->split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &p->split_arg, 0);
    mpp_env_get_u32("split_out", &p->split_out, 0);

    if (p->split_mode) {
        mpp_log_q(quiet, "%p split mode %d arg %d out %d\n", ctx,
                  p->split_mode, p->split_arg, p->split_out);
        mpp_enc_cfg_set_s32(cfg, "split:mode", p->split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", p->split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:out", p->split_out);
    }

    // config gop_len and ref cfg
    mpp_enc_cfg_set_s32(cfg, "rc:gop", p->gop_len ? p->gop_len : p->fps_out_num * 2);

    mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);
    if (gop_mode) {
        mpp_enc_ref_cfg_init(&ref);

        if (p->gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, p->gop_len, p->vi_len);

        mpp_enc_cfg_set_ptr(cfg, "rc:ref_cfg", ref);
    }

    /* setup fine tuning paramters */
    mpp_enc_cfg_set_s32(cfg, "tune:anti_flicker_str", p->anti_flicker_str);
    mpp_enc_cfg_set_s32(cfg, "tune:atf_str", cmd->atf_str);
    mpp_enc_cfg_set_s32(cfg, "tune:atr_str_i", p->atr_str_i);
    mpp_enc_cfg_set_s32(cfg, "tune:atr_str_p", p->atr_str_p);
    mpp_enc_cfg_set_s32(cfg, "tune:atl_str", p->atl_str);
    mpp_enc_cfg_set_s32(cfg, "tune:deblur_en", cmd->deblur_en);
    mpp_enc_cfg_set_s32(cfg, "tune:deblur_str", cmd->deblur_str);
    mpp_enc_cfg_set_s32(cfg, "tune:sao_str_i", p->sao_str_i);
    mpp_enc_cfg_set_s32(cfg, "tune:sao_str_p", p->sao_str_p);
    mpp_enc_cfg_set_s32(cfg, "tune:lambda_idx_p", cmd->lambda_idx_p);
    mpp_enc_cfg_set_s32(cfg, "tune:lambda_idx_i", cmd->lambda_idx_i);
    mpp_enc_cfg_set_s32(cfg, "tune:rc_container", cmd->rc_container);
    mpp_enc_cfg_set_s32(cfg, "tune:scene_mode", p->scene_mode);
    mpp_enc_cfg_set_s32(cfg, "tune:speed", cmd->speed);
    mpp_enc_cfg_set_s32(cfg, "tune:vmaf_opt", 0);

    /* setup hardware specified parameters */
    if (cmd->rc_mode == MPP_ENC_RC_MODE_SMTRC) {
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_i", aq_thd_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_p", aq_thd_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_step_smart);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_p", aq_step_smart);
    } else {
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_i", aq_thd);
        mpp_enc_cfg_set_st(cfg, "hw:aq_thrd_p", aq_thd);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_step_i_ipc);
        mpp_enc_cfg_set_st(cfg, "hw:aq_step_p", aq_step_p_ipc);
    }
    mpp_enc_cfg_set_st(cfg, "hw:aq_rnge_arr", aq_rnge_arr);

    mpp_enc_cfg_set_s32(cfg, "hw:qbias_en", 1);
    mpp_enc_cfg_set_s32(cfg, "hw:qbias_i", cmd->bias_i);
    mpp_enc_cfg_set_s32(cfg, "hw:qbias_p", cmd->bias_p);

    if (p->type == MPP_VIDEO_CodingAVC) {
        mpp_enc_cfg_set_st(cfg, "hw:qbias_arr", qbias_arr_avc);
    } else if (p->type == MPP_VIDEO_CodingHEVC) {
        mpp_enc_cfg_set_st(cfg, "hw:qbias_arr", qbias_arr_hevc);
    }

    mpp_enc_cfg_set_s32(cfg, "hw:skip_bias_en", 0);
    mpp_enc_cfg_set_s32(cfg, "hw:skip_bias", 4);
    mpp_enc_cfg_set_s32(cfg, "hw:skip_sad", 8);

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        mpp_err("mpi control enc set cfg failed ret %d\n", ret);
        goto RET;
    }

    if (cmd->type == MPP_VIDEO_CodingAVC || cmd->type == MPP_VIDEO_CodingHEVC) {
        RcApiBrief rc_api_brief;
        rc_api_brief.type = cmd->type;
        rc_api_brief.name = (cmd->rc_mode == MPP_ENC_RC_MODE_SMTRC) ?
                            "smart" : "default";

        ret = mpi->control(ctx, MPP_ENC_SET_RC_API_CURRENT, &rc_api_brief);
        if (ret) {
            mpp_err("mpi control enc set rc api failed ret %d\n", ret);
            goto RET;
        }
    }

    if (ref)
        mpp_enc_ref_cfg_deinit(&ref);

    /* optional */
    {
        RK_U32 sei_mode;

        mpp_env_get_u32("sei_mode", &sei_mode, MPP_ENC_SEI_MODE_DISABLE);
        p->sei_mode = sei_mode;
        ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &p->sei_mode);
        if (ret) {
            mpp_err("mpi control enc set sei cfg failed ret %d\n", ret);
            goto RET;
        }
    }

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        p->header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &p->header_mode);
        if (ret) {
            mpp_err("mpi control enc set header mode failed ret %d\n", ret);
            goto RET;
        }
    }

    /* setup test mode by env */
    mpp_env_get_u32("osd_enable", &p->osd_enable, 0);
    mpp_env_get_u32("osd_mode", &p->osd_mode, MPP_ENC_OSD_PLT_TYPE_DEFAULT);
    mpp_env_get_u32("roi_enable", &p->roi_enable, 0);
    mpp_env_get_u32("user_data_enable", &p->user_data_enable, 0);

    if (p->roi_enable) {
        mpp_enc_roi_init(&p->roi_ctx, p->width, p->height, p->type, 4);
        mpp_assert(p->roi_ctx);
    }

RET:
    return ret;
}

MPP_RET test_mpp_run(MpiEncMultiCtxInfo *info)
{
    MpiEncTestArgs *cmd = info->cmd;
    MpiEncTestData *p = &info->ctx;
    MppApi *mpi = p->mpi;
    MppCtx ctx = p->ctx;
    RK_U32 quiet = cmd->quiet;
    RK_S32 chn = info->chn;
    RK_U32 cap_num = 0;
    DataCrc checkcrc;
    MPP_RET ret = MPP_OK;
    RK_FLOAT psnr_const = 0;
    RK_U32 sse_unit_in_pixel = 0;

    memset(&checkcrc, 0, sizeof(checkcrc));
    checkcrc.sum = mpp_malloc(RK_ULONG, 512);

    if (p->type == MPP_VIDEO_CodingAVC || p->type == MPP_VIDEO_CodingHEVC) {
        MppPacket packet = NULL;

        /*
         * Can use packet with normal malloc buffer as input not pkt_buf.
         * Please refer to vpu_api_legacy.cpp for normal buffer case.
         * Using pkt_buf buffer here is just for simplifing demo.
         */
        mpp_packet_init_with_buffer(&packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);

        ret = mpi->control(ctx, MPP_ENC_GET_HDR_SYNC, packet);
        if (ret) {
            mpp_err("mpi control enc get extra info failed\n");
            goto RET;
        } else {
            /* get and write sps/pps for H.264 */

            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);

            if (p->fp_output)
                fwrite(ptr, 1, len, p->fp_output);
        }

        mpp_packet_deinit(&packet);

        sse_unit_in_pixel = p->type == MPP_VIDEO_CodingAVC ? 16 : 8;
        psnr_const = (16 + log2(MPP_ALIGN(p->width, sse_unit_in_pixel) *
                                MPP_ALIGN(p->height, sse_unit_in_pixel)));
    }
    while (!p->pkt_eos) {
        MppMeta meta = NULL;
        MppFrame frame = NULL;
        MppPacket packet = NULL;
        void *buf = mpp_buffer_get_ptr(p->frm_buf);
        RK_S32 cam_frm_idx = -1;
        MppBuffer cam_buf = NULL;
        RK_U32 eoi = 1;

        if (p->fp_input) {
            mpp_buffer_sync_begin(p->frm_buf);
            ret = read_image(buf, p->fp_input, p->width, p->height,
                             p->hor_stride, p->ver_stride, p->fmt);
            if (ret == MPP_NOK || feof(p->fp_input)) {
                p->frm_eos = 1;

                if (p->frame_num < 0) {
                    clearerr(p->fp_input);
                    rewind(p->fp_input);
                    p->frm_eos = 0;
                    mpp_log_q(quiet, "chn %d loop times %d\n", chn, ++p->loop_times);
                    continue;
                }
                mpp_log_q(quiet, "chn %d found last frame. feof %d\n", chn, feof(p->fp_input));
            } else if (ret == MPP_ERR_VALUE)
                goto RET;
            mpp_buffer_sync_end(p->frm_buf);
        } else {
            if (p->cam_ctx == NULL) {
                mpp_buffer_sync_begin(p->frm_buf);
                ret = fill_image(buf, p->width, p->height, p->hor_stride,
                                 p->ver_stride, p->fmt, p->frame_count * p->frm_step);
                if (ret)
                    goto RET;
                mpp_buffer_sync_end(p->frm_buf);
            } else {
                cam_frm_idx = camera_source_get_frame(p->cam_ctx);
                mpp_assert(cam_frm_idx >= 0);

                /* skip unstable frames */
                if (cap_num++ < 50) {
                    camera_source_put_frame(p->cam_ctx, cam_frm_idx);
                    continue;
                }

                cam_buf = camera_frame_to_buf(p->cam_ctx, cam_frm_idx);
                mpp_assert(cam_buf);
            }
        }

        ret = mpp_frame_init(&frame);
        if (ret) {
            mpp_err_f("mpp_frame_init failed\n");
            goto RET;
        }

        mpp_frame_set_width(frame, p->width);
        mpp_frame_set_height(frame, p->height);
        mpp_frame_set_hor_stride(frame, p->hor_stride);
        mpp_frame_set_ver_stride(frame, p->ver_stride);
        mpp_frame_set_fmt(frame, p->fmt);
        mpp_frame_set_eos(frame, p->frm_eos);

        if (p->fp_input && feof(p->fp_input))
            mpp_frame_set_buffer(frame, NULL);
        else if (cam_buf)
            mpp_frame_set_buffer(frame, cam_buf);
        else
            mpp_frame_set_buffer(frame, p->frm_buf);

        meta = mpp_frame_get_meta(frame);
        mpp_packet_init_with_buffer(&packet, p->pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);
        mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
        mpp_meta_set_buffer(meta, KEY_MOTION_INFO, p->md_info);

        if (p->osd_enable || p->user_data_enable || p->roi_enable) {
            if (p->user_data_enable) {
                MppEncUserData user_data;
                char *str = "this is user data\n";

                if ((p->frame_count & 10) == 0) {
                    user_data.pdata = str;
                    user_data.len = strlen(str) + 1;
                    mpp_meta_set_ptr(meta, KEY_USER_DATA, &user_data);
                }
                static RK_U8 uuid_debug_info[16] = {
                    0x57, 0x68, 0x97, 0x80, 0xe7, 0x0c, 0x4b, 0x65,
                    0xa9, 0x06, 0xae, 0x29, 0x94, 0x11, 0xcd, 0x9a
                };

                MppEncUserDataSet data_group;
                MppEncUserDataFull datas[2];
                char *str1 = "this is user data 1\n";
                char *str2 = "this is user data 2\n";
                data_group.count = 2;
                datas[0].len = strlen(str1) + 1;
                datas[0].pdata = str1;
                datas[0].uuid = uuid_debug_info;

                datas[1].len = strlen(str2) + 1;
                datas[1].pdata = str2;
                datas[1].uuid = uuid_debug_info;

                data_group.datas = datas;

                mpp_meta_set_ptr(meta, KEY_USER_DATAS, &data_group);
            }

            if (p->osd_enable) {
                /* gen and cfg osd plt */
                mpi_enc_gen_osd_plt(&p->osd_plt, p->frame_count);

                p->osd_plt_cfg.change = MPP_ENC_OSD_PLT_CFG_CHANGE_ALL;
                p->osd_plt_cfg.type = MPP_ENC_OSD_PLT_TYPE_USERDEF;
                p->osd_plt_cfg.plt = &p->osd_plt;

                ret = mpi->control(ctx, MPP_ENC_SET_OSD_PLT_CFG, &p->osd_plt_cfg);
                if (ret) {
                    mpp_err("mpi control enc set osd plt failed ret %d\n", ret);
                    goto RET;
                }

                /* gen and cfg osd plt */
                mpi_enc_gen_osd_data(&p->osd_data, p->buf_grp, p->width,
                                     p->height, p->frame_count);
                mpp_meta_set_ptr(meta, KEY_OSD_DATA, (void*)&p->osd_data);
            }

            if (p->roi_enable) {
                RoiRegionCfg *region = &p->roi_region;

                /* calculated in pixels */
                region->x = MPP_ALIGN(p->width / 8, 16);
                region->y = MPP_ALIGN(p->height / 8, 16);
                region->w = 128;
                region->h = 256;
                region->force_intra = 0;
                region->qp_mode = 1;
                region->qp_val = 24;

                mpp_enc_roi_add_region(p->roi_ctx, region);

                region->x = MPP_ALIGN(p->width / 2, 16);
                region->y = MPP_ALIGN(p->height / 4, 16);
                region->w = 256;
                region->h = 128;
                region->force_intra = 1;
                region->qp_mode = 1;
                region->qp_val = 10;

                mpp_enc_roi_add_region(p->roi_ctx, region);

                /* send roi info by metadata */
                mpp_enc_roi_setup_meta(p->roi_ctx, meta);
            }
        }

        if (!p->first_frm)
            p->first_frm = mpp_time();
        /*
         * NOTE: in non-block mode the frame can be resent.
         * The default input timeout mode is block.
         *
         * User should release the input frame to meet the requirements of
         * resource creator must be the resource destroyer.
         */
        ret = mpi->encode_put_frame(ctx, frame);
        if (ret) {
            mpp_err("chn %d encode put frame failed\n", chn);
            mpp_frame_deinit(&frame);
            goto RET;
        }

        mpp_frame_deinit(&frame);

        do {
            ret = mpi->encode_get_packet(ctx, &packet);
            if (ret) {
                mpp_err("chn %d encode get packet failed\n", chn);
                goto RET;
            }

            mpp_assert(packet);

            if (packet) {
                // write packet to file here
                void *ptr   = mpp_packet_get_pos(packet);
                size_t len  = mpp_packet_get_length(packet);
                char log_buf[256];
                RK_S32 log_size = sizeof(log_buf) - 1;
                RK_S32 log_len = 0;

                if (!p->first_pkt)
                    p->first_pkt = mpp_time();

                p->pkt_eos = mpp_packet_get_eos(packet);

                if (p->fp_output)
                    fwrite(ptr, 1, len, p->fp_output);

                if (p->fp_verify && !p->pkt_eos) {
                    calc_data_crc((RK_U8 *)ptr, (RK_U32)len, &checkcrc);
                    // mpp_log_q("p->frame_count=%d, len=%d\n", p->frame_count, len);
                    write_data_crc(p->fp_verify, &checkcrc);
                }

                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    "encoded frame %-4d", p->frame_count);

                /* for low delay partition encoding */
                if (mpp_packet_is_partition(packet)) {
                    eoi = mpp_packet_is_eoi(packet);

                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        " pkt %d", p->frm_pkt_cnt);
                    p->frm_pkt_cnt = (eoi) ? (0) : (p->frm_pkt_cnt + 1);
                }

                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    " size %-7zu", len);

                if (mpp_packet_has_meta(packet)) {
                    meta = mpp_packet_get_meta(packet);
                    RK_S32 temporal_id = 0;
                    RK_S32 lt_idx = -1;
                    RK_S32 avg_qp = -1, bps_rt = -1;
                    RK_S32 use_lt_idx = -1;
                    RK_S64 sse = 0;
                    RK_FLOAT psnr = 0;
                    RK_S32 hw_stat;
                    RK_S32 hw_time;

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &temporal_id))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " tid %d", temporal_id);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &lt_idx))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " lt %d", lt_idx);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &avg_qp))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " qp %2d", avg_qp);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_BPS_RT, &bps_rt))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " bps_rt %d", bps_rt);

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_USE_LTR, &use_lt_idx))
                        log_len += snprintf(log_buf + log_len, log_size - log_len, " vi");

                    if (MPP_OK == mpp_meta_get_s64(meta, KEY_ENC_SSE, &sse)) {
                        psnr = 3.01029996 * (psnr_const - log2(sse));
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " psnr %.4f", psnr);
                    }
                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_HW_STATUS, &hw_stat))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " hw_stat 0x%08x", hw_stat);
                    p->hw_status |= hw_stat;

                    if (MPP_OK == mpp_meta_get_s32(meta, KEY_ENC_HW_TIME, &hw_time))
                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                            " hw_time %d", hw_time);
                    p->total_hw_time += (hw_time / 1000);
                }

                // mpp_log_q(quiet, "chn %d %s\n", chn, log_buf);

                mpp_packet_deinit(&packet);
                fps_calc_inc(cmd->fps);

                p->stream_size += len;
                p->frame_count += eoi;

                if (p->pkt_eos) {
                    mpp_log_q(quiet, "chn %d found last packet\n", chn);
                    mpp_assert(p->frm_eos);
                }
            }
        } while (!eoi);

        if (cam_frm_idx >= 0)
            camera_source_put_frame(p->cam_ctx, cam_frm_idx);

        if (p->frame_num > 0 && p->frame_count >= p->frame_num)
            break;

        if (p->loop_end)
            break;

        if (p->frm_eos && p->pkt_eos)
            break;
    }
RET:
    MPP_FREE(checkcrc.sum);

    return ret;
}

static void verify_check(MpiEncTestArgs *cmd, MpiEncTestData *p, MpiEncMultiCtxRet *enc_ret)
{
    pthread_mutex_lock(&g_file_mutex);

    if (p->fp_verify && p->fp_slt_gold) {
        fclose(p->fp_verify);
        fclose(p->fp_slt_gold);
        p->fp_verify = NULL;
        p->fp_slt_gold = NULL;

        const char *verify_file = cmd->file_slt;
        const char *gold_file = cmd->file_slt_gold;

        FILE *fp1 = fopen(verify_file, "rb");
        FILE *fp2 = fopen(gold_file, "rb");

        if (fp1 && fp2) {
            int files_match = 1;
            char buf1[4096], buf2[4096];
            size_t read1, read2;

            while (files_match && !feof(fp1) && !feof(fp2)) {
                read1 = fread(buf1, 1, sizeof(buf1), fp1);
                read2 = fread(buf2, 1, sizeof(buf2), fp2);

                if (read1 != read2 || memcmp(buf1, buf2, read1) != 0) {
                    files_match = 0;
                    break;
                }
            }

            if (files_match && (!feof(fp1) || !feof(fp2))) {
                files_match = 0;
            }

            enc_ret->verify = files_match ? 1 : 0;

            fclose(fp1);
            fclose(fp2);
        } else {
            mpp_err("Failed to open files for comparison\n");
            if (verify_file) mpp_err("==Verify== verify_file: %s\n", verify_file);
            if (gold_file) mpp_err("==Verify== gold_file: %s\n", gold_file);
            enc_ret->verify = 0;

            if (fp1) fclose(fp1);
            if (fp2) fclose(fp2);
        }
    } else if (p->fp_verify) {
        fclose(p->fp_verify);
        p->fp_verify = NULL;

        const char *verify_file = cmd->file_slt;
        const char *gold_file = cmd->file_slt_gold;

        if (verify_file && gold_file) {
            FILE *src = fopen(verify_file, "rb");
            FILE *dst = fopen(gold_file, "wb");

            if (src && dst) {
                char buffer[4096];
                size_t bytes_read;

                while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                    fwrite(buffer, 1, bytes_read, dst);
                }

                fclose(src);
                fclose(dst);

                mpp_log("[Verify] Created gold file from verify file.\n");
                enc_ret->verify = 0;
            } else {
                mpp_err("Failed to copy verify file to gold file\n");
                enc_ret->verify = 0;

                if (src) fclose(src);
                if (dst) fclose(dst);
            }
        } else {
            mpp_err("Missing file paths for verify or gold file\n");
            enc_ret->verify = 0;
        }
    }

    pthread_mutex_unlock(&g_file_mutex);
}

void *enc_test(void *arg)
{
    MpiEncMultiCtxInfo *info = (MpiEncMultiCtxInfo *)arg;
    MpiEncTestArgs *cmd = info->cmd;
    MpiEncTestData *p = &info->ctx;
    MpiEncMultiCtxRet *enc_ret = &info->ret;
    MppPollType timeout = MPP_POLL_BLOCK;
    RK_U32 quiet = cmd->quiet;
    MPP_RET ret = MPP_OK;
    RK_S64 t_s = 0;
    RK_S64 t_e = 0;

    mpp_log_q(quiet, "%s start\n", info->name);

    ret = test_ctx_init(info);
    if (ret) {
        mpp_err_f("test data init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_group_get_internal(&p->buf_grp, MPP_BUFFER_TYPE_DRM | MPP_BUFFER_FLAGS_CACHABLE);
    if (ret) {
        mpp_err_f("failed to get mpp buffer group ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->frm_buf, p->frame_size + p->header_size);
    if (ret) {
        mpp_err_f("failed to get buffer for input frame ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->pkt_buf, p->frame_size);
    if (ret) {
        mpp_err_f("failed to get buffer for output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_buffer_get(p->buf_grp, &p->md_info, p->mdinfo_size);
    if (ret) {
        mpp_err_f("failed to get buffer for motion info output packet ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    // encoder demo
    ret = mpp_create(&p->ctx, &p->mpi);
    if (ret) {
        mpp_err("mpp_create failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    mpp_log_q(quiet, "%p encoder test start w %d h %d type %d\n",
              p->ctx, p->width, p->height, p->type);

    ret = p->mpi->control(p->ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
    if (MPP_OK != ret) {
        mpp_err("mpi control set output timeout %d ret %d\n", timeout, ret);
        goto MPP_TEST_OUT;
    }

    if (cmd->kmpp_en)
        kmpp_cfg_init(info);

    ret = mpp_init(p->ctx, MPP_CTX_ENC, p->type);
    if (ret) {
        mpp_err("mpp_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = mpp_enc_cfg_init(&p->cfg);
    if (ret) {
        mpp_err_f("mpp_enc_cfg_init failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->control(p->ctx, MPP_ENC_GET_CFG, p->cfg);
    if (ret) {
        mpp_err_f("get enc cfg failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = test_mpp_enc_cfg_setup(info);
    if (ret) {
        mpp_err_f("test mpp setup failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    t_s = mpp_time();
    ret = test_mpp_run(info);
    t_e = mpp_time();
    if (ret) {
        mpp_err_f("test mpp run failed ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    ret = p->mpi->reset(p->ctx);
    if (ret) {
        mpp_err("mpi->reset failed\n");
        goto MPP_TEST_OUT;
    }

    enc_ret->elapsed_time = t_e - t_s;
    enc_ret->frame_count = p->frame_count;
    enc_ret->frame_rate = (float)p->frame_count * 1000000 / enc_ret->elapsed_time;
    enc_ret->stream_size = p->stream_size;
    enc_ret->hw_status = p->hw_status;
    enc_ret->total_hw_time = p->total_hw_time;
    enc_ret->hw_fps = (p->total_hw_time > 0) ?
                      (float)(1000 * p->frame_count / p->total_hw_time) : 0;

    verify_check(cmd, p, enc_ret);

MPP_TEST_OUT:
    if (p->ctx) {
        mpp_destroy(p->ctx);
        p->ctx = NULL;
    }

    if (p->cfg) {
        mpp_enc_cfg_deinit(p->cfg);
        p->cfg = NULL;
    }

    if (p->frm_buf) {
        mpp_buffer_put(p->frm_buf);
        p->frm_buf = NULL;
    }

    if (p->pkt_buf) {
        mpp_buffer_put(p->pkt_buf);
        p->pkt_buf = NULL;
    }

    if (p->md_info) {
        mpp_buffer_put(p->md_info);
        p->md_info = NULL;
    }

    if (p->osd_data.buf) {
        mpp_buffer_put(p->osd_data.buf);
        p->osd_data.buf = NULL;
    }

    if (p->buf_grp) {
        mpp_buffer_group_put(p->buf_grp);
        p->buf_grp = NULL;
    }

    if (p->roi_ctx) {
        mpp_enc_roi_deinit(p->roi_ctx);
        p->roi_ctx = NULL;
    }
    if (p->init_kcfg)
        mpp_venc_kcfg_deinit(p->init_kcfg);

    test_ctx_deinit(p);

    return NULL;
}

MPP_RET enc_test_multi(MpiEncTestArgs* cmd, const char *name, AutoTestResult *result)
{
    MpiEncMultiCtxInfo *ctxs = NULL;
    MpiEncTestArgs **thread_cmds = NULL;
    RK_S32 ret = MPP_NOK;
    RK_S32 i = 0;
    RK_U32 piexl_per_ms = 0;

    ctxs = mpp_calloc(MpiEncMultiCtxInfo, cmd->nthreads);
    thread_cmds = mpp_calloc(MpiEncTestArgs*, cmd->nthreads);
    if (NULL == ctxs || NULL == thread_cmds) {
        mpp_err("failed to alloc context for instances\n");
        ret = -1;
        goto CLEANUP;
    }

    for (i = 0; i < cmd->nthreads; i++) {
        thread_cmds[i] = mpp_calloc(MpiEncTestArgs, 1);
        if (!thread_cmds[i]) {
            mpp_err("failed to allocate cmd for thread %d\n", i);
            ret = MPP_ERR_MALLOC;
            goto CLEANUP;
        }

        memcpy(thread_cmds[i], cmd, sizeof(MpiEncTestArgs));

        if (cmd->file_output) {
            size_t output_len = strlen(cmd->file_output) + 32;
            char *thread_output = mpp_calloc(char, output_len);
            if (!thread_output) {
                mpp_err("failed to allocate memory for thread %d output filename\n", i);
                MPP_FREE(thread_cmds[i]);
                thread_cmds[i] = NULL;
                ret = MPP_ERR_MALLOC;
                goto CLEANUP;
            }

            if (cmd->nthreads > 1) {
                char *dot = strrchr(cmd->file_output, '.');
                if (dot) {
                    size_t base_len = dot - cmd->file_output;
                    snprintf(thread_output, output_len,
                             "%.*s_t%d%s", (int)base_len, cmd->file_output, i, dot);
                } else {
                    snprintf(thread_output, output_len,
                             "%s_t%d", cmd->file_output, i);
                }
            } else {
                strcpy(thread_output, cmd->file_output);
            }

            thread_cmds[i]->file_output = thread_output;

        } else {
            thread_cmds[i]->file_output = NULL;
        }

        if (cmd->file_input) {
            thread_cmds[i]->file_input = mpp_calloc(char, strlen(cmd->file_input) + 1);
            if (thread_cmds[i]->file_input) {
                strcpy(thread_cmds[i]->file_input, cmd->file_input);
            }
        }

        if (cmd->file_slt) {
            thread_cmds[i]->file_slt = mpp_calloc(char, strlen(cmd->file_slt) + 1);
            if (thread_cmds[i]->file_slt) {
                strcpy(thread_cmds[i]->file_slt, cmd->file_slt);
            }
        }

        if (cmd->file_slt_gold) {
            thread_cmds[i]->file_slt_gold = mpp_calloc(char, strlen(cmd->file_slt_gold) + 1);
            if (thread_cmds[i]->file_slt_gold) {
                strcpy(thread_cmds[i]->file_slt_gold, cmd->file_slt_gold);
            }
        }

        ctxs[i].cmd = thread_cmds[i];
        ctxs[i].name = name;
        ctxs[i].chn = i;

        ret = pthread_create(&ctxs[i].thd, NULL, enc_test, &ctxs[i]);
        if (ret) {
            mpp_err("failed to create thread %d\n", i);
            goto CLEANUP;
        }
    }

    if (cmd->frame_num < 0) {
        // wait for input then quit encoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop encoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        for (i = 0; i < cmd->nthreads; i++)
            ctxs[i].ctx.loop_end = 1;
    }

    for (i = 0; i < cmd->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

    for (i = 0; i < cmd->nthreads; i++) {
        MpiEncMultiCtxRet *enc_ret = &ctxs[i].ret;

        result->hw_status |= enc_ret->hw_status;
        result->verify |= enc_ret->verify;
        result->hw_time = MPP_MAX(result->hw_time, enc_ret->total_hw_time);
        result->hw_fps = MPP_MAX(result->hw_fps, enc_ret->hw_fps);
        result->stream_size = MPP_MAX(result->stream_size, enc_ret->stream_size);
        result->sft_fps = MPP_MAX(result->sft_fps, enc_ret->frame_rate);
        result->sft_time = MPP_MAX(result->sft_time, enc_ret->elapsed_time / 1000);

        piexl_per_ms = (cmd->width * cmd->height) / result->hw_time * cmd->frame_num;

        if (result->verify) {
            if (cmd->type == MPP_VIDEO_CodingAVC || cmd->type == MPP_VIDEO_CodingHEVC) {
                if (piexl_per_ms > 497664) // 8K/30 fps h264e/h265e doule core, requires 500000 pixels/ms/core.
                    result->pass = 1;
                else
                    result->pass = 0;
            } else {
                if (piexl_per_ms > 12500) // 8k/15 fps jepge 4 core, requires 12500 pixels/ms/core.
                    result->pass = 1;
                else
                    result->pass = 0;
            }
        } else {
            result->pass = 0;
        }

        if (result->pass)
            ret = MPP_OK;
    }

CLEANUP:
    if (thread_cmds) {
        for (i = 0; i < cmd->nthreads; i++) {
            if (thread_cmds[i]) {
                if (thread_cmds[i]->file_output && thread_cmds[i]->file_output != cmd->file_output) {
                    MPP_FREE(thread_cmds[i]->file_output);
                }
                if (thread_cmds[i]->file_input && thread_cmds[i]->file_input != cmd->file_input) {
                    MPP_FREE(thread_cmds[i]->file_input);
                }
                if (thread_cmds[i]->file_slt && thread_cmds[i]->file_slt != cmd->file_slt) {
                    MPP_FREE(thread_cmds[i]->file_slt);
                }
                if ( thread_cmds[i]->file_slt_gold && thread_cmds[i]->file_slt_gold != cmd->file_slt_gold) {
                    MPP_FREE(thread_cmds[i]->file_slt_gold);
                }
                MPP_FREE(thread_cmds[i]);
            }
        }
        MPP_FREE(thread_cmds);
    }

    if (ctxs) {
        MPP_FREE(ctxs);
    }

    return ret;
}

MPP_RET mpi_enc_slt_ctx_update_by_args(MpiEncSltCtx* ctx, int argc, char **argv)
{
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;

    if ((argc < 2) || NULL == ctx || NULL == argv)
        goto done;

    if (!strcmp(argv[1] + 2, "all")) {
        ctx->slt_type = MPP_ENC_SLT_TEST_ALL;
        ctx->slt_cfg_type = MPP_SLT_CONGIG_DEFAULT;

    } else if (!strcmp(argv[1] + 2, "base")) {
        ctx->slt_type = MPP_ENC_SLT_BASE_ALL;
        ctx->slt_cfg_type = MPP_SLT_CONGIG_DEFAULT;

    } else if (!strcmp(argv[1] + 2, "module")) {
        ctx->slt_type = MPP_ENC_SLT_MODULE_ALL;
        ctx->slt_cfg_type = MPP_SLT_CONGIG_DEFAULT;

    } else if (!strcmp(argv[1] + 2, "stress")) {
        ctx->slt_type = MPP_ENC_SLT_STRESS_ALL;
        ctx->slt_cfg_type = MPP_SLT_CONGIG_DEFAULT;

    } else if (!strcmp(argv[1], "--config")) {
        ctx->slt_cfg_type = MPP_SLT_CONGIG_OUT;
        for (i = 2; i < argc; i++) {
            if (!strcmp(argv[i], "--file") && i + 1 < argc) {
                if (ctx->config_file) free(ctx->config_file);
                ctx->config_file = strdup(argv[i + 1]);
                i++;
            } else if (!strcmp(argv[i], "--suite") && i + 1 < argc) {
                if (ctx->test_suite) free(ctx->test_suite);
                ctx->test_suite = strdup(argv[i + 1]);
                i++;
            } else if (argv[i][0] != '-') {
                if (ctx->config_file) free(ctx->config_file);
                ctx->config_file = strdup(argv[i]);
            }
        }
        mpp_log("Config mode: file=%s, suite=%s\n", ctx->config_file, ctx->test_suite);

    } else {
        mpp_err("Unknown option: %s\n", argv[1]);
        ret = MPP_NOK;
        goto done;
    }

done:
    return ret;
}

MPP_RET mpi_enc_slt_dir_init(const char *base_path)
{
    if (!base_path) {
        mpp_err("Invalid base path for SLT directories\n");
        return MPP_ERR_NULL_PTR;
    }

    char cmd[512];

    char check_cmd[512];
    snprintf(check_cmd, sizeof(check_cmd), "test -d %s", base_path);
    int dir_exists = (system(check_cmd) == 0);

    if (!dir_exists) {
        mpp_log("Creating SLT test directory structure...\n");

        snprintf(cmd, sizeof(cmd), "mkdir -p %s", base_path);
        int ret = system(cmd);
        if (ret != 0) {
            mpp_err("Failed to create base directory %s (ret=%d)\n", base_path, ret);
            return MPP_NOK;
        } else {
            mpp_log("Created base directory: %s\n", base_path);
        }

        const char *sub_dirs[] = {
            "result",
            "gold",
            "verify",
            "output",
            "config"
        };

        for (size_t i = 0; i < sizeof(sub_dirs) / sizeof(sub_dirs[0]); i++) {
            snprintf(cmd, sizeof(cmd), "mkdir -p %s/%s", base_path, sub_dirs[i]);
            ret = system(cmd);
            if (ret != 0) {
                mpp_err("Failed to create directory %s/%s (ret=%d)\n",
                        base_path, sub_dirs[i], ret);
                return MPP_NOK;
            } else {
                mpp_log("Created directory: %s/%s\n", base_path, sub_dirs[i]);
            }
        }

        mpp_log("SLT test directory structure created successfully\n");
    } else {
        mpp_log("SLT test directory exists, cleaning up old test files...\n");

        const char *dirs_to_clean[] = {
            "verify",
            "output",
            "config"
        };

        const char *all_dirs[] = {
            "result",
            "gold",
            "verify",
            "output",
            "config"
        };

        for (size_t i = 0; i < sizeof(all_dirs) / sizeof(all_dirs[0]); i++) {
            snprintf(cmd, sizeof(cmd), "mkdir -p %s/%s", base_path, all_dirs[i]);
            system(cmd);
        }

        for (size_t i = 0; i < sizeof(dirs_to_clean) / sizeof(dirs_to_clean[0]); i++) {
            snprintf(cmd, sizeof(cmd), "rm -rf %s/%s/*", base_path, dirs_to_clean[i]);
            int ret = system(cmd);
            if (ret == 0) {
                mpp_log("Cleaned directory: %s/%s\n", base_path, dirs_to_clean[i]);
            } else {
                mpp_log("Warning: Failed to clean directory %s/%s (ret=%d)\n",
                        base_path, dirs_to_clean[i], ret);
            }
        }

        mpp_log("Preserved gold directory: gold and result\n", base_path);
        mpp_log("SLT test directory cleanup completed\n");
    }

    return MPP_OK;
}

MPP_RET mpi_enc_slt_ctx_init(MpiEncSltCtx* ctx)
{
    if (!ctx)
        return MPP_ERR_NULL_PTR;

    ctx->slt_type = MPP_ENC_SLT_TEST_ALL;
    ctx->slt_cfg_type = MPP_SLT_CONGIG_NONE;
    ctx->slt_base_path = strdup("/sdcard/vepu_slt");
    ctx->config_file = strdup("/sdcard/vepu_slt/config/vepu_slt_test.cfg");
    ctx->test_suite = strdup("all");

    ctx->test_count = 0;
    ctx->test_cases = NULL;

    return MPP_OK;
}

void mpi_enc_slt_ctx_deinit(MpiEncSltCtx* ctx)
{
    if (!ctx)
        return;

    if (ctx->slt_base_path) {
        free(ctx->slt_base_path);
        ctx->slt_base_path = NULL;
    }

    if (ctx->config_file) {
        free(ctx->config_file);
        ctx->config_file = NULL;
    }

    if (ctx->test_suite) {
        free(ctx->test_suite);
        ctx->test_suite = NULL;
    }

    if (ctx->test_cases) {
        MPP_FREE(ctx->test_cases);
        ctx->test_cases = NULL;
        ctx->test_count = 0;
    }

    ctx->test_count = 0;
    ctx->base_test_count = 0;
    ctx->module_test_count = 0;
    ctx->stress_test_count = 0;

    ctx->slt_type = MPP_ENC_SLT_TEST_ALL;
    ctx->slt_cfg_type = MPP_SLT_CONGIG_NONE;
}

MpiEncSltCtx *mpi_enc_slt_get_ctx(void)
{
    static MpiEncSltCtx ctx;
    static RK_U32 init_flag = 0;

    if (!init_flag) {
        memset(&ctx, 0, sizeof(ctx));
        mpi_enc_slt_ctx_init(&ctx);
        init_flag = 1;
    }

    return &ctx;
}

MPP_RET setup_enc_slt_cmd(MpiEncTestArgs *cmd, const CodecInfo *config)
{
    MPP_RET ret = MPP_OK;

    cmd->width = config->width;
    cmd->height = config->height;
    cmd->frame_num = config->frame_num;
    cmd->type = (MppCodingType)config->coding_type;
    cmd->format = (MppFrameFormat)config->format;
    cmd->nthreads = config->nthreads;
    cmd->quiet = 1;

    if (cmd->file_input) MPP_FREE(cmd->file_input);
    cmd->file_input = mpp_calloc(char, strlen(config->input_file) + 1);
    strcpy(cmd->file_input, config->input_file);

    if (cmd->file_output) MPP_FREE(cmd->file_output);
    cmd->file_output = mpp_calloc(char, strlen(config->output_file) + 1);
    strcpy(cmd->file_output, config->output_file);

    if (cmd->file_slt) MPP_FREE(cmd->file_slt);
    cmd->file_slt = mpp_calloc(char, strlen(config->verify_file) + 1);
    strcpy(cmd->file_slt, config->verify_file);

    if (cmd->file_slt_gold) MPP_FREE(cmd->file_slt_gold);
    cmd->file_slt_gold = mpp_calloc(char, strlen(config->file_slt_gold) + 1);
    strcpy(cmd->file_slt_gold, config->file_slt_gold);

    cmd->rc_mode = MPP_ENC_RC_MODE_FIXQP;

    if (!cmd->hor_stride)
        cmd->hor_stride = mpi_enc_width_default_stride(cmd->width, cmd->format);
    if (!cmd->ver_stride)
        cmd->ver_stride = MPP_ALIGN(cmd->height, 2);

    if (cmd->width <= 0 || cmd->height <= 0 ||
        cmd->hor_stride <= 0 || cmd->ver_stride <= 0) {
        mpp_err("invalid w:h [%d:%d] stride [%d:%d]\n",
                cmd->width, cmd->height, cmd->hor_stride, cmd->ver_stride);
        ret = MPP_NOK;
    }

    if (cmd->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        if (!cmd->qp_init) {
            if (cmd->type == MPP_VIDEO_CodingAVC ||
                cmd->type == MPP_VIDEO_CodingHEVC)
                cmd->qp_init = 26;
        }
    }
    return ret;
}

void print_test_configuration(CodecInfo *config)
{
    mpp_log("\n========= Starting test: %s ========\n", config->test_name);
    mpp_log("  Coding Type   : %d\n", config->coding_type);
    mpp_log("  Pixel Format  : %d\n", config->format);
    mpp_log("  Resolution    : %dx%d\n", config->width, config->height);
    mpp_log("  Frame Count   : %d\n", config->frame_num);
    mpp_log("  Thread Count  : %d\n", config->nthreads);
    mpp_log("  Loop Count    : %d\n", config->loop_count);
    mpp_log("  Input File    : %s\n", config->input_file[0] ? config->input_file : "CPU created");
    mpp_log("  Output File   : %s\n", config->output_file);
    mpp_log("  Gold File     : %s\n", config->file_slt_gold);
    mpp_log("  Verify File   : %s\n", config->verify_file);
    mpp_log("===========================================================\n\n");
}

MPP_RET enc_slt_single_test(CodecInfo *config, AutoTestResult *result)
{
    RK_RET ret = MPP_OK;
    MpiEncTestArgs *cmd = mpi_enc_test_cmd_get();
    if (!cmd) return MPP_ERR_MALLOC;

    ret = setup_enc_slt_cmd(cmd, config);
    if (ret) {
        mpp_log("Failed to setup command for test %s, error %d\n", config->test_name, ret);
        mpi_enc_test_cmd_put(cmd);
        return ret;
    }

    strcpy(result->test_name, config->test_name);
    result->success_count = 0;
    result->fail_count = 0;
    result->nthreads = config->nthreads;
    result->hw_status = 0;
    result->hw_time = 0;
    result->hw_fps = 0;
    result->sft_fps = 0;
    result->sft_time = 0;
    result->stream_size = 0;
    result->pass = 0;
    result->verify = 0;

    print_test_configuration(config);

    for (RK_U32 loop = 0; loop < config->loop_count; loop++) {
        ret = enc_test_multi(cmd, config->test_name, result);
        if (ret == MPP_OK)
            result->success_count++;
        else
            result->fail_count++;
    }

    mpp_log("\n============== Test Case Result ===============\n");
    mpp_log("Case result: Test Name      : %s\n", result->test_name);
    mpp_log("Case result: Pass           : %s\n", result->pass ? "YES" : "NO");
    mpp_log("=================================================\n");

    mpi_enc_test_cmd_put(cmd);

    if (result->fail_count) {
        mpp_err("Test %s failed with %d errors\n", config->test_name, result->fail_count);
        return MPP_NOK;
    } else {
        return MPP_OK;
    }
}

void* enc_slt_multi_thread(void *arg)
{
    AutoEncThreadArg *thread_arg = (AutoEncThreadArg *)arg;
    CodecInfo *codec_info = thread_arg->codec_info;

    thread_arg->ret_code = enc_slt_single_test(codec_info, &codec_info->test_result);

    return NULL;
}

MPP_RET mpi_enc_slt_get_case(MpiEncSltCtx *ctx, MppEncSltCfgMode cfg_mode)
{
    if (!ctx || !cfg_mode) {
        mpp_err("Invalid context or config mode\n");
        return MPP_NOK;
    }

    RK_U32 total_count = 0;

    if (ctx->test_cases) {
        MPP_FREE(ctx->test_cases);
        ctx->test_cases = NULL;
        ctx->test_count = 0;
    }

    if (cfg_mode == MPP_SLT_CONGIG_OUT) {
        mpp_log("Using external SLT configuration\n");
        mpi_enc_slt_rd_case_file(ctx);
    } else if (cfg_mode == MPP_SLT_CONGIG_DEFAULT) {
        mpp_log("Using default SLT configuration\n");
        if (ctx->slt_type & MPP_ENC_SLT_BASE_ALL) {
            total_count += MPP_ARRAY_ELEMS(base_test);
        }
        if (ctx->slt_type & MPP_ENC_SLT_MODULE_ALL) {
            total_count += MPP_ARRAY_ELEMS(module_test);
        }
        if (ctx->slt_type & MPP_ENC_SLT_STRESS_ALL) {
            total_count += MPP_ARRAY_ELEMS(stress_test);
        }

        if (total_count == 0) {
            mpp_err("No test cases match the slt_type 0x%x\n", ctx->slt_type);
            return MPP_NOK;
        }

        ctx->test_cases = mpp_calloc(CodecInfo, total_count);
        if (!ctx->test_cases) {
            mpp_err("Failed to allocate memory for test cases\n");
            return MPP_ERR_MALLOC;
        }

        RK_U32 index = 0;
        if (ctx->slt_type & MPP_ENC_SLT_BASE_ALL) {
            memcpy(&ctx->test_cases[index], base_test, sizeof(base_test));
            index += MPP_ARRAY_ELEMS(base_test);
        }

        if (ctx->slt_type & MPP_ENC_SLT_MODULE_ALL) {
            memcpy(&ctx->test_cases[index], module_test, sizeof(module_test));
            index += MPP_ARRAY_ELEMS(module_test);
        }

        if (ctx->slt_type & MPP_ENC_SLT_STRESS_ALL) {
            memcpy(&ctx->test_cases[index], stress_test, sizeof(stress_test));
            index += MPP_ARRAY_ELEMS(stress_test);
        }

        ctx->test_count = total_count;
        mpp_log("Successfully loaded %d test cases\n", ctx->test_count);

        RK_U32 base_count = 0, module_count = 0, stress_count = 0;
        for (RK_U32 i = 0; i < ctx->test_count; i++) {
            const char *test_type = ctx->test_cases[i].test_type;
            if (!strcmp(test_type, "base")) {
                base_count++;
            } else if (!strcmp(test_type, "ntrd") || !strcmp(test_type, "module")) {
                module_count++;
            } else if (!strcmp(test_type, "load") || !strcmp(test_type, "stress")) {
                stress_count++;
            }
        }

        mpp_log("Test case breakdown: base=%d, module=%d, stress=%d\n",
                base_count, module_count, stress_count);

    } else {
        mpp_err("Unknown SLT configuration type %d\n", cfg_mode);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpi_enc_slt_save_result(MpiEncSltCtx *ctx)
{
    if (!ctx) {
        mpp_err("Invalid context for saving SLT results\n");
        return MPP_NOK;
    }

    if (!ctx->test_cases || ctx->test_count == 0) {
        mpp_log("No test results to save\n");
        return MPP_NOK;
    }

    RK_U32 total_pass = 0;
    RK_U32 total_verify_success = 0;
    RK_U32 total_verify_fail = 0;
    RK_U32 base_pass = 0, module_pass = 0, stress_pass = 0;

    for (RK_U32 i = 0; i < ctx->test_count; i++) {
        AutoTestResult *result = &ctx->test_cases[i].test_result;
        const char *test_type = ctx->test_cases[i].test_type;

        if (result->pass) {
            total_pass++;
            if (!strcmp(test_type, "base")) {
                base_pass++;
            } else if (!strcmp(test_type, "module")) {
                module_pass++;
            } else if (!strcmp(test_type, "stress")) {
                stress_pass++;
            }
        }

        if (result->verify && result->pass)
            total_verify_success++;
        else
            total_verify_fail++;
    }

    char report_file[512];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    snprintf(report_file, sizeof(report_file),
             "%s/result/slt_test_results_%04d-%02d-%02d_%02d-%02d-%02d.txt",
             ctx->slt_base_path,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);

    FILE *fp = fopen(report_file, "w");
    if (!fp) {
        mpp_err("Failed to create SLT result file %s\n", report_file);
        return MPP_NOK;
    }

#define PRINT_BOTH(format, ...) do { \
        fprintf(fp, format, ##__VA_ARGS__); \
        mpp_log(format, ##__VA_ARGS__); \
    } while(0)

    PRINT_BOTH("MPP Encoder SLT Test Results\n");
    PRINT_BOTH("Generated at: %s", ctime(&now));
    PRINT_BOTH("========================================\n");
    PRINT_BOTH("SUMMARY:\n");
    PRINT_BOTH("  Total_Tests: %d\n", ctx->test_count);
    PRINT_BOTH("  Pass_Tests: %d\n", total_pass);
    PRINT_BOTH("  Fail_Tests: %d\n", ctx->test_count - total_pass);
    PRINT_BOTH("  Pass_Rate: %.1f%%\n", (float)total_pass * 100 / ctx->test_count);
    PRINT_BOTH("  Verify_Success: %d\n", total_verify_success);
    PRINT_BOTH("  Verify_Fail: %d\n", total_verify_fail);
    PRINT_BOTH("\n");
    PRINT_BOTH("  Base_Tests: %d (Pass: %d)\n", ctx->base_test_count, base_pass);
    PRINT_BOTH("  Module_Tests: %d (Pass: %d)\n", ctx->module_test_count, module_pass);
    PRINT_BOTH("  Stress_Tests: %d (Pass: %d)\n", ctx->stress_test_count, stress_pass);
    PRINT_BOTH("========================================\n\n");

    if (ctx->test_count - total_pass > 0) {
        PRINT_BOTH("FAILED TESTS:\n");
        for (RK_U32 i = 0; i < ctx->test_count; i++) {
            AutoTestResult *result = &ctx->test_cases[i].test_result;
            if (!result->pass) {
                PRINT_BOTH("  Test_%03d: %s (Type: %s, Verify: %s)\n",
                           i + 1, result->test_name, ctx->test_cases[i].test_type,
                           result->verify ? "success" : "failed");
            }
        }
        PRINT_BOTH("\n");
    }

#undef PRINT_BOTH

    fprintf(fp, "========================================\n");
    fprintf(fp, "DETAILED RESULTS:\n\n");

    for (RK_U32 i = 0; i < ctx->test_count; i++) {
        AutoTestResult *result = &ctx->test_cases[i].test_result;
        const char *test_type = ctx->test_cases[i].test_type;

        fprintf(fp, "Test_%03d: %s\n", i + 1, result->test_name);
        fprintf(fp, "  Type: %s\n", test_type);
        fprintf(fp, "  Status: %s\n", result->pass ? "PASS" : "FAIL");
        fprintf(fp, "  Verify: %s\n", result->verify ? "success" : "failed");
        fprintf(fp, "  Success_Count: %u\n", result->success_count);
        fprintf(fp, "  Fail_Count: %u\n", result->fail_count);
        fprintf(fp, "  Threads: %d\n", result->nthreads);
        fprintf(fp, "  HW_Status: 0x%08x\n", result->hw_status);
        fprintf(fp, "  HW_Time: %u ms\n", result->hw_time);
        fprintf(fp, "  HW_FPS: %.2f\n", result->hw_fps);
        fprintf(fp, "  Soft_Time: %u ms\n", result->sft_time);
        fprintf(fp, "  Soft_FPS: %.2f\n", result->sft_fps);
        fprintf(fp, "  Stream_Size: %u bytes\n", result->stream_size);
    }

    fclose(fp);

    mpp_log("Results saved to: %s\n", report_file);
    mpp_log("================================\n");

    if (total_verify_fail > 0 || total_pass < ctx->test_count)
        return MPP_NOK;
    else
        return MPP_OK;
}

MPP_RET mpi_enc_slt_run(MpiEncSltCtx * ctx)
{
    MPP_RET ret = MPP_OK;
    RK_U32  stress_idx = 0;

    if (!ctx) {
        mpp_err("Invalid context for mpi_enc_slt_run\n");
        return MPP_NOK;
    }

    for (RK_U32 i = 0; i < ctx->test_count; i++) {
        CodecInfo *test_case = &ctx->test_cases[i];
        generate_test_names(test_case);
        mpp_log("Test case %02d: %s\n", i + 1, test_case->test_name);
    }

    for (RK_U32 i = 0; i < ctx->test_count; i++) {
        CodecInfo *test_case = &ctx->test_cases[i];
        if (!strcmp(test_case->test_type, "base")) {
            ctx->base_test_count++;
            ret = enc_slt_single_test(test_case, &test_case->test_result);
            if (ret) {
                mpp_err("Test case %02d (%s) failed with error %d\n", i + 1, test_case->test_name, ret);
            }
        } else if (!strcmp(test_case->test_type, "module")) {
            ctx->module_test_count++;
            ret = enc_slt_single_test(test_case, &test_case->test_result);
            if (ret) {
                mpp_err("Test case %02d (%s) failed with error %d\n", i + 1, test_case->test_name, ret);
            }
        } else {
            ctx->stress_test_count++;
        }
    }

    AutoEncThreadArg *thread_args = mpp_calloc(AutoEncThreadArg, ctx->stress_test_count);
    pthread_t *threads = mpp_calloc(pthread_t, ctx->stress_test_count);
    if (!thread_args || !threads) {
        mpp_err("Failed to allocate memory for stress test threads\n");
        return MPP_ERR_MALLOC;
    }

    for (RK_U32 i = 0; i < ctx->test_count; i++) {
        CodecInfo *test_case = &ctx->test_cases[i];
        if (!strcmp(test_case->test_type, "stress")) {
            thread_args[stress_idx].codec_info = test_case;
            thread_args[stress_idx].ret_code = MPP_NOK;
            ret = pthread_create(&threads[stress_idx], NULL, enc_slt_multi_thread, &thread_args[stress_idx]);
            if (ret) {
                mpp_err("Failed to create stress test thread %d for %s\n", stress_idx, test_case->test_name);
                for (RK_U32 j = 0; j < stress_idx; j++) {
                    pthread_join(threads[j], NULL);
                }
                MPP_FREE(thread_args);
                MPP_FREE(threads);
                return MPP_NOK;
            }
            stress_idx++;
        }
    }

    for (RK_U32 i = 0; i < stress_idx; i++) {
        pthread_join(threads[i], NULL);
        if (thread_args[i].ret_code != MPP_OK) {
            mpp_err("Stress test %s failed with error %d\n", thread_args[i].codec_info->test_name, thread_args[i].ret_code);
        } else {
            mpp_log("Stress test %s passed\n", thread_args[i].codec_info->test_name);
        }
    }

    MPP_FREE(thread_args);
    MPP_FREE(threads);
    return ret;
}

int main(int argc, char **argv)
{
    int ret = MPP_NOK;

    set_dmc_performance_mode();
    if (argc < 2) {
        show_usage(argv[0]);
        restore_dmc_governor_mode();
        return ret;
    }

    MpiEncSltCtx* slt_ctx = mpi_enc_slt_get_ctx();
    if (!slt_ctx) {
        mpp_err("Failed to get SLT context\n");
        restore_dmc_governor_mode();
        return ret;
    }

    ret = mpi_enc_slt_dir_init(slt_ctx->slt_base_path);
    if (ret != MPP_OK) {
        mpp_err("Failed to initialize SLT directories\n");
        restore_dmc_governor_mode();
        return ret;
    }

    ret = mpi_enc_slt_ctx_update_by_args(slt_ctx, argc, argv);
    if (ret != MPP_OK) {
        show_usage(argv[0]);
        restore_dmc_governor_mode();
        return ret;
    }

    ret = mpi_enc_slt_get_case(slt_ctx, slt_ctx->slt_cfg_type);
    if (ret != MPP_OK) {
        mpp_err("Failed to get SLT test case\n");
        return ret;
    }

    ret = mpi_enc_slt_run(slt_ctx);
    if (ret != MPP_OK) {
        mpp_err("Failed to begin SLT with error %d\n", ret);
        mpi_enc_slt_ctx_deinit(slt_ctx);
        restore_dmc_governor_mode();
        return ret;
    }

    ret = mpi_enc_slt_save_result(slt_ctx);
    if (ret) {
        mpp_err("SLT results: failed, return %d\n", ret);
    }

    mpi_enc_slt_ctx_deinit(slt_ctx);
    restore_dmc_governor_mode();
    return ret;
}
