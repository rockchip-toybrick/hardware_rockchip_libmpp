/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_UACCESSS_H__
#define __KMPP_UACCESSS_H__

#include "rk_type.h"

rk_ul osal_copy_from_user(void *dst, const void *src, rk_ul size);
rk_ul osal_copy_to_user(void *dst, const void *src, rk_ul size);

#endif /* __KMPP_UACCESSS_H__ */
