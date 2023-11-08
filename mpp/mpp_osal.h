/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd
 *
 */

#ifndef __ROCKCHIP_MPP_OSAL_H__
#define __ROCKCHIP_MPP_OSAL_H__

#include <linux/platform_device.h>
#include <linux/pm_wakeup.h>

#ifdef CONFIG_ROCKCHIP_MPP_OSAL
struct device_node *mpp_dev_of_node(struct device *dev);
void mpp_pm_relax(struct device *dev);
void mpp_pm_stay_awake(struct device *dev);
int mpp_device_init_wakeup(struct device *dev, bool enable);

#else
static inline struct device_node *mpp_dev_of_node(struct device *dev)
{
	return dev_of_node(dev);
}

static inline void mpp_pm_relax(struct device *dev)
{
	return pm_relax(dev);
}

static inline void mpp_pm_stay_awake(struct device *dev)
{
	return pm_stay_awake(dev);
}

static inline int mpp_device_init_wakeup(struct device *dev, bool enable)
{
	return device_init_wakeup(dev, enable);
}
#endif

#endif
