/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_PM_H__
#define __KMPP_PM_H__

#include "kmpp_dev.h"

void osal_pm_stay_awake(osal_dev *dev);
void osal_pm_relax(osal_dev *dev);
rk_s32 osal_device_init_wakeup(osal_dev *dev, rk_bool enable);

void osal_pm_rt_enable(osal_dev *dev);
void osal_pm_rt_disable(osal_dev *dev);
rk_s32 osal_pm_rt_get_sync(osal_dev *dev);
rk_s32 osal_pm_rt_get_if_in_use(osal_dev *dev);
rk_s32 osal_pm_rt_put(osal_dev *dev);
rk_s32 osal_pm_rt_put_sync(osal_dev *dev);
void osal_pm_rt_set_autosuspend_delay(osal_dev *dev, rk_s32 us);
void osal_pm_rt_use_autosuspend(osal_dev *dev);
void osal_pm_rt_mark_last_busy(osal_dev *dev);
rk_s32 osal_pm_rt_put_autosuspend(osal_dev *dev);
rk_s32 osal_pm_rt_put_sync_suspend(osal_dev *dev);
rk_s32 osal_pm_rt_force_suspend(osal_dev *dev);
rk_s32 osal_pm_rt_force_resume(osal_dev *dev);

#endif /* __KMPP_PM_H__ */