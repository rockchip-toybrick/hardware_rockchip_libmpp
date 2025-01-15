/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_META_IMPL_H__
#define __KMPP_META_IMPL_H__

#include "rk_list.h"
#include "kmpp_meta.h"
#include "kmpp_sys_defs.h"

#define MPP_TAG_SIZE            32

typedef struct __attribute__((packed)) KmppMetaVal_t {
    rk_u32              state;
    union {
        rk_s32          val_s32;
        rk_s64          val_s64;
        void            *val_ptr;
    };
} KmppMetaVal;

typedef struct __attribute__((packed)) KmppMetaShmVal_t {
    rk_u32              state;
    KmppShmPtr          val_shm;
} KmppMetaObj;

typedef struct __attribute__((packed)) KmppMetaVals_t {
    KmppMetaObj         in_frm;
    KmppMetaObj         in_pkt;
    KmppMetaObj         out_frm;
    KmppMetaObj         out_pkt;

    KmppMetaObj         md_buf;
    KmppMetaObj         hdr_buf;
    KmppMetaVal         hdr_meta_offset;
    KmppMetaVal         hdr_meta_size;

    KmppMetaVal         in_block;
    KmppMetaVal         out_block;
    KmppMetaVal         in_idr_req;
    KmppMetaVal         out_intra;

    KmppMetaVal         temporal_id;
    KmppMetaVal         lt_ref_idx;
    KmppMetaVal         enc_avg_qp;
    KmppMetaVal         enc_start_qp;
    KmppMetaVal         enc_bps_rt;

    KmppMetaObj         enc_roi;
    KmppMetaObj         enc_roi2;
    KmppMetaObj         enc_osd;
    KmppMetaObj         enc_osd2;
    KmppMetaObj         usr_data;
    KmppMetaObj         usr_datas;
    KmppMetaObj         enc_qpmap0;
    KmppMetaObj         enc_mv_list;

    KmppMetaVal         enc_inter64_num;
    KmppMetaVal         enc_inter32_num;
    KmppMetaVal         enc_inter16_num;
    KmppMetaVal         enc_inter8_num;
    KmppMetaVal         enc_intra32_num;
    KmppMetaVal         enc_intra16_num;
    KmppMetaVal         enc_intra8_num;
    KmppMetaVal         enc_intra4_num;
    KmppMetaVal         enc_out_pskip;
    KmppMetaVal         enc_in_skip;
    KmppMetaVal         enc_sse;

    KmppMetaVal         enc_mark_ltr;
    KmppMetaVal         enc_use_ltr;
    KmppMetaVal         enc_frm_qp;
    KmppMetaVal         enc_base_layer_pid;

    KmppMetaVal         dec_thumb_en;
    KmppMetaVal         dec_thumb_y_offset;
    KmppMetaVal         dec_thumb_uv_offset;
} KmppMetaVals;

typedef struct __attribute__((packed)) KmppMetaImpl_t {
    const rk_u8         *caller;
    rk_s32              meta_id;
    rk_s32              ref_count;

    osal_list_head      list;
    rk_s32              node_count;
    KmppMetaVals        vals;
} KmppMetaImpl;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET kmpp_meta_inc_ref(KmppMeta meta);

#ifdef __cplusplus
}
#endif

#endif /*__KMPP_META_IMPL_H__*/
