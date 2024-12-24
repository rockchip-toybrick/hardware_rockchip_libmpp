/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_allocator"
#include <linux/string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_maths.h"
#include "mpp_allocator_api.h"
#include "mpp_allocator.h"
#include "allocator_mpibuf.h"
#include "allocator_dmaheap.h"
#include "allocator_rkdmaheap.h"

typedef enum MppAllocatorType_e {
        MPP_ALLOCATOR_DMABUF,
        MPP_ALLOCATOR_RK_DMABUF,
        MPP_ALLOCATOR_MPI_BUF,
        MPP_ALLOCATOR_BUTT,
} MppAllocatorType;

typedef struct AllocatorInfo_t {
	MppAllocatorType type;
	RK_U32 valid;
	mpp_allocator_api *api;
} AllocatorInfo;

static AllocatorInfo infos[MPP_ALLOCATOR_BUTT];
static mpp_allocator_api *apis[] = {
	&dma_heap_allocator,
	&rk_dma_heap_allocator,
	&mpi_buf_allocator,
};

static mpp_allocator_api *get_allocator(MppBufferType type)
{
	RK_U32 i;
	mpp_allocator_api *api = NULL;
	MppAllocatorType allocator_type = (MppAllocatorType)type;

	if (allocator_type >= MPP_ALLOCATOR_BUTT) {
		mpp_err_f("invalid allocator type %d\n", type);
		return NULL;
	}

	if (!infos[allocator_type].valid) {
		MPP_RET ret = MPP_OK;

		ret = apis[allocator_type]->init(__func__);
		if (!ret) {
			infos[allocator_type].type = allocator_type;
			infos[allocator_type].api = apis[allocator_type];
			infos[allocator_type].valid = 1;
			goto __return;
		}
		for (i = 0; i < MPP_ARRAY_ELEMS(infos); i++) {
			if (infos[i].valid && infos[i].api) {
				allocator_type = infos[i].type;
				break;
			}
		}
	}

__return:
	api = infos[allocator_type].api;

	return api;
}

MPP_RET mpp_allocator_init(const char* caller)
{
	RK_U32 i;
	MPP_RET ret = MPP_OK;

	memset(&infos, 0, sizeof(infos));
	for (i = 0; i < MPP_ARRAY_ELEMS(apis); i++) {
		if (apis[i] && apis[i]->init) {
			ret = apis[i]->init(caller);
			if (ret)
				continue;
			infos[i].type = i;
			infos[i].api = apis[i];
			infos[i].valid = 1;
		}
	}

	return MPP_NOK;
}

void mpp_allocator_deinit(const char *caller)
{
	RK_U32 i;

	for (i = 0; i < MPP_ARRAY_ELEMS(infos); i++) {
		if (infos[i].api && infos[i].api->deinit) {
			infos[i].api->deinit(caller);
			infos[i].type = i;
			infos[i].api = NULL;
			infos[i].valid = 0;
		}
	}
}

MPP_RET mpp_allocator_alloc(MppBufferInfo *info, const char *caller)
{
	mpp_allocator_api *api = get_allocator(info->type);

	if (!api || !api->alloc)
		return MPP_NOK;

	return api->alloc(info, caller);
}

MPP_RET mpp_allocator_free(MppBufferInfo *info, const char *caller)
{
	mpp_allocator_api *api = get_allocator(info->type);

	if (!api || !api->free)
		return MPP_NOK;

	return api->free(info, caller);
}

MPP_RET mpp_allocator_import(MppBufferInfo *info, const char *caller)
{
	mpp_allocator_api *api = get_allocator(info->type);

	if (!api || !api->import)
		return MPP_NOK;

	return api->import(info, caller);
}

MPP_RET mpp_allocator_mmap(MppBufferInfo *info, const char *caller)
{
	mpp_allocator_api *api = get_allocator(info->type);

	if (!api || !api->mmap)
		return MPP_NOK;

	return api->mmap(info, caller);
}
