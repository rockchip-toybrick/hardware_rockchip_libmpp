/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SYS_DEFS_H__
#define __KMPP_SYS_DEFS_H__

#include "rk_type.h"

/*
 * kmpp system definitions including the kernel space accessor and
 * userspace accessor helper.
 */

/* KmppObjDef - kmpp object name size and access table trie definition */
typedef void* KmppObjDef;
/* KmppObj - kmpp object for string name access and function access */
typedef void* KmppObj;

/* KmppShmMgr - fix size share memory manager for name and size */
typedef void* KmppShmMgr;
/* KmppShmGrp - share memory device for allocation and free */
typedef void* KmppShmGrp;
/* KmppShm - share memory for address access and read / write */
typedef void* KmppShm;

int sys_init(void);
void sys_exit(void);

#endif /* __KMPP_SYS_DEFS_H__ */