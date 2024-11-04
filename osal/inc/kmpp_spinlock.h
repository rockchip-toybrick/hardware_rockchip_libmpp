/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SPINLOCK_H__
#define __KMPP_SPINLOCK_H__

#include "rk_type.h"

typedef struct osal_spinlock_t osal_spinlock;

rk_s32 osal_spinlock_init(osal_spinlock **lock);
void   osal_spinlock_deinit(osal_spinlock **lock);

rk_s32 osal_spinlock_size(void);
rk_s32 osal_spinlock_assign(osal_spinlock **lock, void *buf, rk_s32 size);

void   osal_spin_lock(osal_spinlock *lock);
rk_s32 osal_spin_trylock(osal_spinlock *lock);
void   osal_spin_unlock(osal_spinlock *lock);

void   osal_spin_lock_bh(osal_spinlock *lock);
rk_s32 osal_spin_trylock_bh(osal_spinlock *lock);
void   osal_spin_unlock_bh(osal_spinlock *lock);

void   osal_spin_lock_irqsave(osal_spinlock *lock, rk_ul *flags);
rk_s32 osal_spin_trylock_irqsave(osal_spinlock *lock, rk_ul *flags);
void   osal_spin_unlock_irqrestore(osal_spinlock *lock, rk_ul *flags);

#endif /* __KMPP_SPINLOCK_H__ */
