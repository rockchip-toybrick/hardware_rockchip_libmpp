/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_ATOMIC_H__
#define __KMPP_ATOMIC_H__

#include "rk_type.h"

#define KMPP_FETCH_ADD          __sync_fetch_and_add
#define KMPP_FETCH_SUB          __sync_fetch_and_sub
#define KMPP_FETCH_OR           __sync_fetch_and_or
#define KMPP_FETCH_AND          __sync_fetch_and_and
#define KMPP_FETCH_XOR          __sync_fetch_and_xor
#define KMPP_FETCH_NAND         __sync_fetch_and_nand

#define KMPP_ADD_FETCH          __sync_add_and_fetch
#define KMPP_SUB_FETCH          __sync_sub_and_fetch
#define KMPP_OR_FETCH           __sync_or_and_fetch
#define KMPP_AND_FETCH          __sync_and_and_fetch
#define KMPP_XOR_FETCH          __sync_xor_and_fetch
#define KMPP_NAND_FETCH         __sync_nand_and_fetch

#define KMPP_BOOL_CAS           __sync_bool_compare_and_swap
#define KMPP_VAL_CAS            __sync_val_compare_and_swap

#define KMPP_SYNC               __sync_synchronize
#define KMPP_SYNC_TEST_SET      __sync_lock_test_and_set
#define KMPP_SYNC_CLR           __sync_lock_release

typedef struct osal_atomic_t osal_atomic;

rk_s32 osal_atomic_size(void);
rk_s32 osal_atomic_init(osal_atomic **atomic);
rk_s32 osal_atomic_assign(osal_atomic **atomic, void *buf, rk_s32 size);
void   osal_atomic_deinit(osal_atomic **atomic);

rk_s32 osal_atomic_read(osal_atomic *v);
void   osal_atomic_set(osal_atomic *v, rk_s32 i);

rk_s32 osal_atomic_fetch_inc(osal_atomic *v);
rk_s32 osal_atomic_fetch_dec(osal_atomic *v);
rk_s32 osal_atomic_inc_return(osal_atomic *v);
rk_s32 osal_atomic_dec_return(osal_atomic *v);
rk_s32 osal_atomic_dec_and_test(osal_atomic *v);

rk_s32 osal_atomic_xchg(osal_atomic *v, rk_s32 i);

#define osal_cmpxchg(ptr, oldval, newval) cmpxchg(ptr, oldval, newval)

#define osal_force_cmpxchg(ptr, oldval, newval) \
({ \
        typeof(*ptr) __val; \
        while ((__val = cmpxchg(ptr, oldval, newval)) && (*ptr != newval)); \
        __val; \
})

#endif /* __KMPP_ATOMIC_H__ */
