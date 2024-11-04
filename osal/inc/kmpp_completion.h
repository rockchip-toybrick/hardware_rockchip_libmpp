/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_COMPLETION_H__
#define __KMPP_COMPLETION_H__

#include "rk_type.h"

typedef struct osal_completion_t osal_completion;

void    osal_completion_init(osal_completion **completion);
void    osal_completion_deinit(osal_completion **completion);

rk_s32  osal_completion_size(void);
rk_s32  osal_completion_assign(osal_completion **completion, void *buf, rk_s32 size);

void    osal_completion_reinit(osal_completion *completion);
void    osal_wait_for_completion(osal_completion *completion);
rk_s32  osal_wait_for_completion_interruptible(osal_completion *completion);
rk_s32  osal_wait_for_completion_timeout(osal_completion *completion, rk_s32 timeout);

rk_s32  osal_try_wait_for_completion(osal_completion *completion);
rk_s32  osal_complete_done(osal_completion *completion);
void    osal_complete(osal_completion *completion);
void    osal_complete_all(osal_completion *completion);

#endif /* __KMPP_COMPLETION_H__ */