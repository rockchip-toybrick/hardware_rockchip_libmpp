/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */
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
#include "mpp_rc_api.h"

typedef enum {
    MPP_ENC_SLT_BASE_ALL        = 0x00000001,
    MPP_ENC_SLT_BASE_MJPEG,
    MPP_ENC_SLT_BASE_AVC,
    MPP_ENC_SLT_BASE_HEVC,
    MPP_ENC_SLT_BASE_VP8,

    MPP_ENC_SLT_MODULE_ALL      = 0x00000010,
    MPP_ENC_SLT_MODULE_MJPEG,
    MPP_ENC_SLT_MODULE_AVC,
    MPP_ENC_SLT_MODULE_HEVC,
    MPP_ENC_SLT_MODULE_VP8,

    MPP_ENC_SLT_STRESS_ALL      = 0x00000100,
    MPP_ENC_SLT_STRESS_MJPEG,
    MPP_ENC_SLT_STRESS_AVC,
    MPP_ENC_SLT_STRESS_HEVC,
    MPP_ENC_SLT_STRESS_VP8,

    MPP_ENC_SLT_TEST_ALL = MPP_ENC_SLT_BASE_ALL | MPP_ENC_SLT_MODULE_ALL | MPP_ENC_SLT_STRESS_ALL,
} MppEncSltType;

typedef enum {
    MPP_SLT_CONGIG_NONE     = 0,
    MPP_SLT_CONGIG_DEFAULT  = 1,
    MPP_SLT_CONGIG_OUT      = 2,
} MppEncSltCfgMode;

typedef struct {
    char test_name[128];
    RK_U32      success_count;
    RK_U32      fail_count;
    RK_FLOAT    avg_fps;
    RK_U32      pass;
    RK_U32      vepu_type;
    RK_U32      nthreads;
    RK_U32      hw_status;
    RK_U32      sft_time;
    RK_FLOAT    sft_fps;
    RK_U32      stream_size;
    RK_U32      hw_time;
    RK_FLOAT    hw_fps;
    RK_U32      verify; // 1: success, 0: failed
} AutoTestResult;

typedef struct {
    const char *test_type;
    RK_U32 coding_type;
    RK_U32 width;
    RK_U32 height;
    RK_U32 format;
    RK_U32 loop_count;
    RK_U32 frame_num;
    RK_U32 nthreads;

    char test_name[128];
    char input_file[512];
    char output_file[512];
    char verify_file[512];
    char file_slt_gold[512];
    AutoTestResult test_result;
} CodecInfo;

typedef struct {
    MppEncSltType slt_type;
    MppEncSltCfgMode slt_cfg_type;

    char *slt_base_path;
    char *config_file;
    char *test_suite;

    CodecInfo *test_cases;
    RK_U32 test_count;
    RK_U32 base_test_count;
    RK_U32 module_test_count;
    RK_U32 stress_test_count;
} MpiEncSltCtx;

static pthread_mutex_t g_file_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    CodecInfo *codec_info;
    MPP_RET ret_code;
} AutoEncThreadArg;

static CodecInfo base_test[] = {
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_YUV420P,        1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_YUV420SP,       1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_YUV422_YUYV,    1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_RGB444,         1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_RGB555,         1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_RGB565,         1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_BGRA8888,       1,  10, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingMJPEG, 8176,   8176,   MPP_FMT_RGB101010,      1,  10, 1, "", "", "", "", "", {0}},

    {"base", MPP_VIDEO_CodingAVC,   7680,   4320,   MPP_FMT_YUV420P,        1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingAVC,   7680,   4320,   MPP_FMT_YUV420SP,       1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingAVC,   7680,   4320,   MPP_FMT_YUV422SP,       1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingAVC,   7680,   4320,   MPP_FMT_YUV422_YUYV,    1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingAVC,   7680,   4320,   MPP_FMT_RGB888,         1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingAVC,   7680,   4320,   MPP_FMT_ARGB8888,       1,  60, 1, "", "", "", "", "", {0}},

    {"base", MPP_VIDEO_CodingHEVC,  7680,   4320,   MPP_FMT_YUV420P,        1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingHEVC,  7680,   4320,   MPP_FMT_YUV420SP,       1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingHEVC,  7680,   4320,   MPP_FMT_YUV422SP,       1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingHEVC,  7680,   4320,   MPP_FMT_YUV422_YUYV,    1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingHEVC,  7680,   4320,   MPP_FMT_RGB888,         1,  60, 1, "", "", "", "", "", {0}},
    {"base", MPP_VIDEO_CodingHEVC,  7680,   4320,   MPP_FMT_ARGB8888,       1,  60, 1, "", "", "", "", "", {0}},
};

static CodecInfo module_test[] = {
    // {"module", MPP_VIDEO_CodingMJPEG, 1920,   1080,   MPP_FMT_YUV420SP,       1,  1 , 4, "", "output_250_350.yuv", "", "", "", {0}},
    // {"module", MPP_VIDEO_CodingHEVC,  1920,   1080,   MPP_FMT_YUV420SP,       1,  30, 4, "", "output_250_350.yuv", "", "", "", {0}},
    // {"module", MPP_VIDEO_CodingAVC,   1920,   1080,   MPP_FMT_YUV420SP,       1,  30, 4, "", "output_250_350.yuv",  "", "", "", {0}},
    {"module", MPP_VIDEO_CodingMJPEG, 1920,   1080,   MPP_FMT_YUV420SP,       1,  120, 4, "", "", "", "", "", {0}},
    {"module", MPP_VIDEO_CodingHEVC,  3840,   2160,   MPP_FMT_YUV420SP,       1,  120, 4, "", "", "", "", "", {0}},
    {"module", MPP_VIDEO_CodingAVC,   3840,   2160,   MPP_FMT_YUV420SP,       1,  120, 4, "", "",  "", "", "", {0}},
};

static CodecInfo stress_test[] = {
    {"stress", MPP_VIDEO_CodingMJPEG, 3840,   2160,   MPP_FMT_YUV420SP,       1,  300, 1, "", "", "", "", "", {0}},
    {"stress", MPP_VIDEO_CodingHEVC,  3840,   2160,   MPP_FMT_YUV420SP,       1,  300, 1, "", "", "", "", "", {0}},
    {"stress", MPP_VIDEO_CodingAVC,   3840,   2160,   MPP_FMT_YUV420SP,       1,  300, 1, "", "", "", "", "", {0}},
};

MPP_RET mpi_enc_slt_rd_case_file(MpiEncSltCtx *ctx);
void generate_test_names(CodecInfo *codec);
void show_usage(const char *program_name);
MPP_RET set_dmc_performance_mode(void);
MPP_RET restore_dmc_governor_mode(void);