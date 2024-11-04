/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_dev_pm"

#include <linux/pm_runtime.h>

#include "kmpp_dev_impl.h"

void osal_pm_stay_awake(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_stay_awake(impl->device);
}
EXPORT_SYMBOL(osal_pm_stay_awake);

void osal_pm_relax(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_relax(impl->device);
}
EXPORT_SYMBOL(osal_pm_relax);

rk_s32 osal_device_init_wakeup(osal_dev *dev, rk_bool enable)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return device_init_wakeup(impl->device, enable);
}
EXPORT_SYMBOL(osal_device_init_wakeup);

void osal_pm_rt_enable(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_runtime_enable(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_enable);

void osal_pm_rt_disable(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_runtime_disable(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_disable);

rk_s32 osal_pm_rt_get_sync(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_get_sync(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_get_sync);

rk_s32 osal_pm_rt_get_if_in_use(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_get_if_in_use(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_get_if_in_use);

rk_s32 osal_pm_rt_put(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_put(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_put);

rk_s32 osal_pm_rt_put_sync(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_put_sync(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_put_sync);

void osal_pm_rt_set_autosuspend_delay(osal_dev *dev, rk_s32 us)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_runtime_set_autosuspend_delay(impl->device, us);
}
EXPORT_SYMBOL(osal_pm_rt_set_autosuspend_delay);

void osal_pm_rt_use_autosuspend(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_runtime_use_autosuspend(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_use_autosuspend);

void osal_pm_rt_mark_last_busy(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    pm_runtime_mark_last_busy(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_mark_last_busy);

rk_s32 osal_pm_rt_put_autosuspend(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_put_autosuspend(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_put_autosuspend);

rk_s32 osal_pm_rt_put_sync_suspend(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_put_sync_suspend(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_put_sync_suspend);

rk_s32 osal_pm_rt_force_suspend(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_force_suspend(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_force_suspend);

rk_s32 osal_pm_rt_force_resume(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return pm_runtime_force_resume(impl->device);
}
EXPORT_SYMBOL(osal_pm_rt_force_resume);