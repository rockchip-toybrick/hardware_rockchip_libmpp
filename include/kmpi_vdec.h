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

/*
 * There are two sets for interfaces:
 * context interface and chan id interface
 *
 * Each set of interface has the same interface functions:
 *
 * init / deinit
 * - decoder instance create / destroy interfaces.
 *
 * reset / start / stop / ctrl
 * - decoder control flow interfaces.
 *
 * cfg / rt_cfg
 * - decoder static parameter and runtime parameter interfaces.
 *
 * decode / put_pkt / get_frm
 * - decoder data flow interfaces.
 */

rk_s32 kmpp_vdec_init(KmppCtx *ctx, KmppVdecInitCfg cfg);
rk_s32 kmpp_vdec_deinit(KmppCtx ctx, KmppVdecDeinitCfg cfg);
rk_s32 kmpp_vdec_chan_init(KmppChanId *id, KmppVdecInitCfg cfg);
rk_s32 kmpp_vdec_chan_deinit(KmppChanId id, KmppVdecDeinitCfg cfg);

rk_s32 kmpp_vdec_reset(KmppCtx ctx, KmppVdecResetCfg cfg);
rk_s32 kmpp_vdec_start(KmppCtx ctx, KmppVdecStartCfg cfg);
rk_s32 kmpp_vdec_stop(KmppCtx ctx, KmppVdecStopCfg cfg);
rk_s32 kmpp_vdec_ctrl(KmppCtx ctx, rk_s32 cmd, void *arg);
rk_s32 kmpp_vdec_chan_reset(KmppChanId id, KmppVdecResetCfg cfg);
rk_s32 kmpp_vdec_chan_start(KmppChanId id, KmppVdecStartCfg cfg);
rk_s32 kmpp_vdec_chan_stop(KmppChanId id, KmppVdecStopCfg cfg);
rk_s32 kmpp_vdec_chan_ctrl(KmppChanId id, rk_s32 cmd, void *arg);

/* static config */
rk_s32 kmpp_vdec_get_cfg(KmppCtx ctx, KmppVdecStCfg cfg);
rk_s32 kmpp_vdec_set_cfg(KmppCtx ctx, KmppVdecStCfg cfg);
rk_s32 kmpp_vdec_chan_get_cfg(KmppChanId id, KmppVdecStCfg cfg);
rk_s32 kmpp_vdec_chan_set_cfg(KmppChanId id, KmppVdecStCfg cfg);

/* runtime config */
rk_s32 kmpp_vdec_get_rt_cfg(KmppCtx ctx, KmppVdecRtCfg cfg);
rk_s32 kmpp_vdec_set_rt_cfg(KmppCtx ctx, KmppVdecRtCfg cfg);
rk_s32 kmpp_vdec_chan_get_rt_cfg(KmppChanId id, KmppVdecRtCfg cfg);
rk_s32 kmpp_vdec_chan_set_rt_cfg(KmppChanId id, KmppVdecRtCfg cfg);

rk_s32 kmpp_vdec_decode(KmppCtx ctx, KmppPacket pkt, KmppFrame *frm);
rk_s32 kmpp_vdec_put_pkt(KmppCtx ctx, KmppPacket pkt);
rk_s32 kmpp_vdec_get_frm(KmppCtx ctx, KmppFrame *frm);
rk_s32 kmpp_vdec_chan_decode(KmppChanId id, KmppPacket pkt, KmppFrame *frm);
rk_s32 kmpp_vdec_chan_put_pkt(KmppChanId id, KmppPacket pkt);
rk_s32 kmpp_vdec_chan_get_frm(KmppChanId id, KmppFrame *frm);

#endif /*__KMPI_VDEC_H__*/
