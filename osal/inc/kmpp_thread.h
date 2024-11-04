/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_THREAD_H__
#define __KMPP_THREAD_H__

#include "rk_type.h"

typedef rk_s32 (*osal_work_func)(void *data);

typedef struct osal_work_t {
    osal_work_func func;
    void *work;
    void *data;
    rk_s32 ret;
    rk_s32 delay_ms;
} osal_work;

typedef struct osal_worker_t {
    void *worker;
} osal_worker;

void   osal_worker_init(osal_worker **worker, const char *name);
void   osal_worker_deinit(osal_worker **worker);

void   osal_worker_flush(osal_worker *worker);

rk_s32 osal_work_init(osal_work **work, osal_work_func func, void *data);
void   osal_work_deinit(osal_work **work);
rk_s32 osal_work_queue(osal_worker *worker, osal_work *work);

rk_s32 osal_delayed_work_init(osal_work **work, osal_work_func func, void *data);
void   osal_delayed_work_deinit(osal_work **work);
rk_s32 osal_delayed_work_queue(osal_worker *worker, osal_work *work, rk_s32 delay_ms);


#endif /* __KMPP_THREAD_H__ */