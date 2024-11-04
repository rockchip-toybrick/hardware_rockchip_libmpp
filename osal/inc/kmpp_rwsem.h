/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_RWSEM_H__
#define __KMPP_RWSEM_H__

#include "rk_type.h"

typedef struct osal_rwsemaphore_t osal_rwsemaphore;

rk_s32 osal_rwsem_init(osal_rwsemaphore **rwsem);
void   osal_rwsem_deinit(osal_rwsemaphore **rwsem);
rk_s32 osal_rwsem_size(void);
rk_s32 osal_rwsem_assign(osal_rwsemaphore **rwsem, void *buf, rk_s32 size);

void   osal_down_read(osal_rwsemaphore *rwsem);
rk_s32 osal_down_read_interruptible(osal_rwsemaphore *rwsem);
rk_s32 osal_down_read_killable(osal_rwsemaphore *rwsem);
rk_s32 osal_down_read_trylock(osal_rwsemaphore *rwsem);

void   osal_down_write(osal_rwsemaphore *rwsem);
rk_s32 osal_down_write_killable(osal_rwsemaphore *rwsem);
rk_s32 osal_down_write_trylock(osal_rwsemaphore *rwsem);

void   osal_up_read(osal_rwsemaphore *rwsem);
void   osal_up_write(osal_rwsemaphore *rwsem);

void   osal_downgrade_write(osal_rwsemaphore *rwsem);
rk_s32 osal_rwsem_is_locked(osal_rwsemaphore *rwsem);

#endif /* __KMPP_RWSEM_H__ */
