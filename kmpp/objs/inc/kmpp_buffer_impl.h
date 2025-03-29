/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_BUFFER_IMPL_H__
#define __KMPP_BUFFER_IMPL_H__

#include "rk_list.h"
#include "kmpp_spinlock.h"

#include "kmpp_buffer.h"
#include "kmpp_dmaheap.h"

/* buffer group share name storage for both name and allocator */
#define BUF_GRP_STR_BUF_SIZE    (64)

#define BUF_ST_NONE             (0)
#define BUF_ST_INIT             (1)
#define BUF_ST_INIT_TO_USED     (2)
#define BUF_ST_USED             (3)
#define BUF_ST_UNUSED           (4)
#define BUF_ST_USED_TO_DEINIT   (5)
#define BUF_ST_DEINIT_AT_GRP    (6)
#define BUF_ST_DEINIT_AT_SRV    (7)

typedef struct KmppBufGrpCfgImpl_t {
    rk_u32              flag;
    rk_u32              count;
    rk_u32              size;
    KmppBufferMode      mode;
    rk_s32              fd;
    rk_s32              grp_id;
    rk_s32              used;
    rk_s32              unused;
    void                *device;
    KmppShmPtr          allocator;
    KmppShmPtr          name;
    void                *grp_impl;
} KmppBufGrpCfgImpl;

typedef struct KmppBufGrpImpl_t {
    /* group share config pointer */
    KmppShmPtr          cfg;
    KmppBufGrpCfg       cfg_ext;
    KmppBufGrpCfgImpl   *cfg_usr;
    KmppObj             obj;

    /* internal parameter set on used */
    rk_u32              flag;
    rk_u32              count;
    rk_u32              size;
    KmppBufferMode      mode;

    osal_spinlock       *lock;
    osal_list_head      list_srv;
    osal_list_head      list_used;
    osal_list_head      list_unused;
    rk_s32              count_used;
    rk_s32              count_unused;

    /* allocator */
    KmppDmaHeap         heap;
    rk_s32              heap_fd;
    rk_s32              grp_id;
    rk_s32              buf_id;
    rk_s32              buf_cnt;
    rk_s32              buffer_count;

    /* status status */
    rk_s32              log_runtime_en;
    rk_s32              log_history_en;
    rk_s32              clear_on_exit;
    rk_s32              dump_on_exit;
    rk_s32              is_orphan;
    rk_s32              is_finalizing;
    rk_s32              is_default;

    /* string storage */
    rk_s32              name_offset;
    rk_u8               str_buf[BUF_GRP_STR_BUF_SIZE];

    /* buffer group config for internal usage */
    KmppBufGrpCfgImpl   cfg_int;
} KmppBufGrpImpl;

typedef struct KmppBufCfgImpl_t {
    rk_u32              size;
    rk_u32              offset;
    rk_u32              flag;
    rk_s32              fd;
    rk_s32              index;      /* index for external user buffer match */
    rk_s32              grp_id;
    rk_s32              buf_gid;
    rk_s32              buf_uid;

    void                *khnd;
    void                *kdmabuf;
    void                *kdev;
    osal_dev            *dev;
    rk_u64              iova;
    void                *kptr;
    void                *kpriv;
    void                *kfp;
    rk_u64              uptr;
    rk_u64              upriv;
    rk_u64              ufp;
    KmppShmPtr          sptr;
    KmppShmPtr          group;
    void                *buf_impl;
} KmppBufCfgImpl;

typedef struct KmppBufferImpl_t {
    KmppShmPtr          cfg;
    KmppBufCfg          cfg_ext;
    KmppBufCfgImpl      *cfg_usr;

    KmppBufGrpImpl      *grp;
    /* when grp is valid used grp lock else use srv lock */
    osal_spinlock       *lock;
    KmppObj             obj;

    KmppDmaBuf          buf;
    void                *kptr;
    rk_u64              uaddr;
    rk_u32              size;

    rk_s32              grp_id;
    rk_s32              buf_gid;
    rk_s32              buf_uid;
    rk_s32              ref_cnt;

    rk_u32              status;
    rk_u32              discard;
    osal_list_head      list_status;
    osal_list_head      list_maps;
    KmppBufCfgImpl      cfg_int;
} KmppBufferImpl;

rk_s32 kmpp_buf_grp_get_size(KmppBufGrpImpl *grp);
rk_s32 kmpp_buf_grp_get_count(KmppBufGrpImpl *grp);

rk_s32 kmpp_buf_init(void);
rk_s32 kmpp_buf_deinit(void);

#endif /* __KMPP_BUFFER_IMPL_H__ */
