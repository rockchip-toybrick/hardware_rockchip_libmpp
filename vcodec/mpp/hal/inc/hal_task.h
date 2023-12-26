// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#ifndef __HAL_TASK__
#define __HAL_TASK__

#include "hal_enc_task.h"

typedef struct HalTask_u {
	HalTaskHnd hnd;
	union {
		HalEncTask enc;
	};
} HalTaskInfo;

#endif /*__HAL_TASK__*/
