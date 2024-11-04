/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SEMA_H__
#define __KMPP_SEMA_H__

#include "rk_type.h"

typedef struct osal_semaphore_t osal_semaphore;

rk_s32 osal_sema_init(osal_semaphore **sem, rk_s32 val);
void   osal_sema_deinit(osal_semaphore **sem);

rk_s32 osal_sema_size(void);
rk_s32 osal_sema_assign(osal_semaphore **sem, rk_s32 val, void *buf, rk_s32 size);

rk_s32 osal_down(osal_semaphore *sem);
rk_s32 osal_down_interruptible(osal_semaphore *sem);
rk_s32 osal_down_trylock(osal_semaphore *sem);
void   osal_up(osal_semaphore *sem);

#endif /* __KMPP_SEMA_H__ */
