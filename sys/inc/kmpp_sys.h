/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SYS_H__
#define __KMPP_SYS_H__

#include "rk_type.h"

#include "kmpp_obj.h"
#include "kmpp_shm.h"
#include "kmpp_frame.h"

rk_s32 sys_init(void);
void sys_exit(void);

#endif /* __KMPP_SYS_H__ */
