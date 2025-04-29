/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */
#include <linux/dma-buf.h>
#include <linux/list.h>

#include "mpp_mem.h"
#include "mpp_enc.h"
#include "mpp_log.h"
#include "rk_type.h"
#include "rk_err_def.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_packet_impl.h"

#include "kmpi_venc.h"

#include "mpp_vcodec_base.h"
#include "mpp_vcodec_thread.h"
#include "mpp_vcodec_chan.h"
#include "mpp_vcodec_flow.h"

typedef struct KmppVencCtx_t {
    MppEnc enc;
    RK_U32 chan_id;
} KmppVencCtx;

rk_s32 kmpp_venc_init(KmppCtx *ctx, KmppVencInitCfg cfg)
{
    KmppVencCtx *p = NULL;
    MppEnc enc = NULL;
    MPP_RET ret = MPP_OK;
    struct mpp_chan *chan = NULL;
    MppEncInitCfg *init_cfg = (MppEncInitCfg *)cfg;
    RK_S32 chan_id = init_cfg->chan_id;

    if (chan_id >= 0 && chan_id < MAX_ENC_NUM)
        chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);

    if (!chan || chan->handle) {
        chan_id = mpp_vcodec_get_free_chan(MPP_CTX_ENC);
        if (chan_id < 0)
            return MPP_NOK;

        chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
        init_cfg->chan_id = chan_id;
    }

    p = mpp_calloc(KmppVencCtx, 1);
    if (!p)
        return MPP_NOK;

    ret = mpp_enc_init(&enc, init_cfg);
    if (ret) {
        mpp_err("mpp_enc_init failed ret %d\n", ret);
        mpp_free(p);
        return ret;
    }

    mpp_enc_register_chl(enc, mpp_vcodec_enc_int_handle, init_cfg->chan_id);

    mpp_vcodec_chan_entry_init(chan, MPP_CTX_ENC, init_cfg->coding, (void *)enc);
    mpp_vcodec_inc_chan_num(MPP_CTX_ENC);

    p->chan_id = chan_id;
    p->enc = enc;
    *ctx = p;

    return ret;
}

rk_s32 kmpp_venc_deinit(KmppCtx ctx, KmppVencDeinitCfg cfg)
{
    KmppVencCtx *p = NULL;
    struct mpp_chan *chan = NULL;
    bool wait = true;
    RK_U32 chan_id;

    if (!ctx) {
        mpp_err_f("ctx is null\n");
        return MPP_NOK;
    }

    p = (KmppVencCtx *)ctx;
    chan_id = p->chan_id;
    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);

    mpp_log("destroy chan %d hnd %p online %d combo %d mst %d\n",
            chan_id, chan->handle, chan->cfg.online,
            chan->binder_chan_id, chan->master_chan_id);

    kmpp_venc_stop(ctx, cfg);
    if (chan->cfg.online) {
        struct mpp_chan *chan_tmp = chan;

        if (chan->master_chan_id != -1) {
            chan_tmp = mpp_vcodec_get_chan_entry(chan->master_chan_id, MPP_CTX_ENC);
            mpp_enc_unbind_jpeg_task(chan_tmp->handle);
        }
        if (!mpp_enc_check_hw_running(chan_tmp->handle) &&
            !mpp_enc_check_is_int_process(chan_tmp->handle))
            wait = false;
    }

    if (wait)
        wait_event_timeout(chan->stop_wait, !atomic_read(&chan->runing), msecs_to_jiffies(2000));

    // if (chan->cfg.online)
    //     mpp_vcodec_chan_unbind(chan);
    mutex_lock(&chan->chan_mutex);
    mpp_enc_deinit(chan->handle);
    mpp_vcodec_stream_clear(chan);
    mpp_vcodec_dec_chan_num(MPP_CTX_ENC);
    mpp_vcodec_chan_entry_deinit(chan);
    mutex_unlock(&chan->chan_mutex);

    mpp_free(p);
    mpp_log("destroy chan %d done\n", chan_id);

    return 0;
}

rk_s32 kmpp_venc_start(KmppCtx ctx, KmppVencStartCfg cfg)
{
    unsigned long lock_flag;
    struct mpp_chan *chan = NULL;
    KmppVencCtx *p = NULL;

    if (!ctx) {
        mpp_err_f("ctx is null\n");
        return MPP_NOK;
    }

    p = (KmppVencCtx *)ctx;

    chan = mpp_vcodec_get_chan_entry(p->chan_id, MPP_CTX_ENC);
    mpp_assert(chan->handle != NULL);
    if (!chan->handle)
        return MPP_NOK;

    mpp_enc_start(chan->handle);
    spin_lock_irqsave(&chan->chan_lock, lock_flag);
    chan->state = CHAN_STATE_RUN;
    spin_unlock_irqrestore(&chan->chan_lock, lock_flag);
    enc_chan_update_chan_prior_tab();

    return 0;
}

rk_s32 kmpp_venc_stop(KmppCtx ctx, KmppVencStopCfg cfg)
{
    unsigned long lock_flag;
    KmppVencCtx *p = NULL;
    struct mpp_chan *chan = NULL;

    if (!ctx) {
        mpp_err_f("ctx is null\n");
        return MPP_NOK;
    }

    p = (KmppVencCtx *)ctx;

    chan = mpp_vcodec_get_chan_entry(p->chan_id, MPP_CTX_ENC);
    mpp_assert(chan->handle != NULL);
    if (!chan->handle)
        return MPP_NOK;

    mpp_enc_stop(chan->handle);
    spin_lock_irqsave(&chan->chan_lock, lock_flag);
    if (chan->state != CHAN_STATE_RUN) {
        spin_unlock_irqrestore(&chan->chan_lock, lock_flag);
        return 0;
    }
    chan->state = CHAN_STATE_SUSPEND;
    spin_unlock_irqrestore(&chan->chan_lock, lock_flag);
    wake_up(&chan->wait);
    enc_chan_update_chan_prior_tab();

    return 0;
}

/* runtime query / status check */
rk_s32 kmpp_venc_ctrl(KmppCtx ctx, rk_s32 cmd, void *arg)
{
    KmppVencCtx *p = NULL;
    struct mpp_chan *chan = NULL;

    if (!ctx) {
        mpp_err_f("ctx is null\n");
        return MPP_NOK;
    }

    p = (KmppVencCtx *)ctx;

    chan = mpp_vcodec_get_chan_entry(p->chan_id, MPP_CTX_ENC);
    mpp_assert(chan->handle != NULL);
    if (!chan->handle)
        return MPP_NOK;

    if (cmd == MPP_SET_VENC_INIT_KCFG) {
        //mpp_vcodec_chan_change_coding_type(chan_id, arg);
    } else {
        MppEncCfgImpl *cfg = (MppEncCfgImpl *)arg;
        bool prep_change = cfg ? cfg->cfg.prep.change : false;

        mpp_enc_control(chan->handle, cmd, arg);
        /*
        * In the case of wrapping, when switching resolutions,
        * need to ensure that the wrapping frame being encoded finish.
        */
        if (chan->cfg.online && prep_change) {
            if (mpp_enc_check_hw_running(chan->handle)) {
                wait_event_timeout(chan->stop_wait, !atomic_read(&chan->runing), msecs_to_jiffies(200));
            }
        }
    }

    return 0;
}

/* static config */
rk_s32 kmpp_venc_get_cfg(KmppCtx ctx, KmppVencStCfg cfg)
{
    return 0;
}

rk_s32 kmpp_venc_set_cfg(KmppCtx ctx, KmppVencStCfg cfg)
{
    return 0;
}

/* runtime config */
rk_s32 kmpp_venc_get_rt_cfg(KmppCtx ctx, KmppVencRtCfg cfg)
{
    return 0;
}

rk_s32 kmpp_venc_set_rt_cfg(KmppCtx ctx, KmppVencRtCfg cfg)
{
    return 0;
}

rk_s32 kmpp_venc_encode(KmppCtx ctx, KmppFrame frm, KmppPacket *pkt)
{
    return 0;
}

rk_s32 kmpp_venc_put_frm(KmppCtx ctx, KmppFrame frm)
{
    KmppVencCtx *p = NULL;
    struct mpp_chan *chan = NULL;
    struct venc_module *venc = NULL;
    struct vcodec_threads *thd;

    if (!ctx) {
        mpp_err_f("ctx is null\n");
        return MPP_NOK;
    }

    p = (KmppVencCtx *)ctx;
    chan = mpp_vcodec_get_chan_entry(p->chan_id, MPP_CTX_ENC);
    mpp_assert(chan->handle != NULL);
    if (!chan->handle)
        return MPP_NOK;

    venc = mpp_vcodec_get_enc_module_entry();
    thd = venc->thd;

    chan->frame = frm;
    vcodec_thread_trigger(thd);

    return 0;
}

rk_s32 kmpp_venc_get_pkt(KmppCtx ctx, KmppPacket *pkt)
{
    KmppVencCtx *p = NULL;
    struct mpp_chan *chan = NULL;
    MppPacketImpl *packet = NULL;
    RK_UL flags;

    if (!ctx) {
        mpp_err_f("ctx is null\n");
        return MPP_NOK;
    }

    p = (KmppVencCtx *)ctx;
    chan = mpp_vcodec_get_chan_entry(p->chan_id, MPP_CTX_ENC);
    mpp_assert(chan->handle != NULL);
    if (!chan->handle)
        return MPP_NOK;

    if (!atomic_read(&chan->stream_count))
        wait_event_timeout(chan->wait, atomic_read(&chan->stream_count), 500);

    spin_lock_irqsave(&chan->stream_list_lock, flags);
    packet = list_first_entry_or_null(&chan->stream_done, MppPacketImpl, list);
    // list_move_tail(&packet->list, &chan->stream_remove);
    list_del_init(&packet->list);
    spin_unlock_irqrestore(&chan->stream_list_lock, flags);

    /* flush cache before get packet*/
    if (packet->buf.buf)
        mpp_ring_buf_flush(&packet->buf, 1);
    atomic_dec(&chan->stream_count);
    chan->seq_user_get = mpp_packet_get_dts(packet);
    *pkt = (KmppPacket)packet;
    // atomic_inc(&chan->str_out_cnt);
    // atomic_inc(&chan->pkt_user_get);

    return 0;
}

EXPORT_SYMBOL(kmpp_venc_init);
EXPORT_SYMBOL(kmpp_venc_deinit);
EXPORT_SYMBOL(kmpp_venc_start);
EXPORT_SYMBOL(kmpp_venc_stop);
EXPORT_SYMBOL(kmpp_venc_ctrl);
EXPORT_SYMBOL(kmpp_venc_get_cfg);
EXPORT_SYMBOL(kmpp_venc_set_cfg);
EXPORT_SYMBOL(kmpp_venc_get_rt_cfg);
EXPORT_SYMBOL(kmpp_venc_set_rt_cfg);
EXPORT_SYMBOL(kmpp_venc_encode);
EXPORT_SYMBOL(kmpp_venc_put_frm);
EXPORT_SYMBOL(kmpp_venc_get_pkt);