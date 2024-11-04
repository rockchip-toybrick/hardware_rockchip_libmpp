/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_MUTEX_H__
#define __KMPP_MUTEX_H__

#include "rk_type.h"

typedef struct osal_mutex_t osal_mutex;

rk_s32 osal_mutex_init(osal_mutex **mutex);
void osal_mutex_deinit(osal_mutex **mutex);

rk_s32 osal_mutex_size(void);
rk_s32 osal_mutex_assign(osal_mutex **mutex, void *buf, rk_s32 size);

rk_s32 osal_mutex_lock(osal_mutex *mutex);
rk_s32 osal_mutex_lock_interruptible(osal_mutex *mutex);
rk_s32 osal_mutex_trylock(osal_mutex *mutex);
void osal_mutex_unlock(osal_mutex *mutex);

#endif /* __KMPP_MUTEX_H__ */
