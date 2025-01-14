// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 *
 * author:
 *
 *
 */

#define MODULE_TAG "mpp_packet"

#include <linux/string.h>
#include <linux/module.h>
#include <linux/uaccess.h>

#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_maths.h"
#include "mpp_buffer_impl.h"
#include "mpp_stream_ring_buf.h"

RK_U32 ring_buf_debug = 0;
#define DEFALUT_STRM_CNT_IN_POOL 4
module_param(ring_buf_debug, uint, 0644);
MODULE_PARM_DESC(ring_buf_debug, "bits ring_buf debug information");

#define DEFAULT_MIN_ZISE (10 * SZ_1K)

MPP_RET ring_buf_init(ring_buf_pool *ctx, MppBuffer buf, RK_U32 max_strm_cnt)
{
	if (!ctx || ctx->init_done)
		return MPP_NOK;
	if (!max_strm_cnt)
		max_strm_cnt = DEFALUT_STRM_CNT_IN_POOL;
	ctx->w_pos = 0;
	ctx->r_pos = 0;
	ctx->len = mpp_buffer_get_size(buf);
	ctx->use_len = 0;
	ctx->max_use_len = 0;
	ctx->buf_base = mpp_buffer_get_ptr(buf);
	ctx->buf = buf;
	ctx->init_done = 1;
	ctx->min_buf_size = DEFAULT_MIN_ZISE;
	ring_buf_dbg("ctx->len = %d, max_strm_cnt = %d, ctx->min_buf_size = %d", ctx->len,
		     max_strm_cnt, ctx->min_buf_size);

	if (!ctx->buf_base)
		return MPP_NOK;

	return MPP_OK;
}

RK_U32 ring_buf_get_use_size(ring_buf_pool *ctx)
{
	if (!ctx)
		return 0;

	if (ctx->r_pos > ctx->w_pos)
		return ctx->len + ctx->w_pos - ctx->r_pos;

	return ctx->w_pos - ctx->r_pos;
}

MPP_RET ring_buf_put_use(ring_buf_pool *ctx, ring_buf *buf)
{
	RK_U32 w_pos = 0, r_pos = 0;
	RK_U32 start_pos = 0, end_pos = 0;

	if (!ctx || !buf || (ctx->buf != buf->buf) || !buf->use_len)
		return MPP_NOK;
	w_pos = ctx->w_pos;
	r_pos = ctx->r_pos;
	start_pos = buf->start_offset;
	end_pos = ((buf->start_offset + buf->use_len) % ctx->len);

	if (w_pos >= r_pos) {
		if ((start_pos < w_pos && start_pos > r_pos) ||
		    (end_pos >= r_pos && end_pos < w_pos)) {
			mpp_err("INVALID param: r_pos=%x, w_pos=%x, start=%x, size=%x, \
			    buf_len = %x\n", r_pos, w_pos, start_pos, buf->use_len,
				buf->size);
		}
	} else {
		if (start_pos >= r_pos || start_pos < w_pos ||
		    end_pos < w_pos || end_pos >= r_pos) {
			mpp_err("INVALID param: r_pos=%x, w_pos=%x, start=%x, size=%x, \
			    buf_len = %x\n", r_pos, w_pos, start_pos, buf->use_len,
				buf->size);
		}
	}

	if (end_pos < r_pos)
		ctx->use_len = ctx->len + end_pos - r_pos;
	else
		ctx->use_len = end_pos - r_pos;

	if (ctx->max_use_len < ctx->use_len)
		ctx->max_use_len = ctx->use_len;

	ctx->w_pos = end_pos;
	ctx->l_w_pos = end_pos;

	ring_buf_dbg(" pool %p use update ctx->r_pos %d ctx->w_pos %d ctx->use_len %d\n", ctx,
		     ctx->r_pos, ctx->w_pos, ctx->use_len);

	return MPP_OK;
}

MPP_RET ring_buf_put_free(ring_buf_pool *ctx, ring_buf *buf)
{
	RK_U32 w_pos = 0, r_pos = 0;
	RK_U32 start_pos = 0, end_pos = 0;

	if (!ctx || !buf || (ctx->buf != buf->buf) || !buf->use_len)
		return MPP_NOK;
	w_pos = ctx->w_pos;
	r_pos = ctx->r_pos;
	start_pos = buf->start_offset;
	end_pos = ((buf->start_offset + buf->use_len) % ctx->len);

	if (r_pos >= w_pos) {
		if ((end_pos < r_pos && end_pos > w_pos) ||
		    (start_pos >= w_pos && start_pos < r_pos)) {
			mpp_err("INVALID param: r_pos=%x, w_pos=%x, start=%x, size=%x\n",
				r_pos, w_pos, start_pos, buf->use_len);
		}
	} else {
		if (start_pos < r_pos || start_pos >= w_pos ||
		    end_pos < r_pos || end_pos > w_pos) {
			mpp_err("INVALID param: r_pos=%x, w_pos=%x, start=%x, size=%x\n",
				r_pos, w_pos, start_pos, buf->use_len);
		}
	}
	ctx->r_pos = end_pos;
	ctx->l_r_pos = end_pos;

	if (end_pos > w_pos)
		ctx->use_len = ctx->len - end_pos + w_pos;
	else
		ctx->use_len = w_pos - end_pos;

	ring_buf_dbg(" pool %p free update ctx->r_pos %d ctx->w_pos %d ctx->use_len %d \n", ctx,
		     ctx->r_pos, ctx->w_pos, ctx->use_len);

	return MPP_OK;
}

MPP_RET ring_buf_get_free(ring_buf_pool *ctx, ring_buf *buf, RK_U32 align,
			  RK_U32 min_size, RK_U32 stream_num)
{
	RK_U32 align_offset = 0;
	RK_U32 align_w_pos = 0, align_r_pos = 0;
	RK_U32 w_pos = 0, r_pos = 0;

	if (!ctx || !buf)
		return MPP_NOK;
	if (!min_size)
		min_size = ctx->w_pos == ctx->r_pos ? DEFAULT_MIN_ZISE : ctx->min_buf_size;
	w_pos = ctx->w_pos;
	r_pos = ctx->r_pos;
	buf->cir_flag = 1;

	ring_buf_dbg("get free pool %p ctx->r_pos %d ctx->w_pos %d ctx->use_len %d\n", ctx,
		     ctx->r_pos, ctx->w_pos, ctx->use_len);

	if (w_pos & (align - 1))
		align_offset = align - (w_pos & (align - 1));
	align_r_pos = (r_pos & ~(align - 1));
	if (r_pos > w_pos) {
		if (min_size + w_pos + align_offset >= r_pos)
			return MPP_NOK;
		align_w_pos = w_pos + align_offset;
		buf->start_offset = align_w_pos;
		buf->buf_start = ctx->buf_base + buf->start_offset;
		buf->r_pos = align_r_pos;
		buf->buf = ctx->buf;
		buf->size = r_pos - w_pos - align_offset;
		ctx->min_buf_size = DEFAULT_MIN_ZISE;
		return MPP_OK;
	}

	/* w_pos >= r_pos*/
	if (ctx->len - (w_pos + align_offset - r_pos) > min_size) {
		align_w_pos = w_pos + align_offset;
		buf->start_offset = align_w_pos;
		buf->buf_start = ctx->buf_base + buf->start_offset;
		buf->size = ctx->len - align_w_pos + r_pos;
		buf->r_pos = align_r_pos;
		buf->buf = ctx->buf;
		ctx->min_buf_size = DEFAULT_MIN_ZISE;
		return MPP_OK;
	}

	return MPP_NOK;
}

MPP_RET ring_buf_update_min_size(ring_buf_pool *ctx, RK_U32 min_size)
{
	if (!ctx)
		return MPP_NOK;

	ctx->min_buf_size = min_size;

	return MPP_OK;
}

MPP_RET mpp_ring_buf_flush(ring_buf *buf, RK_U8 for_cpu)
{
	MppBuffer buffer = buf->buf;
	RK_U32 offset = buf->start_offset;
	RK_U32 len, size;
	MPP_RET ret = MPP_OK;

	if (!buffer) {
		mpp_err_f("invalid NULL input\n");
		return MPP_ERR_UNKNOW;
	}
	size = mpp_buffer_get_size(buffer);
	if (buf->start_offset + buf->use_len >= size) {
		len = buf->start_offset + buf->use_len - size;
		if (for_cpu) {
			ret = mpp_buffer_flush_for_cpu_partial(buffer, offset, size - offset);
			ret |= mpp_buffer_flush_for_cpu_partial(buffer, 0, len);
		} else {
			ret = mpp_buffer_flush_for_device_partial(buffer, offset, size - offset);
			ret |= mpp_buffer_flush_for_device_partial(buffer, 0, len);
		}
	} else {
		len = buf->use_len;
		if (for_cpu)
			ret = mpp_buffer_flush_for_cpu_partial(buffer, offset, len);
		else
			ret = mpp_buffer_flush_for_device_partial(buffer, offset, len);
	}

	return ret;
}
