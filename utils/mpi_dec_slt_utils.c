/*************************************************************************
    > File Name: utils/mpi_dec_slt_utils.c
    > Author: LiHongjin
    > Mail: 872648180@qq.com
    > Created Time: Mon 02 Jun 2025 04:38:37 PM CST
 ************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <getopt.h>

#include "rk_mpi.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpi_dec_utils.h"
#include "mpi_dec_slt_utils.h"


static void print_usage(FILE* stream, int exit_code, const char *exe_name)
{
    fprintf(stream, "Usage: %s [-c config_file] [-o result_file]\n", exe_name);
    fprintf(stream,
            "  -d|--debug  enable slt debug.\n"
            "  -c|--config slt config file.\n"
            "  -o|--result slt test result.\n");
    exit(exit_code);
}

int proc_paras(int argc, char* argv[], dec_slt_cmd_paras *cmd_paras)
{
    const char* exe_name = argv[0];
    int opt;
    const char* const short_options = "hdc:o:";
    const struct option long_options[] = {
        { "help",    no_argument,       NULL, 'h' },
        { "debug",   no_argument,       NULL, 'd' },
        { "config",  required_argument, NULL, 'c' },
        { "result",  required_argument, NULL, 'o' },
        { NULL, 0, NULL, 0 }
    };

    cmd_paras->config_f = SLT_DEF_CFG_FILE;
    cmd_paras->result_o = SLT_DEF_RES_FILE;

    while ((opt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1) {
        switch (opt) {
        case 'h': {
            print_usage (stdout, 0, exe_name);
        } break;
        case 'd': {
            cmd_paras->en_debug = true;
        } break;
        case 'c': {
            cmd_paras->config_f = optarg;
        } break;
        case 'o': {
            cmd_paras->result_o = optarg;
        } break;
        case '?': {
            print_usage (stderr, 1, exe_name);
        } break;
        default:
            abort ();
        }
    }

    printf("======> cmd paras <======\n");
    printf("en_dbg: %d\n", cmd_paras->en_debug);
    printf("config: %s\n", cmd_paras->config_f);
    printf("output: %s\n", cmd_paras->result_o);
    printf("\n");

    return 0;
}

void write_config_file(const char* filename)
{
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        printf("Failed to create config file %s\n", filename);
        return;
    }

    // Write configuration items
    char f_mpg[]      = "/sdcard/3588_slt/test_mpg_1920x1080_nv12.mpg";
    char f_m2v[]      = "/sdcard/3588_slt/test_m2v_1920x1080_nv12.m2v";
    char f_m4v[]      = "/sdcard/3588_slt/test_m4v_1920x1080_nv12_2.m4v";
    /* char f_h263[]     = "/sdcard/3588_slt/test_h263_704x576_nv12.h263"; */
    char f_h263[]     = "/sdcard/3588_slt/test_h263_1408x1152_nv12.h263";
    char f_h264_400[] = "/sdcard/3588_slt/test_h264_7680x4320_gray.h264";
    char f_h264_420[] = "/sdcard/3588_slt/test_h264_7680x4320_nv12.h264";
    char f_h264_422[] = "/sdcard/3588_slt/test_h264_7680x4320_nv16.h264";
    char f_h265_4k[]  = "/sdcard/3588_slt/test_h265_3840x2160_nv12.h265";
    char f_h265_8k[]  = "/sdcard/3588_slt/test_h265_7680x4320_nv12.h265";
    char f_vp9[]      = "/sdcard/3588_slt/test_vp9_7680x4320_nv12.ivf";
    char f_av1_2k[]   = "/sdcard/3588_slt/test_av1_2560x1440_nv12.ivf";
    char f_av1_4k[]   = "/sdcard/3588_slt/test_av1_3840x2160_nv12.ivf";
    char f_avs2[]     = "/sdcard/3588_slt/test_avs2_7680x4320_nv12.avs2";
    char f_jpeg_400[] = "/sdcard/3588_slt/test_jpg_1920x1080_gray.jpg";
    char f_jpeg_411[] = "/sdcard/3588_slt/test_jpg_1920x1080_yuv411p.jpg";
    char f_jpeg_420[] = "/sdcard/3588_slt/test_jpg_1920x1080_nv12.jpg";
    char f_jpeg_422[] = "/sdcard/3588_slt/test_jpg_1920x1080_nv16.jpg";
    char f_jpeg_440[] = "/sdcard/3588_slt/test_jpg_1920x1080_nv24.jpg";
    char f_jpeg_444[] = "/sdcard/3588_slt/test_jpg_1920x1080_nv42.jpg";

    (void)f_mpg;
    (void)f_m2v;
    (void)f_m4v;
    (void)f_h264_400;
    (void)f_h264_420;
    (void)f_h264_422;
    (void)f_h265_4k;
    (void)f_h265_8k;
    (void)f_vp9;
    (void)f_av1_2k;
    (void)f_av1_4k;
    (void)f_avs2;
    (void)f_jpeg_400;
    (void)f_jpeg_411;
    (void)f_jpeg_420;
    (void)f_jpeg_422;
    (void)f_jpeg_440;
    (void)f_jpeg_444;

    /* fprintf(file, "plt:rk3588 type:base in:%s codec_type:2        width:1920 height:1080 thd:1  o_fps:60  o_fmt:0 \n", f_mpg); */
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:2        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_m2v);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:4        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_m4v);
    /* fprintf(file, "plt:rk3588 type:base in:%s codec_type:3        width:704  height:576  thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_h263); */
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:3        width:1408 height:1152 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_h263);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:7        width:7680 height:4320 thd:1  core_cnt:1  o_fps:30  o_fmt:0 \n", f_h264_400);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:7        width:7680 height:4320 thd:1  core_cnt:1  o_fps:30  o_fmt:0 \n", f_h264_420);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:7        width:7680 height:4320 thd:1  core_cnt:1  o_fps:30  o_fmt:0 \n", f_h264_422);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:16777220 width:3840 height:2160 thd:1  core_cnt:2  o_fps:60  o_fmt:0 \n", f_h265_4k);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:16777220 width:7680 height:4320 thd:1  core_cnt:2  o_fps:60  o_fmt:0 \n", f_h265_8k);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:10       width:7680 height:4320 thd:1  core_cnt:2  o_fps:60  o_fmt:0 \n", f_vp9);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:16777224 width:2560 height:1440 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_av1_2k);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:16777224 width:3840 height:2160 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_av1_4k);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:16777223 width:2560 height:1440 thd:1  core_cnt:2  o_fps:60  o_fmt:0 \n", f_avs2);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_jpeg_400);
    /* fprintf(file, "plt:rk3588 type:base in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_jpeg_411); */
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_jpeg_420);
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_jpeg_422);
    /* fprintf(file, "plt:rk3588 type:base in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_jpeg_440); */
    fprintf(file, "plt:rk3588 type:base in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_jpeg_444);

    fprintf(file, "plt:rk3588 type:mult in:%s codec_type:4        width:1920 height:1080 thd:2  core_cnt:1  o_fps:30  o_fmt:0 \n", f_m4v);
    fprintf(file, "plt:rk3588 type:mult in:%s codec_type:16777220 width:3840 height:2160 thd:4  core_cnt:2  o_fps:30  o_fmt:0 \n", f_h265_4k);
    fprintf(file, "plt:rk3588 type:mult in:%s codec_type:16777224 width:2560 height:1440 thd:4  core_cnt:1  o_fps:60  o_fmt:0 \n", f_av1_2k);
    fprintf(file, "plt:rk3588 type:mult in:%s codec_type:8        width:1920 height:1080 thd:20 core_cnt:1  o_fps:10  o_fmt:0 \n", f_jpeg_420);

    fprintf(file, "plt:rk3588 type:stress in:%s codec_type:4        width:1920 height:1080 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_m4v);
    fprintf(file, "plt:rk3588 type:stress in:%s codec_type:16777220 width:7680 height:4320 thd:1  core_cnt:2  o_fps:60  o_fmt:0 \n", f_h265_8k);
    fprintf(file, "plt:rk3588 type:stress in:%s codec_type:16777224 width:2560 height:1440 thd:1  core_cnt:1  o_fps:60  o_fmt:0 \n", f_av1_2k);
    fprintf(file, "plt:rk3588 type:stress in:%s codec_type:8        width:1920 height:1080 thd:1  core_cnt:1  o_fps:200 o_fmt:0 \n", f_jpeg_420);

    fclose(file);
    printf("Config file %s created successfully\n", filename);
}

static void trim_string(char* str)
{
    char* start = str;

    while (isspace((unsigned char)*start)) start++;

    char* end = str + strlen(str) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;

    memmove(str, start, end - start + 1);
    str[end - start + 1] = '\0';
}

MPP_RET slt_config_init(const char* fname, slt_item **cfgs)
{
    char line[256];
    int line_num = 0;
    FILE* file = NULL;
    slt_item *cfg;
    RK_S32 i;

    cfg = mpp_calloc(slt_item, 3);

    file = fopen(fname, "r");
    if (file == NULL) {
        printf("Failed to open config file %s\n", fname);
        return MPP_NOK;
    }

    printf("\nReading config file %s:\n", fname);

    while (fgets(line, sizeof(line), file)) {
        char *element = NULL;
        char *saveptr;
        MpiDecTestCmd* d_cmd = NULL;
        slt_item *cur_slt_item = NULL;
        RockchipSocType cur_plt = ROCKCHIP_SOC_BUTT;
        slt_type cur_slt_type = MPP_SLT_BUT;
        slt_v_node cur_v = {0};
        slt_v_node *cur_v_p = NULL;

        memset(&cur_v, 0, sizeof(cur_v));

        line_num++;

        if (line[0] == '#' || line[0] == '\n')
            continue;

        line[strcspn(line, "\n")] = '\0';

        element = strtok_r(line, " ", &saveptr);
        while (element) {
            char* colon = strchr(element, ':');
            if (colon == NULL) {
                printf("Warning: Line %d has invalid format (missing ':')\n", line_num);
                continue;
            }

            // Split into key and value
            *colon = '\0';  // Temporarily terminate key
            char* key = element;
            char* value = colon + 1;

            // Trim whitespace from both
            trim_string(key);
            trim_string(value);

            // Skip empty keys
            if (strlen(key) == 0) {
                printf("Warning: Line %d has empty key\n", line_num);
                continue;
            }

            // Process different value types
            if (!strcmp(key, "plt")) {
                // Numeric value
                const MppSocInfo *soc_info = mpp_get_soc_info_by_name(value);
                cur_plt = soc_info->soc_type;
            } else if (!strcmp(key, "type")) {
                if (!strcmp(value, "base"))
                    cur_slt_type = MPP_SLT_BASE;
                else if (!strcmp(value, "mult"))
                    cur_slt_type = MPP_SLT_MOD_MULT;
                else if (!strcmp(value, "stress"))
                    cur_slt_type = MPP_SLT_STRESS;
                else
                    printf("unsupported type: %s\n", value);
            } else if (!strcmp(key, "in")) {
                mpp_assert(SLT_FNAME_LEN > strlen(value));
                strncpy(cur_v.v_name, value, SLT_FNAME_LEN);
                strncpy(cur_v.dec_cmd_ctx.file_input, value, SLT_FNAME_LEN);
                cur_v.dec_cmd_ctx.have_input = 1;
            } else if (!strcmp(key, "width")) {
                cur_v.dec_cmd_ctx.width = atoi(value);
            } else if (!strcmp(key, "height")) {
                cur_v.dec_cmd_ctx.height = atoi(value);
            } else if (!strcmp(key, "codec_type")) {
                cur_v.type = (MppCodingType)atoi(value);
                cur_v.dec_cmd_ctx.type = cur_v.type;
                if (mpp_check_support_format(MPP_CTX_DEC, cur_v.type)) {
                    printf("check support format error!\n");
                    return MPP_NOK;
                }
            } else if (!strcmp(key, "thd")) {
                cur_v.thd_cnt = atoi(value);
                cur_v.dec_cmd_ctx.nthreads = cur_v.thd_cnt;
            } else if (!strcmp(key, "core_cnt")) {
                cur_v.core_cnt = atoi(value);
            } else if (!strcmp(key, "o_fmt")) {
                long number = 0;
                MppFrameFormat format = MPP_FMT_BUTT;

                if (MPP_OK == str_to_frm_fmt(value, &number)) {
                    format = (MppFrameFormat)number;

                    if (MPP_FRAME_FMT_IS_YUV(format) || MPP_FRAME_FMT_IS_RGB(format)) {
                        cur_v.o_fmt = format;
                    } else {
                        mpp_err("invalid input format 0x%x\n", format);
                    }
                }
                cur_v.o_fmt = atoi(value);
                cur_v.dec_cmd_ctx.format = cur_v.o_fmt;
            } else if (!strcmp(key, "o_fps")) {
                cur_v.o_fps = atoi(value);
            } else {
                printf("unknow data type key:%s value:%s", key, value);
            }

            element = strtok_r(NULL, " ", &saveptr);
        }

        /* add video node */
        mpp_assert(cur_slt_type != MPP_SLT_BUT);
        mpp_assert(cur_slt_type != MPP_SLT_BUT);
        cur_slt_item = &cfg[cur_slt_type];
        cur_slt_item->plt = cur_plt;
        cur_slt_item->type = cur_slt_type;
        if (cur_slt_item->v_cnt >= cur_slt_item->v_cap) {
            int cur_cap = cur_slt_item->v_cap;
            slt_v_node *cur_p = cur_slt_item->v_nodes;
            cur_p = mpp_realloc_size(cur_p, slt_v_node, (cur_cap + 10) * sizeof(slt_v_node));
            memset(cur_p + cur_cap, 0, sizeof(slt_v_node) * 10);
            for (i = cur_cap; i < cur_cap + 10; i++)
                mpp_spinlock_init(&cur_p[i].lock);
            mpp_assert(cur_p);
            cur_slt_item->v_nodes = cur_p;
            cur_slt_item->v_cap = cur_cap + 10;
        }
        cur_v_p = &cur_slt_item->v_nodes[cur_slt_item->v_cnt++];
        memcpy(cur_v_p, &cur_v, sizeof(cur_v));
        if (cur_slt_type == MPP_SLT_BASE && cur_v_p->thd_cnt > 1) {
            printf("base test thread cnt must be 1\n");
            cur_v_p->thd_cnt = 1;
            cur_v_p->dec_cmd_ctx.nthreads = 1;
        }

        d_cmd = &cur_v_p->dec_cmd_ctx;

        if (d_cmd->have_input) {
            reader_init(&d_cmd->reader, d_cmd->file_input, d_cmd->type);
            if (d_cmd->reader)
                mpp_log("input file %s size %ld\n", d_cmd->file_input, reader_size(d_cmd->reader));
        }
    }

    fclose(file);

    *cfgs = cfg;

    return MPP_OK;
}

MPP_RET slt_config_deinit(slt_item **cfgs)
{
    RK_U32 i, j;
    slt_item *cfg = *cfgs;

    for (i = 0; i < MPP_SLT_BUT; i++) {
        MpiDecTestCmd* d_cmd = NULL;
        for (j = 0; j < cfg[i].v_cnt; j++) {
            d_cmd = &cfg[i].v_nodes[j].dec_cmd_ctx;

            if (!d_cmd)
                return MPP_NOK;

            if (d_cmd->reader) {
                reader_deinit(d_cmd->reader);
                d_cmd->reader = NULL;
            }

            if (d_cmd->fps) {
                fps_calc_deinit(d_cmd->fps);
                d_cmd->fps = NULL;
            }
        }

        mpp_free(cfg[i].v_nodes);
    }

    mpp_free(cfg);
    *cfgs = NULL;

    return MPP_OK;
}

void dump_cfg(slt_item cfg[MPP_SLT_BUT])
{
    RK_U32 i, j;
    slt_item *cur_cfg = NULL;
    slt_v_node *cur_v = NULL;
    const MppSocInfo *soc_info = NULL;

    for (i = 0; i < MPP_SLT_BUT; i++) {
        cur_cfg = &cfg[i];

        if (!cur_cfg->v_cnt)
            continue;

        soc_info = mpp_get_soc_info_by_soc_type(cur_cfg->plt);
        mpp_assert(soc_info);
        printf("plt:%s\n", soc_info->compatible);

        printf("slt type:%d\n", cur_cfg->type);
        for (j = 0; j < cur_cfg->v_cnt; j++) {
            cur_v = &cur_cfg->v_nodes[j];
            printf("type:%-8d wxh:[%4dx%-4d] thd_cnt:%-3d  core_cnt:%d  o_fmt:%-8d o_fps:%-3d  in:%s\n",
                   cur_v->type, cur_v->dec_cmd_ctx.width, cur_v->dec_cmd_ctx.height,
                   cur_v->thd_cnt, cur_v->core_cnt, cur_v->o_fmt, cur_v->o_fps, cur_v->v_name);
        }
    }

    return;
}

void slt_dec_analyze(dec_slt_cmd_paras *cmd_paras, slt_item *cfgs)
{
    RK_U32 i, j;
    slt_v_node *cur_v_p = NULL;
    const MppSocInfo *soc_info = NULL;
    FILE *fp = NULL;

    if (cmd_paras->en_debug)
        fp = stdout;
    else
        fp = fopen(cmd_paras->result_o, "w");

    for (i = 0; i < MPP_SLT_BUT; i++) {
        soc_info = mpp_get_soc_info_by_soc_type(cfgs[i].plt);
        mpp_assert(soc_info);
        for (j = 0; j < cfgs[i].v_cnt; j++) {
            cur_v_p = &cfgs[i].v_nodes[j];
            if (cur_v_p->err_info || cur_v_p->o_fps >= cur_v_p->avg_fps)
                fprintf(fp, "res:%-6s  ", "failed");
            else
                fprintf(fp, "res:%-6s  ", "pass");
            fprintf(fp, "plt:%-6s  ", soc_info->compatible);
            fprintf(fp, "slt_type:%-2d  ", cfgs[i].type);
            fprintf(fp, "type:%-8d  thd_cnt:%-3d  o_fmt:%-6x  "
                    "total_time:%-9lld  frm_cnt:%-6d  avg_fps:%-3d  max_fps:%-3d  min_fps:%-2d  o_fps:%-3d\n",
                    cur_v_p->type, cur_v_p->thd_cnt, cur_v_p->o_fmt, cur_v_p->total_time, cur_v_p->frm_cnt,
                    cur_v_p->avg_fps, cur_v_p->max_fps,
                    cur_v_p->min_fps, cur_v_p->o_fps);
        }
    }

    return;
}

