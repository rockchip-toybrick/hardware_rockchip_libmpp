/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SYS_DEFS_H__
#define __KMPP_SYS_DEFS_H__

#include "rk_type.h"

typedef enum EntryType_e {
    ENTRY_TYPE_s32,
    ENTRY_TYPE_u32,
    ENTRY_TYPE_s64,
    ENTRY_TYPE_u64,
    ENTRY_TYPE_ptr,
    ENTRY_TYPE_fp,      /* function poineter */
    ENTRY_TYPE_st,
    ENTRY_TYPE_shm,     /* share memory between kernel and userspace */
    ENTRY_TYPE_BUTT     = ENTRY_TYPE_shm,
} EntryType;

/*
 * kmpp system definitions including the kernel space accessor and
 * userspace accessor helper.
 */

/* KmppObjDef - kmpp object name size and access table trie definition */
typedef void* KmppObjDef;
/* KmppObj - kmpp object for string name access and function access */
typedef void* KmppObj;

/* KmppShmGrp - share memory device for allocation and free */
typedef void* KmppShmGrp;
/* KmppShm - share memory for address access and read / write */
typedef void* KmppShm;

/* MUST be the same to the KmppObjShm in rk-mpp-kobj.h */
typedef struct KmppShmAddr_t {
    /* uaddr - the userspace base address for kernel access */
    union {
        rk_u64 uaddr;
        void *uptr;
    };
    /* kaddr - the kernel base address for kernel access */
    union {
        rk_u64 kaddr;
        void *kptr;
    };
    /* DO NOT access reserved data only used by kernel */
} KmppShmPtr;

#endif /* __KMPP_SYS_DEFS_H__ */