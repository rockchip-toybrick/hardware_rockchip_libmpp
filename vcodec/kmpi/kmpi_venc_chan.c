/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/dma-buf.h>
#include <linux/list.h>

#include "mpp_mem.h"
#include "mpp_enc.h"
#include "mpp_log.h"
#include "mpp_enc_cfg_impl.h"
#include "mpp_packet_impl.h"

#include "kmpp_obj.h"
#include "kmpp_venc_objs_impl.h"
#include "kmpp_atomic.h"

#include "mpp_vcodec_base.h"
#include "mpp_vcodec_thread.h"
#include "mpp_vcodec_chan.h"
#include "mpp_vcodec_flow.h"

rk_s32 kmpp_venc_chan_init(KmppChanId *id, KmppVencInitCfg cfg)
{
    MppEnc enc = NULL;
    MPP_RET ret = MPP_OK;
    KmppChanId chan_id = *id;
    struct mpp_chan *chan = NULL;
    MppEncInitCfg init_cfg;
    KmppVencInitCfgImpl *cfg_impl = (KmppVencInitCfgImpl*)kmpp_obj_to_entry(cfg);

    if (chan_id >= 0 && chan_id < MAX_ENC_NUM)
        chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);

    if (!chan || chan->handle) {
        chan_id = mpp_vcodec_get_free_chan(MPP_CTX_ENC);
        if (chan_id < 0)
            return MPP_NOK;

        chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    }

    init_cfg.chan_id = chan_id;
    init_cfg.shared_buf = &chan->shared_buf;
    init_cfg.coding = cfg_impl->coding;
    init_cfg.online = cfg_impl->online;
    init_cfg.buf_size = cfg_impl->buf_size;
    init_cfg.ref_buf_shared = cfg_impl->shared_buf_en;
    init_cfg.max_strm_cnt = cfg_impl->max_strm_cnt;
    init_cfg.smart_en = cfg_impl->smart_en;
    init_cfg.qpmap_en = cfg_impl->qpmap_en;
    init_cfg.only_smartp = cfg_impl->only_smartp;
    init_cfg.tmvp_enable = cfg_impl->tmvp_enable;

    ret = mpp_enc_init(&enc, &init_cfg);
    if (ret) {
        mpp_err("mpp_enc_init failed ret %d\n", ret);
        return ret;
    }
    kmpp_venc_init_cfg_set_chan_id(cfg, chan_id);
    mpp_enc_register_chl(enc, mpp_vcodec_enc_int_handle, init_cfg.chan_id);

    mpp_vcodec_chan_entry_init(chan, MPP_CTX_ENC, init_cfg.coding, (void *)enc);
    mpp_vcodec_inc_chan_num(MPP_CTX_ENC);

    *id = chan_id;

    return ret;
}

rk_s32 kmpp_venc_chan_deinit(KmppChanId id, KmppVencDeinitCfg cfg)
{
    struct mpp_chan *chan = NULL;
    bool wait = true;
    KmppChanId chan_id = id;

    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    if (!chan || !chan->handle) {
        mpp_err_f("invalid chan id %d\n", chan_id);
        return MPP_NOK;
    }
    mpp_log("destroy chan %d hnd %p online %d combo %d mst %d\n",
            chan_id, chan->handle, chan->cfg.online,
            chan->binder_chan_id, chan->master_chan_id);

    kmpp_venc_chan_stop(chan_id, cfg);
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
    mutex_lock(&chan->chan_debug_lock);
    mpp_enc_deinit(chan->handle);
    mpp_vcodec_stream_clear(chan);
    mpp_vcodec_dec_chan_num(MPP_CTX_ENC);
    mpp_vcodec_chan_entry_deinit(chan);
    mutex_unlock(&chan->chan_debug_lock);

    mpp_log("destroy chan %d done\n", chan_id);

    return 0;
}

rk_s32 kmpp_venc_chan_start(KmppChanId id, KmppVencStartCfg cfg)
{
    unsigned long lock_flag;
    struct mpp_chan *chan = NULL;
    KmppChanId chan_id = id;

    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    if (!chan || !chan->handle) {
        mpp_err_f("invalid chan id %d\n", chan_id);
        return MPP_NOK;
    }

    mpp_enc_start(chan->handle);
    spin_lock_irqsave(&chan->chan_lock, lock_flag);
    chan->state = CHAN_STATE_RUN;
    spin_unlock_irqrestore(&chan->chan_lock, lock_flag);
    enc_chan_update_chan_prior_tab();

    return 0;
}

rk_s32 kmpp_venc_chan_stop(KmppChanId id, KmppVencStopCfg cfg)
{
    unsigned long lock_flag;
    struct mpp_chan *chan = NULL;
    KmppChanId chan_id = id;

    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    if (!chan || !chan->handle) {
        mpp_err_f("invalid chan id %d\n", chan_id);
        return MPP_NOK;
    }

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
rk_s32 kmpp_venc_chan_ctrl(KmppChanId id, rk_s32 cmd, void *arg)
{
    struct mpp_chan *chan = NULL;
    KmppChanId chan_id = id;

    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    if (!chan || !chan->handle) {
        mpp_err_f("invalid chan %d\n", chan_id);
        return MPP_NOK;
    }

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
rk_s32 kmpp_venc_chan_get_cfg(KmppChanId id, KmppVencStCfg cfg)
{
    return 0;
}

rk_s32 kmpp_venc_chan_set_cfg(KmppChanId id, KmppVencStCfg cfg)
{
    return 0;
}

/* runtime config */
rk_s32 kmpp_venc_chan_get_rt_cfg(KmppChanId id, KmppVencRtCfg cfg)
{
    return 0;
}

rk_s32 kmpp_venc_chan_set_rt_cfg(KmppChanId id, KmppVencRtCfg cfg)
{
    return 0;
}

rk_s32 kmpp_venc_chan_encode(KmppChanId id, KmppFrame frm, KmppPacket *pkt)
{
    return 0;
}

rk_s32 kmpp_venc_chan_put_frm(KmppChanId id, KmppFrame frm)
{
    struct mpp_chan *chan = NULL;
    struct venc_module *venc = NULL;
    struct vcodec_threads *thd;
    KmppChanId chan_id = id;

    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    if (!chan || !chan->handle) {
        mpp_err_f("invalid chan id %d\n", chan_id);
        return MPP_NOK;
    }

    venc = mpp_vcodec_get_enc_module_entry();
    thd = venc->thd;
    if (osal_cmpxchg(&chan->frame, chan->frame, chan->frame)) {
        mpp_err_f("chan %d frame %p is not NULL\n", chan_id, chan->frame);
        return MPP_NOK;
    }
    osal_cmpxchg(&chan->frame, chan->frame, frm);
    vcodec_thread_trigger(thd);

    return 0;
}

rk_s32 kmpp_venc_chan_get_pkt(KmppChanId id, KmppPacket *pkt)
{
    struct mpp_chan *chan = NULL;
    MppPacketImpl *packet = NULL;
    RK_UL flags;
    KmppChanId chan_id = id;

    chan = mpp_vcodec_get_chan_entry(chan_id, MPP_CTX_ENC);
    if (!chan || !chan->handle) {
        mpp_err_f("invalid chan id %d\n", chan_id);
        return MPP_NOK;
    }

    if (!atomic_read(&chan->stream_count))
        wait_event_timeout(chan->wait, atomic_read(&chan->stream_count), 500);

    spin_lock_irqsave(&chan->stream_list_lock, flags);
    packet = list_first_entry_or_null(&chan->stream_done, MppPacketImpl, list);
    if (packet)
        list_del_init(&packet->list);
    spin_unlock_irqrestore(&chan->stream_list_lock, flags);

    if (!packet) {
        mpp_err_f("no packet in stream_done list\n");
        return MPP_NOK;
    }
    /* flush cache before get packet*/
    if (packet->buf.buf)
        mpp_ring_buf_flush(&packet->buf, 1);
    atomic_dec(&chan->stream_count);
    chan->seq_user_get = mpp_packet_get_dts(packet);
    *pkt = (KmppPacket)packet;

    return 0;
}

EXPORT_SYMBOL(kmpp_venc_chan_init);
EXPORT_SYMBOL(kmpp_venc_chan_deinit);
EXPORT_SYMBOL(kmpp_venc_chan_start);
EXPORT_SYMBOL(kmpp_venc_chan_stop);
EXPORT_SYMBOL(kmpp_venc_chan_ctrl);
EXPORT_SYMBOL(kmpp_venc_chan_get_cfg);
EXPORT_SYMBOL(kmpp_venc_chan_set_cfg);
EXPORT_SYMBOL(kmpp_venc_chan_get_rt_cfg);
EXPORT_SYMBOL(kmpp_venc_chan_set_rt_cfg);
EXPORT_SYMBOL(kmpp_venc_chan_encode);
EXPORT_SYMBOL(kmpp_venc_chan_put_frm);
EXPORT_SYMBOL(kmpp_venc_chan_get_pkt);