// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#ifndef __MPP_FRAME_IMPL_H__
#define __MPP_FRAME_IMPL_H__

#include "mpp_frame.h"
#include "rk_venc_cmd.h"

typedef struct MppFrameImpl_t MppFrameImpl;

struct MppFrameImpl_t {
	const char *name;

	/*
	 * dimension parameter for display
	 */
	RK_U32 width;
	RK_U32 height;
	RK_U32 hor_stride;
	RK_U32 ver_stride;
	RK_U32 hor_stride_pixel;
	RK_U32 offset_x;
	RK_U32 offset_y;

	/*
	 * poc - picture order count
	 */
	RK_U32 poc;
	/*
	 * pts - display time stamp
	 * dts - decode time stamp
	 */
	RK_S64 pts;
	RK_S64 dts;

	/*
	 * eos - end of stream
	 * info_change - set when buffer resized or frame infomation changed
	 */
	RK_U32 eos;
	MppFrameColorRange color_range;
	MppFrameColorPrimaries color_primaries;
	MppFrameColorTransferCharacteristic color_trc;

	/**
	 * YUV colorspace type.
	 * It must be accessed using av_frame_get_colorspace() and
	 * av_frame_set_colorspace().
	 * - encoding: Set by user
	 * - decoding: Set by libavcodec
	 */
	MppFrameColorSpace colorspace;
	MppFrameChromaLocation chroma_location;

	MppFrameFormat fmt;

	MppFrameRational sar;

	/*
	 * buffer information
	 * NOTE: buf_size only access internally
	 */
	MppBuffer buffer;
	size_t buf_size;

	/*
	 * meta data information
	 */
//    MppMeta         meta;

	/*
	 * frame buffer compression (FBC) information
	 *
	 * NOTE: some constraint on fbc data
	 * 1. FBC config need two addresses but only one buffer.
	 *    The second address should be represented by base + offset form.
	 * 2. FBC has header address and payload address
	 *    Both addresses should be 4K aligned.
	 * 3. The header section size is default defined by:
	 *    header size = aligned(aligned(width, 16) * aligned(height, 16) / 16, 4096)
	 * 4. The stride in header section is defined by:
	 *    stride = aligned(width, 16)
	 */
	RK_U32 fbc_offset;

	MppRoi   roi;
	MppOsd   osd;
	RK_U32   is_gray;
	RK_U32   is_full;
	RK_U32   phy_addr;
	MppPpInfo pp_info;

	RK_U32 idr_request;
	RK_U32 pskip_request;
	RK_U32 pskip_num;
	MppFrame combo_frame;
	RK_U32 chan_id;
};

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_frame_info_cmp(MppFrame frame0, MppFrame frame1);
RK_U32 mpp_frame_get_fbc_offset(MppFrame frame);
RK_U32 mpp_frame_get_fbc_stride(MppFrame frame);

MPP_RET check_is_mpp_frame(void *pointer);

#ifdef __cplusplus
}
#endif
#endif /*__MPP_FRAME_IMPL_H__*/
