/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_SYS_DEFS_H__
#define __KMPP_SYS_DEFS_H__

#include "kmpi_defs.h"

typedef enum EntryType_e {
    /* commaon fix size value */
    ENTRY_TYPE_FIX      = 0x0,
    ENTRY_TYPE_s32      = (ENTRY_TYPE_FIX + 0),
    ENTRY_TYPE_u32      = (ENTRY_TYPE_FIX + 1),
    ENTRY_TYPE_s64      = (ENTRY_TYPE_FIX + 2),
    ENTRY_TYPE_u64      = (ENTRY_TYPE_FIX + 3),
    /* value only structure */
    ENTRY_TYPE_st       = (ENTRY_TYPE_FIX + 4),

    /* kernel and userspace share data */
    ENTRY_TYPE_SHARE    = 0x6,
    /* share memory between kernel and userspace */
    ENTRY_TYPE_shm      = (ENTRY_TYPE_SHARE + 0),

    /* kernel access only data */
    ENTRY_TYPE_KERNEL   = 0x8,
    /* kenrel object poineter */
    ENTRY_TYPE_kobj     = (ENTRY_TYPE_KERNEL + 0),
    /* kenrel normal data poineter */
    ENTRY_TYPE_kptr     = (ENTRY_TYPE_KERNEL + 1),
    /* kernel function poineter */
    ENTRY_TYPE_kfp      = (ENTRY_TYPE_KERNEL + 2),

    /* userspace access only data */
    ENTRY_TYPE_USER     = 0xc,
    /* userspace object poineter */
    ENTRY_TYPE_uobj     = (ENTRY_TYPE_USER + 0),
    /* userspace normal data poineter */
    ENTRY_TYPE_uptr     = (ENTRY_TYPE_USER + 1),
    /* userspace function poineter */
    ENTRY_TYPE_ufp      = (ENTRY_TYPE_USER + 2),

    ENTRY_TYPE_BUTT     = 0xf,
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

#endif /* __KMPP_SYS_DEFS_H__ */