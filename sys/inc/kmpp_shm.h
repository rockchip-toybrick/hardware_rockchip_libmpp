/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SHM_H__
#define __KMPP_SHM_H__

#include "kmpp_cls.h"

#include "kmpp_sys_defs.h"

typedef struct KmppShmGrpCfg_t {
    KmppShmMgr mgr;
    osal_fs_dev *file;
    void *buf;
    rk_s32 size;
} KmppShmGrpCfg;

/* global init / exit funciton on module init / exit */
rk_s32 kmpp_shm_init(void);
rk_s32 kmpp_shm_deinit(void);
KmppShmMgr *kmpp_shm_get_objs_mgr(void);
rk_s32 kmpp_shm_entry_offset(void);

/* share memory manager bind all kmpp object definitions to /dev/kmpp_objs: */
rk_s32 kmpp_shm_mgr_get(KmppShmMgr *mgr, const rk_u8 *name);
/* kmpp object share memory manager add KmppObjDef binding */
rk_s32 kmpp_shm_mgr_bind_objdef(KmppShmMgr mgr, KmppObjDef def);
rk_s32 kmpp_shm_mgr_unbind_objdef(KmppShmMgr mgr, KmppObjDef def);

rk_s32 kmpp_shm_mgr_put(KmppShmMgr mgr);
rk_s32 kmpp_shm_mgr_query(KmppShmMgr *mgr, const rk_u8 *name);

/* manager create group for each allocator file handle */
rk_s32 kmpp_shm_grp_size(void);
rk_s32 kmpp_shm_grp_get(KmppShmGrp *grp, KmppShmGrpCfg *cfg);
rk_s32 kmpp_shm_grp_put(KmppShmGrp grp);
rk_s32 kmpp_shm_grp_check(KmppShmGrp grp, KmppShmMgr mgr);

/* KmppShmGrp grp is NULL for pure kernel allocation */
rk_s32 kmpp_shm_get(KmppShm *shm, KmppShmGrp grp, KmppObj obj);
rk_s32 kmpp_shm_put(KmppShm shm);

void *kmpp_shm_get_kbase(KmppShm shm);
void *kmpp_shm_get_kaddr(KmppShm shm);
rk_ul kmpp_shm_get_ubase(KmppShm shm);
rk_u64 kmpp_shm_get_uaddr(KmppShm shm);

#endif /* __KMPP_SHM_H__ */