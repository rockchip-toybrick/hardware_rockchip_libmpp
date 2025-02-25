/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_MEM_POOL_IMPL_H__
#define __KMPP_MEM_POOL_IMPL_H__

#include "rk_list.h"

#include "kmpp_mem_pool.h"

typedef struct MppMemPoolNode_t {
    void                *check;
    /* list to used and unused in KmppMemPoolImpl */
    osal_list_head      list;
    void                *ptr;
    rk_s32              size;
    rk_u32              id;
} KmppMemPoolNode;

typedef struct MppMemPoolImpl_t {
    void                *check;
    /* list to list_pool in KmppMemPoolSrv */
    osal_list_head      list_srv;
    /* list for list in KmppMemPoolNode */
    osal_list_head      used;
    /* list for list in KmppMemPoolNode */
    osal_list_head      unused;
    rk_s32              used_count;
    rk_s32              unused_count;
    rk_u32              max_cnt;
    rk_s32              pool_id;
    rk_s32              node_id;
    rk_s32              size;
    const rk_u8         name[0];
} KmppMemPoolImpl;

void kmpp_mem_pool_init(void);
void kmpp_mem_pool_deinit(void);

#endif /*__KMPP_MEM_POOL_IMPL_H__*/
