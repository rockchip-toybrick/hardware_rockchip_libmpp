/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_IOCTL_H__
#define __KMPP_IOCTL_H__

#include "rk_type.h"

typedef void* KmppIoc;

#define KMPP_IOC_ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, u32,  rk_u32,         def) \
    ENTRY(prefix, u32,  rk_u32,         cmd) \
    ENTRY(prefix, u32,  rk_u32,         flags) \
    ENTRY(prefix, u32,  rk_u32,         id)

#define KMPP_IOC_STRUCT_TABLE(ENTRY, prefix) \
    ENTRY(prefix, shm,  KmppShmPtr,     in) \
    ENTRY(prefix, shm,  KmppShmPtr,     out)

#define KMPP_OBJ_NAME                   kmpp_ioc
#define KMPP_OBJ_INTF_TYPE              KmppIoc
#define KMPP_OBJ_ENTRY_TABLE            KMPP_IOC_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE           KMPP_IOC_STRUCT_TABLE
#include "kmpp_obj_func.h"

#endif /*__KMPP_IOCTL_H__*/
