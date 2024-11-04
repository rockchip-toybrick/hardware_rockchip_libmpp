/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_TIME_H__
#define __KMPP_TIME_H__

#include "rk_type.h"

rk_s64 kmpp_time(void);
void kmpp_time_diff(rk_s64 start, rk_s64 end, rk_s64 limit, const char *fmt);

#endif /* __KMPP_TIME_H__ */