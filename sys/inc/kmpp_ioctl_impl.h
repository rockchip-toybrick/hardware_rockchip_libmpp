/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_IOCTL_IMPL_H__
#define __KMPP_IOCTL_IMPL_H__

#include "kmpp_ioctl.h"

typedef struct KmppIocImpl_t {
    /* object defintion index for ioctl functions */
    rk_u32              def;
    /* object defintion ioctl command */
    rk_u32              cmd;
    /*
     * flags for:
     * last in the batch / not last
     * block / non-block
     * return / non-return
     * sync / async
     * ack / non-ack
     * direct call / timer call
     */
    rk_u32              flags;
    /* ioc object id for input and output queue match */
    rk_u32              id;
    /* input config object */
    KmppShmPtr          in;
    /* output return object */
    KmppShmPtr          out;
} KmppIocImpl;

rk_s32 kmpp_ioctl_init(void);
rk_s32 kmpp_ioctl_deinit(void);

#endif /* __KMPP_IOCTL_IMPL_H__ */
