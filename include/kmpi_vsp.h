/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPI_VSP_H__
#define __KMPI_VSP_H__

#include "kmpi_defs.h"

/* string - value config for control flow */
typedef void* KmppVspInitCfg;
typedef void* KmppVspDeinitCfg;
typedef void* KmppVspResetCfg;
typedef void* KmppVspStartCfg;
typedef void* KmppVspStopCfg;

/* static config */
typedef void* KmppVspStCfg;
/* runtime config */
typedef void* KmppVspRtCfg;

/* channel status change sync operation and allow bulk operation */
rk_s32 kmpp_vsp_init(KmppCtx *ctx, KmppVspInitCfg cfg);
rk_s32 kmpp_vsp_deinit(KmppCtx ctx, KmppVspDeinitCfg cfg);

rk_s32 kmpp_vsp_reset(KmppCtx ctx, KmppVspResetCfg cfg);
rk_s32 kmpp_vsp_start(KmppCtx ctx, KmppVspStartCfg cfg);
rk_s32 kmpp_vsp_stop(KmppCtx ctx, KmppVspStopCfg cfg);
/* runtime query / status check */
rk_s32 kmpp_vsp_ctrl(KmppCtx ctx, rk_s32 cmd, void *arg);

/* static config */
rk_s32 kmpp_vsp_get_cfg(KmppCtx ctx, KmppVspStCfg cfg);
rk_s32 kmpp_vsp_set_cfg(KmppCtx ctx, KmppVspStCfg cfg);

/* runtime config */
rk_s32 kmpp_vsp_get_rt_cfg(KmppCtx ctx, KmppVspRtCfg cfg);
rk_s32 kmpp_vsp_set_rt_cfg(KmppCtx ctx, KmppVspRtCfg cfg);

rk_s32 kmpp_vsp_proc(KmppCtx ctx, KmppFrame in, KmppFrame *out);
rk_s32 kmpp_vsp_put_frm(KmppCtx ctx, KmppFrame frm);
rk_s32 kmpp_vsp_get_frm(KmppCtx ctx, KmppFrame *frm);

#endif /*__KMPI_VSP_H__*/
