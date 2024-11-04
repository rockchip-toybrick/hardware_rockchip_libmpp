/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_WAIT_H__
#define __KMPP_WAIT_H__

#include "rk_type.h"

typedef rk_s32 (*osal_wait_cond_func)(const void *param);

typedef struct osal_wait_t osal_wait;

rk_s32 osal_wait_init(osal_wait **wait);
void   osal_wait_deinit(osal_wait **wait);

rk_s32 osal_wait_size(void);
rk_s32 osal_wait_assign(osal_wait **wait, void *buf, rk_s32 size);

rk_s32 osal_wait_interruptible(osal_wait *wait, osal_wait_cond_func func, void *param);
rk_s32 osal_wait_uninterruptible(osal_wait *wait, osal_wait_cond_func func, void *param);
rk_s32 osal_wait_timeout_interruptible(osal_wait *wait, osal_wait_cond_func func, void *param, rk_u32 ms);
rk_s32 osal_wait_timeout_uninterruptible(osal_wait *wait, osal_wait_cond_func func, void *param, rk_u32 ms);

void   osal_wait_wakeup(osal_wait *wait);

#endif /* __KMPP_WAIT_H__ */
