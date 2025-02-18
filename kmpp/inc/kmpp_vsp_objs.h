/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_VSP_OBJS_H__
#define __KMPP_VSP_OBJS_H__

#include "kmpi_vsp.h"

/* vsp pp init config */
#define KMPP_VSP_INIT_CFG_ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, s32, rk_s32,      chan_id) \
    ENTRY(prefix, s32, rk_s32,      max_width) \
    ENTRY(prefix, s32, rk_s32,      max_height)

#define KMPP_VSP_INIT_CFG_STRUCT_TABLE(ENTRY, prefix) \
    ENTRY(prefix, shm, KmppShmPtr,  name)

#define KMPP_OBJ_NAME           kmpp_vsp_init_cfg
#define KMPP_OBJ_INTF_TYPE      KmppVspInitCfg
#define KMPP_OBJ_ENTRY_TABLE    KMPP_VSP_INIT_CFG_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE   KMPP_VSP_INIT_CFG_STRUCT_TABLE
#include "kmpp_obj_func.h"

/* vsp pp runtime config */
#define KMPP_VSP_PP_CFG_ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, s32, rk_s32,      width) \
    ENTRY(prefix, s32, rk_s32,      height) \
    ENTRY(prefix, s32, rk_s32,      down_scale_en)

#define KMPP_VSP_PP_CFG_ENTRY_TABLE2(ENTRY, prefix) \
    ENTRY(prefix, s32, rk_s32,      md, enable) \
    ENTRY(prefix, s32, rk_s32,      md, switch_sad) \
    ENTRY(prefix, s32, rk_s32,      md, thres_sad) \
    ENTRY(prefix, s32, rk_s32,      md, thres_move) \
    ENTRY(prefix, s32, rk_s32,      md, night_mode) \
    ENTRY(prefix, s32, rk_s32,      md, filter_switch) \
    ENTRY(prefix, s32, rk_s32,      md, thres_dust_move) \
    ENTRY(prefix, s32, rk_s32,      md, thres_dust_blk) \
    ENTRY(prefix, s32, rk_s32,      md, thres_dust_chng) \
    ENTRY(prefix, s32, rk_s32,      od, enable) \
    ENTRY(prefix, s32, rk_s32,      od, is_background) \
    ENTRY(prefix, s32, rk_s32,      od, thres_complex) \
    ENTRY(prefix, s32, rk_s32,      od, thres_area_complex) \
    ENTRY(prefix, s32, rk_s32,      od, thres_complex_cnt) \
    ENTRY(prefix, s32, rk_s32,      od, thres_sad)

#define KMPP_OBJ_NAME               kmpp_vsp_pp_rt_cfg
#define KMPP_OBJ_INTF_TYPE          KmppVspRtCfg
#define KMPP_OBJ_ENTRY_TABLE        KMPP_VSP_PP_CFG_ENTRY_TABLE
#define KMPP_OBJ_ENTRY_TABLE2       KMPP_VSP_PP_CFG_ENTRY_TABLE2
#include "kmpp_obj_func.h"

/* vsp pp md / od output data */
#define KMPP_VSP_PP_OUT_ENTRY_TABLE2(ENTRY, prefix) \
    ENTRY(prefix, s32, rk_s32,      od, flag) \
    ENTRY(prefix, s32, rk_s32,      od, pix_sum)

#define KMPP_OBJ_NAME               kmpp_vsp_pp_rt_out
#define KMPP_OBJ_INTF_TYPE          KmppVspRtOut
#define KMPP_OBJ_ENTRY_TABLE2       KMPP_VSP_PP_OUT_ENTRY_TABLE2
#include "kmpp_obj_func.h"

#endif /*__KMPP_VSP_OBJS_H__*/
