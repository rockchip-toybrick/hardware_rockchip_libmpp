/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Rockchip mpp system driver ioctl object
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef _UAPI_RK_MPP_KOBJ_H
#define _UAPI_RK_MPP_KOBJ_H

#include <linux/types.h>

/*
 * kernel object trie share info
 * used in KMPP_SHM_IOC_QUERY_INFO
 *
 * input  : object name userspace address
 * output : trie_root userspace address (read only)
 */
typedef union KmppObjTrie_u {
    __u64       name_uaddr;
    __u64       trie_root;
} KmppObjTrie;

/* kernel object share memory */
typedef struct KmppObjShm_u {
    union {
    __u64       name_uaddr;
    __u64       kobj_uaddr;
    };
    __u64       kobj_kaddr;
    /* DO NOT access reserved data only used by kernel */
} KmppObjShm;

#endif /* _UAPI_RK_MPP_KOBJ_H */
