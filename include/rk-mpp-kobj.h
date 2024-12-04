/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Rockchip mpp system driver ioctl object
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef _UAPI_RK_MPP_KOBJ_H
#define _UAPI_RK_MPP_KOBJ_H

#include <linux/types.h>

/* kernel object trie share info */
typedef struct KmppObjTrie_t {
    /* share object trie root userspace address (read only) */
    __u64   trie_root;
} KmppObjTrie;

/* kernel object share memory */
typedef struct KmppObjShm_t {
    __u64   kobj_uaddr;
    __u64   kobj_kaddr;
    /* DO NOT access reserved data only used by kernel */
} KmppObjShm;

#endif /* _UAPI_RK_MPP_KOBJ_H */
