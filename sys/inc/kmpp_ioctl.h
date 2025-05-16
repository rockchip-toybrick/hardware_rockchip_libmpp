/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_IOCTL_H__
#define __KMPP_IOCTL_H__

#include "rk_type.h"

typedef void* KmppIoc;

#define KMPP_IOC_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    ENTRY(prefix, u32,  rk_u32,         def,    ELEM_FLAG_NONE, def) \
    ENTRY(prefix, u32,  rk_u32,         cmd,    ELEM_FLAG_NONE, cmd) \
    ENTRY(prefix, u32,  rk_u32,         flags,  ELEM_FLAG_NONE, flags) \
    ENTRY(prefix, u32,  rk_u32,         id,     ELEM_FLAG_NONE, id) \
    STRCT(prefix, shm,  KmppShmPtr,     in,     ELEM_FLAG_NONE, in) \
    STRCT(prefix, shm,  KmppShmPtr,     out,    ELEM_FLAG_NONE, out)

#define KMPP_OBJ_NAME                   kmpp_ioc
#define KMPP_OBJ_INTF_TYPE              KmppIoc
#define KMPP_OBJ_ENTRY_TABLE            KMPP_IOC_ENTRY_TABLE
#include "kmpp_obj_func.h"

#endif /*__KMPP_IOCTL_H__*/
