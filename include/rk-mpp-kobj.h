/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Rockchip mpp system driver ioctl object
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef _UAPI_RK_MPP_KOBJ_H
#define _UAPI_RK_MPP_KOBJ_H

#include <linux/types.h>

#define KMPP_SHM_IOC_MAX_COUNT  64

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

/* kernel object share memory userspace visable header data */
typedef struct KmppObjShm_t {
    /* kobj_uaddr   - the userspace base address for kernel object */
    __u64       kobj_uaddr;
    /* kobj_kaddr   - the kernel base address for kernel object */
    __u64       kobj_kaddr;
    /* DO NOT access reserved data only used by kernel */
} KmppObjShm;

/* kernel object share memory get / put ioctl data */
typedef struct KmppObjIocArg_t {
    /* address array element count */
    __u32       count;

    /* flag for batch operation */
    __u32       flag;

    /*
     * at KMPP_SHM_IOC_GET_SHM
     * name_uaddr   - kernel object name in userspace address
     * kobj_uaddr   - kernel object userspace address for KmppObjShm
     *
     * at KMPP_SHM_IOC_PUT_SHM
     * kobj_uaddr   - kernel object userspace address for KmppObjShm
     */
    union {
    __u64       name_uaddr[0];
    __u64       kobj_uaddr[0];
    };
} KmppObjIocArg;

#endif /* _UAPI_RK_MPP_KOBJ_H */
