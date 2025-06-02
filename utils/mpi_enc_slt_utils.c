/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */
#ifndef __MPI_ENC_SLT_UTILS_H__
#define __MPI_ENC_SLT_UTILS_H__
#include <stdio.h>
#include <stdlib.h>
#include "mpi_enc_slt_utils.h"

void show_usage(const char *program_name)
{
    mpp_log("Usage: %s [options]\n", program_name);
    mpp_log("\nBuilt-in test suites:\n");
    mpp_log("  --all                    Run all built-in tests (base + module + stress)\n");
    mpp_log("  --base                   Run built-in base tests\n");
    mpp_log("  --module                 Run built-in module tests\n");
    mpp_log("  --stress                 Run built-in stress tests\n");
    mpp_log("\nConfig file based tests:\n");
    mpp_log("  --config [config_file]   Run tests from external config file\n");
    mpp_log("\nExamples:\n");
    mpp_log("  %s --all                         Run all built-in tests\n", program_name);
    mpp_log("  %s --base                        Run built-in base tests\n", program_name);
    mpp_log("  %s --config /path/to/test.cfg    Use custom config file, run all tests only\n", program_name);
    mpp_log("\nConfig file format:\n");
    mpp_log("  Each line should follow the CodecInfo structure format:\n");
    mpp_log("  {\"test_type\", coding_type, width, height, format, loop_count, frame_num, nthreads}\n");
    mpp_log("  Lines starting with ';' or '//' are treated as comments\n");
    mpp_log("\nSupported test_type: base, module, stress\n");
    mpp_log("Supported coding_type: MPP_VIDEO_CodingMJPEG, MPP_VIDEO_CodingAVC, MPP_VIDEO_CodingHEVC\n");
    mpp_log("Supported format: MPP_FMT_YUV420SP, MPP_FMT_YUV420P, MPP_FMT_YUV422SP, etc.\n");
}

MPP_RET set_dmc_performance_mode(void)
{
    MPP_RET ret = MPP_NOK;

    ret = system("echo performance > /sys/class/devfreq/dmc/governor");
    if (ret)
        mpp_log("Warning: Failed to set DMC governor to performance mode (ret=%d)\n", ret);
    else
        mpp_log("Successfully set DMC governor to performance mode\n");

    return ret;
}

MPP_RET restore_dmc_governor_mode(void)
{
    MPP_RET ret = MPP_NOK;

    ret = system("echo dmc_ondemand > /sys/class/devfreq/dmc/governor");
    if (ret)
        mpp_log("Warning: Failed to restore DMC governor to dmc_ondemand mode (ret=%d)\n", ret);
    else
        mpp_log("Successfully restored DMC governor to dmc_ondemand mode\n");

    return ret;
}

static char* trim_whitespace(char* str)
{
    char* end;

    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return str;

    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;

    *(end + 1) = 0;
    return str;
}

static int parse_key_value(const char* line, char* key, char* value)
{
    const char* eq = strchr(line, '=');
    if (!eq) return 0;

    size_t key_len = eq - line;
    if (key_len >= 64) return 0;

    strncpy(key, line, key_len);
    key[key_len] = '\0';
    strcpy(key, trim_whitespace(key));

    strcpy(value, eq + 1);
    strcpy(value, trim_whitespace(value));

    return 1;
}

static RK_U32 parse_coding_type(const char *str)
{
    if (!str) return MPP_VIDEO_CodingUnused;

    if (strstr(str, "MPP_VIDEO_CodingMJPEG") || strstr(str, "MJPEG"))
        return MPP_VIDEO_CodingMJPEG;
    if (strstr(str, "MPP_VIDEO_CodingAVC") || strstr(str, "AVC") || strstr(str, "H264"))
        return MPP_VIDEO_CodingAVC;
    if (strstr(str, "MPP_VIDEO_CodingHEVC") || strstr(str, "HEVC") || strstr(str, "H265"))
        return MPP_VIDEO_CodingHEVC;
    if (strstr(str, "MPP_VIDEO_CodingVP8") || strstr(str, "VP8"))
        return MPP_VIDEO_CodingVP8;
    if (strstr(str, "MPP_VIDEO_CodingVP9") || strstr(str, "VP9"))
        return MPP_VIDEO_CodingVP9;

    return MPP_VIDEO_CodingUnused;
}

static RK_U32 parse_pixel_format(const char *str)
{
    if (!str) return MPP_FMT_YUV420SP;

    if (strstr(str, "MPP_FMT_YUV420SP")) return MPP_FMT_YUV420SP;
    if (strstr(str, "MPP_FMT_YUV420P")) return MPP_FMT_YUV420P;
    if (strstr(str, "MPP_FMT_YUV422SP")) return MPP_FMT_YUV422SP;
    if (strstr(str, "MPP_FMT_YUV422P")) return MPP_FMT_YUV422P;
    if (strstr(str, "MPP_FMT_YUV422_YUYV")) return MPP_FMT_YUV422_YUYV;
    if (strstr(str, "MPP_FMT_YUV422_UYVY")) return MPP_FMT_YUV422_UYVY;
    if (strstr(str, "MPP_FMT_YUV422_YVYU")) return MPP_FMT_YUV422_YVYU;
    if (strstr(str, "MPP_FMT_YUV422_VYUY")) return MPP_FMT_YUV422_VYUY;
    if (strstr(str, "MPP_FMT_YUV420SP_VU")) return MPP_FMT_YUV420SP_VU;
    if (strstr(str, "MPP_FMT_YUV422SP_VU")) return MPP_FMT_YUV422SP_VU;
    if (strstr(str, "MPP_FMT_YUV400")) return MPP_FMT_YUV400;
    if (strstr(str, "MPP_FMT_YUV444SP")) return MPP_FMT_YUV444SP;
    if (strstr(str, "MPP_FMT_YUV444P")) return MPP_FMT_YUV444P;

    if (strstr(str, "MPP_FMT_RGB444")) return MPP_FMT_RGB444;
    if (strstr(str, "MPP_FMT_BGR444")) return MPP_FMT_BGR444;
    if (strstr(str, "MPP_FMT_RGB555")) return MPP_FMT_RGB555;
    if (strstr(str, "MPP_FMT_BGR555")) return MPP_FMT_BGR555;
    if (strstr(str, "MPP_FMT_RGB565")) return MPP_FMT_RGB565;
    if (strstr(str, "MPP_FMT_BGR565")) return MPP_FMT_BGR565;
    if (strstr(str, "MPP_FMT_RGB888")) return MPP_FMT_RGB888;
    if (strstr(str, "MPP_FMT_BGR888")) return MPP_FMT_BGR888;
    if (strstr(str, "MPP_FMT_RGB101010")) return MPP_FMT_RGB101010;
    if (strstr(str, "MPP_FMT_BGR101010")) return MPP_FMT_BGR101010;

    if (strstr(str, "MPP_FMT_ARGB8888")) return MPP_FMT_ARGB8888;
    if (strstr(str, "MPP_FMT_ABGR8888")) return MPP_FMT_ABGR8888;
    if (strstr(str, "MPP_FMT_BGRA8888")) return MPP_FMT_BGRA8888;
    if (strstr(str, "MPP_FMT_RGBA8888")) return MPP_FMT_RGBA8888;

    return MPP_FMT_YUV420SP;
}

MPP_RET mpi_enc_slt_rd_case_file(MpiEncSltCtx *ctx)
{
    if (!ctx || !ctx->config_file) {
        mpp_err("Invalid context or config file path\n");
        return MPP_ERR_NULL_PTR;
    }

    FILE *fp = fopen(ctx->config_file, "r");
    if (!fp) {
        mpp_err("Failed to open config file: %s\n", ctx->config_file);
        return MPP_ERR_OPEN_FILE;
    }

    mpp_log("Reading SLT config from: %s\n", ctx->config_file);
    mpp_log("Test suite filter: %s\n", ctx->test_suite ? ctx->test_suite : "all");

    char line[512];
    RK_U32 section_count = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim_whitespace(line);
        if (trimmed[0] == '[' && strchr(trimmed, ']')) {
            section_count++;
        }
    }

    if (section_count == 0) {
        mpp_err("No test cases found in config file\n");
        fclose(fp);
        return MPP_NOK;
    }

    ctx->test_cases = mpp_calloc(CodecInfo, section_count);
    if (!ctx->test_cases) {
        mpp_err("Failed to allocate memory for %d test cases\n", section_count);
        fclose(fp);
        return MPP_ERR_MALLOC;
    }

    rewind(fp);
    ctx->test_count = 0;
    ctx->base_test_count = 0;
    ctx->module_test_count = 0;
    ctx->stress_test_count = 0;

    CodecInfo *current_codec = NULL;
    int in_section = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim_whitespace(line);

        if (trimmed[0] == '#' || trimmed[0] == '\0') {
            continue;
        }

        if (trimmed[0] == '[' && strchr(trimmed, ']')) {
            if (current_codec && in_section) {
                if (!ctx->test_suite || !strcmp(ctx->test_suite, "all") ||
                    !strcmp(current_codec->test_type, ctx->test_suite)) {

                    generate_test_names(current_codec);
                    ctx->test_count++;
                    mpp_log("Loaded test case %d: %s\n", ctx->test_count, current_codec->test_name);
                } else {
                    if (current_codec->test_type) {
                        free((void*)current_codec->test_type);
                        current_codec->test_type = NULL;
                    }
                }
            }

            if (ctx->test_count < section_count) {
                current_codec = &ctx->test_cases[ctx->test_count];
                memset(current_codec, 0, sizeof(CodecInfo));

                current_codec->coding_type = MPP_VIDEO_CodingUnused;
                current_codec->format = MPP_FMT_YUV420SP;
                current_codec->loop_count = 1;
                current_codec->frame_num = 1;
                current_codec->nthreads = 1;

                in_section = 1;
            }
            continue;
        }

        if (in_section && current_codec) {
            char key[64], value[256];
            if (parse_key_value(trimmed, key, value)) {
                if (!strcmp(key, "test_type")) {
                    current_codec->test_type = strdup(value);
                } else if (!strcmp(key, "coding_type")) {
                    current_codec->coding_type = parse_coding_type(value);
                } else if (!strcmp(key, "width")) {
                    current_codec->width = (RK_U32)atoi(value);
                } else if (!strcmp(key, "height")) {
                    current_codec->height = (RK_U32)atoi(value);
                } else if (!strcmp(key, "format")) {
                    current_codec->format = parse_pixel_format(value);
                } else if (!strcmp(key, "loop_count")) {
                    current_codec->loop_count = (RK_U32)atoi(value);
                } else if (!strcmp(key, "frame_num")) {
                    current_codec->frame_num = (RK_U32)atoi(value);
                } else if (!strcmp(key, "nthreads")) {
                    current_codec->nthreads = (RK_U32)atoi(value);
                } else if (!strcmp(key, "input_file") && strlen(value) > 0) {
                    if (value[0] != '/') {
                        snprintf(current_codec->input_file, sizeof(current_codec->input_file),
                                 "/sdcard/%s", value);
                    } else {
                        strncpy(current_codec->input_file, value, sizeof(current_codec->input_file) - 1);
                    }
                } else if (!strcmp(key, "output_file") && strlen(value) > 0) {
                    strncpy(current_codec->output_file, value, sizeof(current_codec->output_file) - 1);
                }
            }
        }
    }

    if (current_codec && in_section) {
        if (!ctx->test_suite || !strcmp(ctx->test_suite, "all") ||
            !strcmp(current_codec->test_type, ctx->test_suite)) {

            generate_test_names(current_codec);

            if (!strcmp(current_codec->test_type, "base")) {
                ctx->base_test_count++;
            } else if (!strcmp(current_codec->test_type, "module")) {
                ctx->module_test_count++;
            } else if (!strcmp(current_codec->test_type, "stress")) {
                ctx->stress_test_count++;
            }

            ctx->test_count++;
            mpp_log("Loaded test case %d: %s\n", ctx->test_count, current_codec->test_name);
        } else {
            if (current_codec->test_type) {
                free((void*)current_codec->test_type);
                current_codec->test_type = NULL;
            }
        }
    }

    if (ctx->test_count < section_count) {
        CodecInfo *new_cases = mpp_realloc(ctx->test_cases, CodecInfo, ctx->test_count);
        if (new_cases) {
            ctx->test_cases = new_cases;
        }
    }

    fclose(fp);

    mpp_log("Successfully loaded %d test cases from config file\n", ctx->test_count);
    mpp_log("  Base tests: %d\n", ctx->base_test_count);
    mpp_log("  Module tests: %d\n", ctx->module_test_count);
    mpp_log("  Stress tests: %d\n", ctx->stress_test_count);

    return MPP_OK;
}

void generate_test_names(CodecInfo *codec)
{
    const char *file_ext;
    const char *codec_name;
    const char *fmt_name;
    const char *thread_name;

    switch (codec->coding_type) {
    case MPP_VIDEO_CodingAVC:
        file_ext = "264";
        codec_name = "H264";
        break;
    case MPP_VIDEO_CodingHEVC:
        file_ext = "265";
        codec_name = "H265";
        break;
    case MPP_VIDEO_CodingMJPEG:
        file_ext = "jpg";
        codec_name = "JPEG";
        break;
    case MPP_VIDEO_CodingVP8:
        file_ext = "webm";
        codec_name = "VP8";
        break;
    default:
        file_ext = "bin";
        codec_name = "UNKNOWN";
        break;
    }

    switch (codec->format) {
    case MPP_FMT_YUV420SP:      fmt_name = "YUV420SP";      break;
    case MPP_FMT_YUV422SP:      fmt_name = "YUV422SP";      break;
    case MPP_FMT_YUV420P:       fmt_name = "YUV420P";       break;
    case MPP_FMT_YUV420SP_VU:   fmt_name = "YUV420SPVU";    break;
    case MPP_FMT_YUV422P:       fmt_name = "YUV422P";       break;
    case MPP_FMT_YUV422SP_VU:   fmt_name = "YUV422SPVU";    break;
    case MPP_FMT_YUV422_YUYV:   fmt_name = "YUV422YUYV";    break;
    case MPP_FMT_YUV422_UYVY:   fmt_name = "YUV422UYVY";    break;
    case MPP_FMT_YUV400:        fmt_name = "YUV400";        break;
    case MPP_FMT_YUV444SP:      fmt_name = "YUV444SP";      break;
    case MPP_FMT_YUV444P:       fmt_name = "YUV444P";       break;
    case MPP_FMT_RGB565:        fmt_name = "RGB565";        break;
    case MPP_FMT_BGR565:        fmt_name = "BGR565";        break;
    case MPP_FMT_RGB555:        fmt_name = "RGB555";        break;
    case MPP_FMT_RGB444:        fmt_name = "RGB444";        break;
    case MPP_FMT_RGB101010:     fmt_name = "RGB101010";     break;
    case MPP_FMT_RGB888:        fmt_name = "RGB888";        break;
    case MPP_FMT_BGR888:        fmt_name = "BGR888";        break;
    case MPP_FMT_ARGB8888:      fmt_name = "ARGB8888";      break;
    case MPP_FMT_ABGR8888:      fmt_name = "ABGR8888";      break;
    case MPP_FMT_BGRA8888:      fmt_name = "BGRA8888";      break;
    case MPP_FMT_RGBA8888:      fmt_name = "RGBA8888";      break;
    default:                    fmt_name = "UNKNOWN";       break;
    }

    if (codec->nthreads == 1) {
        thread_name = "1t";
    } else if (codec->nthreads == 2) {
        thread_name = "2t";
    } else if (codec->nthreads == 4) {
        thread_name = "4t";
    } else if (codec->nthreads == 8) {
        thread_name = "8t";
    } else {
        static char custom_thread_name[16];
        snprintf(custom_thread_name, sizeof(custom_thread_name), "%dt", codec->nthreads);
        thread_name = custom_thread_name;
    }

    snprintf(codec->test_name, sizeof(codec->test_name),
             "%s_%dx%d_%s_%s_%s",
             codec_name,
             codec->width, codec->height,
             fmt_name,
             thread_name,
             codec->test_type);

    snprintf(codec->output_file, sizeof(codec->output_file),
             "%s/%s_%s_%dx%d_%s_%s.%s",
             "/sdcard/vepu_slt/output",
             codec->test_type,
             codec_name,
             codec->width, codec->height,
             fmt_name,
             thread_name,
             file_ext);

    snprintf(codec->verify_file, sizeof(codec->verify_file),
             "%s/%s_%s_%dx%d_%s_%s.verify",
             "/sdcard/vepu_slt/verify",
             codec->test_type,
             codec_name,
             codec->width, codec->height,
             fmt_name,
             thread_name);

    snprintf(codec->file_slt_gold, sizeof(codec->file_slt_gold),
             "%s/%s_%s_%dx%d_%s_%s.verify",
             "/sdcard/vepu_slt/gold",
             codec->test_type,
             codec_name,
             codec->width, codec->height,
             fmt_name,
             thread_name);

    if (codec->input_file[0] != '\0') {
        char full_input_path[512];
        snprintf(full_input_path, sizeof(full_input_path), "%s/%s", "/sdcard", codec->input_file);
        strncpy(codec->input_file, full_input_path, sizeof(codec->input_file) - 1);
        codec->input_file[sizeof(codec->input_file) - 1] = '\0';
    }
}
#endif /* __MPI_ENC_SLT_UTILS_H__ */