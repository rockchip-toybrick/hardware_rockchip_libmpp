/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPI_VDEC_H__
#define __KMPI_VDEC_H__

#include "kmpi_defs.h"

/* string - value config for control flow */
typedef void* KmppVdecInitCfg;
typedef void* KmppVdecDeinitCfg;
typedef void* KmppVdecResetCfg;
typedef void* KmppVdecStartCfg;
typedef void* KmppVdecStopCfg;

/* static config */
typedef void* KmppVdecStCfg;
/* runtime config */
typedef void* KmppVdecRtCfg;

/* channel status change sync operation and allow bulk operation */
rk_s32 kmpp_vdec_init(KmppCtx *ctx, KmppVdecInitCfg cfg);
rk_s32 kmpp_vdec_deinit(KmppCtx ctx, KmppVdecDeinitCfg cfg);

rk_s32 kmpp_vdec_reset(KmppCtx ctx, KmppVdecResetCfg cfg);
rk_s32 kmpp_vdec_start(KmppCtx ctx, KmppVdecStartCfg cfg);
rk_s32 kmpp_vdec_stop(KmppCtx ctx, KmppVdecStopCfg cfg);
/* runtime query / status check */
rk_s32 kmpp_vdec_ctrl(KmppCtx ctx, rk_s32 cmd, void *arg);

/* static config */
rk_s32 kmpp_vdec_get_cfg(KmppCtx ctx, KmppVdecStCfg cfg);
rk_s32 kmpp_vdec_set_cfg(KmppCtx ctx, KmppVdecStCfg cfg);

/* runtime config */
rk_s32 kmpp_vdec_get_rt_cfg(KmppCtx ctx, KmppVdecRtCfg cfg);
rk_s32 kmpp_vdec_set_rt_cfg(KmppCtx ctx, KmppVdecRtCfg cfg);

rk_s32 kmpp_vdec_decode(KmppCtx ctx, KmppPacket pkt, KmppFrame *frm);
rk_s32 kmpp_vdec_put_pkt(KmppCtx ctx, KmppPacket pkt);
rk_s32 kmpp_vdec_get_frm(KmppCtx ctx, KmppFrame *frm);

#endif /*__KMPI_VDEC_H__*/
