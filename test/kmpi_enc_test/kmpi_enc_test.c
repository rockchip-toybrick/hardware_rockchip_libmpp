/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/module.h>

#include "rk_mpi_cmd.h"
#include "mpp_vcodec_base.h"
#include "mpp_vcodec_chan.h"
#include "mpp_buffer.h"
#include "mpp_enc.h"

#include "kmpp_osal.h"
#include "kmpi_venc.h"

#include "kmpp_frame.h"
#include "kmpp_packet.h"

static RK_U32 frame_num = 10;
module_param(frame_num, uint, 0644);

int mpp_enc_cfg_setup(KmppCtx ctx)
{
    RK_U32 width = 1280;
    RK_U32 height = 720;
    RK_U32 hor_stride = 1280;
    RK_U32 ver_stride = 720;
    RK_S32 fps_in_flex = 0;
    RK_S32 fps_in_den = 1;
    RK_S32 fps_in_num = 25;
    RK_S32 fps_out_flex = 0;
    RK_S32 fps_out_den = 1;
    RK_S32 fps_out_num = 25;
    RK_S32 bps = 0;
    RK_S32 bps_max = 0;
    RK_S32 bps_min = 0;
    RK_S32 rc_mode = MPP_ENC_RC_MODE_CBR;
    RK_U32 gop_len = 60;
    MppCodingType type = MPP_VIDEO_CodingHEVC;
    MppEncCfg cfg = NULL;
    int ret;

    mpp_enc_cfg_init(&cfg);

    if (!bps)
        bps = width * height / 8 * (fps_out_num / fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "prep:width", width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", hor_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", ver_stride);
    mpp_enc_cfg_set_s32(cfg, "prep:format", 0);
    mpp_enc_cfg_set_s32(cfg, "rc:mode", rc_mode);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "rc:gop",
                gop_len ? gop_len : fps_out_num * 2);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode",
                MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);    /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);    /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", bps);
    switch (rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP: {

        /* do not setup bitrate on FIXQP mode */
    }
    break;
    case MPP_ENC_RC_MODE_CBR: {

        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max",
                    bps_max ? bps_max : bps * 17 /
                    16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min",
                    bps_min ? bps_min : bps * 15 / 16);
    }
    break;
    case MPP_ENC_RC_MODE_VBR:
    case MPP_ENC_RC_MODE_AVBR:
    case MPP_ENC_RC_MODE_SMTRC: {

        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max",
                    bps_max ? bps_max : bps * 17 /
                    16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min",
                    bps_min ? bps_min : bps * 1 / 16);
    }
    break;
    default: {

        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max",
                    bps_max ? bps_max : bps * 17 /
                    16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min",
                    bps_min ? bps_min : bps * 15 / 16);
    }
    break;
    }

    /* setup qp for different codec and rc_mode */
    switch (type) {
    case MPP_VIDEO_CodingAVC:
    case MPP_VIDEO_CodingHEVC: {
        switch (rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP: {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init",
                        20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max",
                        20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min",
                        20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i",
                        20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i",
                        20);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip",
                        2);
        }
        break;
        case MPP_ENC_RC_MODE_CBR:
        case MPP_ENC_RC_MODE_VBR:
        case MPP_ENC_RC_MODE_AVBR: {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init",
                        26);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max",
                        51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min",
                        10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i",
                        51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i",
                        10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip",
                        2);
        }
        break;
        default: {
            kmpp_loge_f("unsupport encoder rc mode %d\n", rc_mode);
        }
        break;
        }
    }
    break;
    case MPP_VIDEO_CodingVP8: {

        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    }
    break;
    case MPP_VIDEO_CodingMJPEG: {

        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    }
    break;
    default: {
    }
    break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", type);
    switch (type) {
    case MPP_VIDEO_CodingAVC: {

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
    }
    break;
    case MPP_VIDEO_CodingHEVC:
    case MPP_VIDEO_CodingMJPEG:
    case MPP_VIDEO_CodingVP8: {
    }
    break;
    default: {
        kmpp_loge_f("unsupport encoder coding type %d\n", type);
    }
    break;
    }

    ret = kmpp_venc_ctrl(ctx, MPP_ENC_SET_CFG, cfg);

    mpp_enc_cfg_deinit(cfg);

    return ret;
}

void enc_test(void)
{
    RK_S32 ret;
    KmppCtx ctx = NULL;
    MppEncInitCfg cfg;
    RK_U32 frame_count = 0;
    MppBuffer buffer;
    KmppFrame frame = NULL;

    memset(&cfg, 0, sizeof(cfg));
    mpp_enc_cfg_api_init();
    kmpp_err_f("mpp_enc_cfg_api_init ok");

    cfg.coding         = MPP_VIDEO_CodingAVC,
    cfg.chan_id        = -1,

    ret = kmpp_venc_init(&ctx, &cfg);
    if (ret) {
        kmpp_err_f("kmpp_venc_init failed ret %d\n", ret);
        return;
    }

    ret = mpp_enc_cfg_setup(ctx);
    if (ret) {
        kmpp_err_f("mpp_enc_cfg_setup failed ret %d\n", ret);
        return;
    }

    kmpp_venc_start(ctx, &cfg);

    while (frame_count++ < frame_num) {
        KmppPacket packet = NULL;
        RK_U32 size = 1280 * 720 * 3 / 2;
        RK_U32 width = 1280;
        RK_U32 height = 720;
        RK_U32 hor_stride = 1280;
        RK_U32 ver_stride = 720;

        mpp_buffer_get(NULL, &buffer, size);

        kmpp_frame_get(&frame);
        kmpp_frame_set_width(frame, width);
        kmpp_frame_set_height(frame, height);
        kmpp_frame_set_hor_stride(frame, hor_stride);
        kmpp_frame_set_ver_stride(frame, ver_stride);
        kmpp_frame_set_fmt(frame, 0);
        kmpp_frame_set_buffer(frame, buffer);
        mpp_buffer_put(buffer);

        kmpp_venc_put_frm(ctx, frame);

        kmpp_venc_get_pkt(ctx, &packet);
        if (packet) {
            RK_U32 len;

            kmpp_packet_get_length(packet, &len);
            kmpp_err_f("get stream size %d\n", len);
            kmpp_packet_put(packet);
        }
    }

    kmpp_venc_stop(ctx, &cfg);
    kmpp_venc_deinit(ctx, &cfg);
    mpp_enc_cfg_api_deinit();
}

rk_s32 kmpi_enc_test_func(void *param)
{
    enc_test();

    return rk_ok;
}

static osal_worker *test_worker = NULL;
static osal_work *test_work = NULL;

void kmpi_enc_test_prepare(void)
{
    kmpp_logi("kmpp enc test start\n");
}

void kmpi_enc_test_finish(void)
{
    kmpp_logi("kmpp enc test end\n");
}

int kmpi_enc_test_init(void)
{
    kmpi_enc_test_prepare();

    osal_worker_init(&test_worker, "kmpp enc test");
    osal_work_init(&test_work, kmpi_enc_test_func, NULL);

    osal_work_queue(test_worker, test_work);

    return 0;
}

void kmpi_enc_test_exit(void)
{
    osal_worker_deinit(&test_worker);
    osal_work_deinit(&test_work);

    kmpi_enc_test_finish();
}

module_init(kmpi_enc_test_init);
module_exit(kmpi_enc_test_exit);

MODULE_AUTHOR("rockchip");
MODULE_LICENSE("proprietary");
MODULE_VERSION("1.0");
