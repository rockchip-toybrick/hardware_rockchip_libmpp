/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_CLK_IMPL_H__
#define __KMPP_CLK_IMPL_H__

#include "kmpp_dev.h"
#include "kmpp_clk.h"

typedef struct kmpp_clk_info_t {
    const char *name;
    void *clk;
    rk_u32 rate_tbl[CLK_MODE_BUTT];
    rk_u32 rate_real[CLK_MODE_BUTT];
} kmpp_clk_info;

typedef struct kmpp_clk_t {
    rk_s32 clk_cnt;
    kmpp_clk_info clks[];
} kmpp_clk;

rk_s32 osal_clk_probe(osal_dev *dev);
rk_s32 osal_clk_release(osal_dev *dev);

#endif /* __KMPP_CLK_IMPL_H__ */