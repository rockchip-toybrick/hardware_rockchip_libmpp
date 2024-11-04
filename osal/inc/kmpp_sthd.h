/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_STHD_H__
#define __KMPP_STHD_H__

#include "rk_type.h"

typedef rk_s32 (*kmpp_func)(void *);

/*
 * self for thread group pointer
 * idx -1 for thread group
 * idx not negtive for thread in thread group
 * ctx for thread context for each thread
 */
typedef struct kmpp_thds_t {
    void        *self;
    void        *ctx;
    rk_s32      idx;
} kmpp_thds;

/* multi-thread group with same callback and context */
rk_s32 kmpp_thds_init(kmpp_thds **thds, const char *name, RK_S32 count);
void kmpp_thds_deinit(kmpp_thds **thds);

void kmpp_thds_setup(kmpp_thds *thds, kmpp_func func, void *ctx);
kmpp_thds *kmpp_thds_get_entry(kmpp_thds *thds, rk_s32 idx);
const char *kmpp_thds_get_name(kmpp_thds *thds);

/* single thread function */
void kmpp_thds_lock(kmpp_thds *thds);
void kmpp_thds_unlock(kmpp_thds *thds);
rk_s32 kmpp_thds_run_nl(kmpp_thds *thds);

#endif /* __KMPP_STHD_H__ */