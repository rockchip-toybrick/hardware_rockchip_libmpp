/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/kernel.h>

#include "kmpp_log.h"
#include "kmpp_mem.h"

#include "kmpi_vsp.h"

rk_s32 kmpp_vsp_init(KmppCtx *ctx, KmppVspInitCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_deinit(KmppCtx ctx, KmppVspDeinitCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_init(KmppChanId *id, KmppVspInitCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_deinit(KmppChanId id, KmppVspDeinitCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_reset(KmppCtx ctx, KmppVspResetCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_start(KmppCtx ctx, KmppVspStartCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_stop(KmppCtx ctx, KmppVspStopCfg cfg)
{
    return rk_ok;
}

/* runtime query / status check */
rk_s32 kmpp_vsp_ctrl(KmppCtx ctx, rk_s32 cmd, void *arg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_reset(KmppChanId id, KmppVspResetCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_start(KmppChanId id, KmppVspStartCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_stop(KmppChanId id, KmppVspStopCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_ctrl(KmppChanId id, rk_s32 cmd, void *arg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_get_cfg(KmppCtx ctx, KmppVspStCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_set_cfg(KmppCtx ctx, KmppVspStCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_get_cfg(KmppChanId id, KmppVspStCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_set_cfg(KmppChanId id, KmppVspStCfg cfg)
{
    return rk_ok;
}

/* runtime config */
rk_s32 kmpp_vsp_get_rt_cfg(KmppCtx ctx, KmppVspRtCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_set_rt_cfg(KmppCtx ctx, KmppVspRtCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_get_rt_cfg(KmppChanId id, KmppVspRtCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_set_rt_cfg(KmppChanId id, KmppVspRtCfg cfg)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_proc(KmppCtx ctx, KmppFrame in, KmppFrame *out)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_put_frm(KmppCtx ctx, KmppFrame frm)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_get_frm(KmppCtx ctx, KmppFrame *frm)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_proc(KmppChanId id, KmppFrame in, KmppFrame *out)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_put_frm(KmppChanId id, KmppFrame frm)
{
    return rk_ok;
}

rk_s32 kmpp_vsp_chan_get_frm(KmppChanId id, KmppFrame *frm)
{
    return rk_ok;
}

EXPORT_SYMBOL(kmpp_vsp_init);
EXPORT_SYMBOL(kmpp_vsp_deinit);
EXPORT_SYMBOL(kmpp_vsp_chan_init);
EXPORT_SYMBOL(kmpp_vsp_chan_deinit);
EXPORT_SYMBOL(kmpp_vsp_reset);
EXPORT_SYMBOL(kmpp_vsp_start);
EXPORT_SYMBOL(kmpp_vsp_stop);
EXPORT_SYMBOL(kmpp_vsp_ctrl);
EXPORT_SYMBOL(kmpp_vsp_chan_reset);
EXPORT_SYMBOL(kmpp_vsp_chan_start);
EXPORT_SYMBOL(kmpp_vsp_chan_stop);
EXPORT_SYMBOL(kmpp_vsp_chan_ctrl);
EXPORT_SYMBOL(kmpp_vsp_get_cfg);
EXPORT_SYMBOL(kmpp_vsp_set_cfg);
EXPORT_SYMBOL(kmpp_vsp_chan_get_cfg);
EXPORT_SYMBOL(kmpp_vsp_chan_set_cfg);
EXPORT_SYMBOL(kmpp_vsp_get_rt_cfg);
EXPORT_SYMBOL(kmpp_vsp_set_rt_cfg);
EXPORT_SYMBOL(kmpp_vsp_chan_get_rt_cfg);
EXPORT_SYMBOL(kmpp_vsp_chan_set_rt_cfg);
EXPORT_SYMBOL(kmpp_vsp_proc);
EXPORT_SYMBOL(kmpp_vsp_put_frm);
EXPORT_SYMBOL(kmpp_vsp_get_frm);
EXPORT_SYMBOL(kmpp_vsp_chan_proc);
EXPORT_SYMBOL(kmpp_vsp_chan_put_frm);
EXPORT_SYMBOL(kmpp_vsp_chan_get_frm);