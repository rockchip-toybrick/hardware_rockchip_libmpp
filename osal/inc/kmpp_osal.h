/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OSAL_H__
#define __KMPP_OSAL_H__

#include "rk_list.h"
#include "kmpp_macro.h"

#include "kmpp_atomic.h"
#include "kmpp_completion.h"
#include "kmpp_dmabuf.h"
#include "kmpp_dmaheap.h"
#include "kmpp_bitops.h"
#include "kmpp_clk.h"
#include "kmpp_cls.h"
#include "kmpp_dev.h"
#include "kmpp_env.h"
#include "kmpp_file.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_mutex.h"
#include "kmpp_pm.h"
#include "kmpp_sema.h"
#include "kmpp_rwsem.h"
#include "kmpp_spinlock.h"
#include "kmpp_thread.h"
#include "kmpp_sthd.h"
#include "kmpp_string.h"
#include "kmpp_time.h"
#include "kmpp_wait.h"
#include "kmpp_delay.h"
#include "kmpp_uaccess.h"

int osal_init(void);
void osal_exit(void);

rk_s32 mpp_func_arg_init(void **mpp_args, const char *name);
rk_s32 mpp_func_arg_deinit(void *mpp_args);
rk_s32 mpp_func_arg_run(void *mpp_args);

rk_s32 mpp_func_arg_set_u32(void *mpp_args, const char *name, rk_u32 val);
rk_s32 mpp_func_arg_set_s32(void *mpp_args, const char *name, rk_s32 val);
rk_s32 mpp_func_arg_set_u64(void *mpp_args, const char *name, rk_u64 val);
rk_s32 mpp_func_arg_set_ptr(void *mpp_args, const char *name, void *val);

rk_s32 mpp_func_arg_get_u32(void *mpp_args, const char *name, rk_u32 *val);
rk_s32 mpp_func_arg_get_s32(void *mpp_args, const char *name, rk_s32 *val);
rk_s32 mpp_func_arg_get_u64(void *mpp_args, const char *name, rk_u64 *val);
rk_s32 mpp_func_arg_get_ptr(void *mpp_args, const char *name, void **val);

#endif /* __KMPP_OSAL_H__ */