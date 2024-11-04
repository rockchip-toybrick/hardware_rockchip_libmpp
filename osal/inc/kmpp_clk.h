/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */
#ifndef __KMPP_CLK_H__
#define __KMPP_CLK_H__

#include "kmpp_dev.h"

typedef enum osal_clk_mode_e {
    CLK_MODE_BASE       = 0,
    CLK_MODE_USER       = CLK_MODE_BASE,    /* user defined frequency */
    CLK_MODE_DEFAULT,                       /* dts default frequency on init */
    CLK_MODE_LOW,                           /* dts low frequency for reset or low-power */
    CLK_MODE_NORMAL,                        /* dts normal frequency */
    CLK_MODE_HIGH,                          /* dts high frequency for full speed */
    CLK_MODE_BUTT,
} osal_clk_mode;

typedef struct osal_clk_rate_tbl_t {
    rk_u32 rate_hz[CLK_MODE_BUTT];
} osal_clk_rate_tbl;

rk_s32 osal_clk_get_index(osal_dev *dev, const char *name);
rk_s32 osal_clk_set_rate_tbl(osal_dev *dev, rk_s32 index, osal_clk_rate_tbl *tbl);
rk_s32 osal_clk_update_rate_mode(osal_dev *dev, rk_s32 index, osal_clk_mode mode, rk_u32 rate);

rk_s32 osal_clk_get_rate(osal_dev *dev, rk_s32 index, osal_clk_mode mode);
rk_s32 osal_clk_set_rate(osal_dev *dev, rk_s32 index, osal_clk_mode mode);

rk_s32 osal_clk_on(osal_dev *dev, rk_s32 index);
rk_s32 osal_clk_off(osal_dev *dev, rk_s32 index);
rk_s32 osal_clk_on_all(osal_dev *dev);
rk_s32 osal_clk_off_all(osal_dev *dev);

#endif /* __KMPP_CLK_H__ */