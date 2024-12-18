/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPI_VENC_H__
#define __KMPI_VENC_H__

#include "kmpi_defs.h"

/* string - value config for control flow */
typedef void* KmppVencInitCfg;
typedef void* KmppVencDeinitCfg;
typedef void* KmppVencResetCfg;
typedef void* KmppVencStartCfg;
typedef void* KmppVencStopCfg;

/* static config */
typedef void* KmppVencStCfg;
/* runtime config */
typedef void* KmppVencRtCfg;

/* channel status change sync operation and allow bulk operation */
rk_s32 kmpp_venc_init(KmppCtx *ctx, KmppVencInitCfg cfg);
rk_s32 kmpp_venc_deinit(KmppCtx ctx, KmppVencDeinitCfg cfg);

rk_s32 kmpp_venc_reset(KmppCtx ctx, KmppVencResetCfg cfg);
rk_s32 kmpp_venc_start(KmppCtx ctx, KmppVencStartCfg cfg);
rk_s32 kmpp_venc_stop(KmppCtx ctx, KmppVencStopCfg cfg);
/* runtime query / status check */
rk_s32 kmpp_venc_ctrl(KmppCtx ctx, rk_s32 cmd, void *arg);

/* static config */
rk_s32 kmpp_venc_get_cfg(KmppCtx ctx, KmppVencStCfg cfg);
rk_s32 kmpp_venc_set_cfg(KmppCtx ctx, KmppVencStCfg cfg);

/* runtime config */
rk_s32 kmpp_venc_get_rt_cfg(KmppCtx ctx, KmppVencRtCfg cfg);
rk_s32 kmpp_venc_set_rt_cfg(KmppCtx ctx, KmppVencRtCfg cfg);

rk_s32 kmpp_venc_encode(KmppCtx ctx, KmppFrame frm, KmppPacket *pkt);
rk_s32 kmpp_venc_put_frm(KmppCtx ctx, KmppFrame frm);
rk_s32 kmpp_venc_get_pkt(KmppCtx ctx, KmppPacket *pkt);

#endif /*__KMPI_VENC_H__*/
