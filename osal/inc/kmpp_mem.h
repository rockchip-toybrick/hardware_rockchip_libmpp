/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_MEM_H__
#define __KMPP_MEM_H__

#include "rk_type.h"

#define osal_gfp_normal                             0
#define osal_gfp_atomic                             1

void osal_mem_init(void);
void osal_mem_deinit(void);

#define osal_kmalloc(size, gfp)                     osal_kmalloc_f(__func__, size, gfp)
#define osal_kcalloc(size, gfp)                     osal_kcalloc_f(__func__, size, gfp)
#define osal_krealloc(ptr, oldsize, newsize, gfp)   osal_krealloc_f(__func__, ptr, oldsize, newsize, gfp)
#define osal_kfree(ptr)                             osal_kfree_f(__func__, ptr)

#define kmpp_malloc(size)                           osal_kmalloc(size, osal_gfp_normal)
#define kmpp_calloc(size)                           osal_kcalloc(size, osal_gfp_normal)
#define kmpp_realloc(ptr, oldsize, newsize)         osal_krealloc(ptr, oldsize, newsize, osal_gfp_normal)

#define kmpp_malloc_atomic(size)                    osal_kmalloc(size, osal_gfp_atomic)
#define kmpp_calloc_atomic(size)                    osal_kcalloc(size, osal_gfp_atomic)
#define kmpp_realloc_atomic(ptr, oldsize, newsize)  osal_krealloc(ptr, oldsize, newsize, osal_gfp_atomic)

#define kmpp_free(ptr)                              do { if(ptr) osal_kfree(ptr); ptr = NULL; } while (0)

/* memory for userspace */
#define osal_malloc_share(size)                     osal_malloc_share_f(__func__, size)
#define osal_kfree_share(ptr)                       osal_kfree_share_f(__func__, ptr)

#define kmpp_malloc_share(size)                     osal_malloc_share(size)
#define kmpp_free_share(ptr)                        do { if(ptr) osal_kfree_share(ptr); ptr = NULL; } while (0)

void *osal_kmalloc_f(const char *func, rk_u32 size, rk_u32 gfp);
void *osal_kcalloc_f(const char *func, rk_u32 size, rk_u32 gfp);
void *osal_krealloc_f(const char *func, void *ptr, rk_u32 oldsize, rk_u32 newsize, rk_u32 gfp);
void osal_kfree_f(const char *func, void *ptr);

/* share alloc do not set zero and do not have debug segment */
void *osal_malloc_share_f(const char *func, rk_u32 size);
void osal_kfree_share_f(const char *func, void *ptr);

#endif /* __KMPP_MEM_H__ */
