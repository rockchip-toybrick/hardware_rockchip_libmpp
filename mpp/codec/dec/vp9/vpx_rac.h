/*
 * Copyright (C) 2016 The FFmpeg project
 * Copyright (c) 2016 Rockchip Electronics Co., Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef VPX_RAC_H
#define VPX_RAC_H

#include <stdint.h>
#include <stdlib.h>

#include "mpp_common.h"

#define DECLARE_ALIGNED(n,t,v)      t v

typedef struct Vpxmv {
    DECLARE_ALIGNED(4, int16_t, x);
    int16_t y;
} Vpxmv;

typedef struct VpxRangeCoder {
    int high;
    int bits; /* stored negated (i.e. negative "bits" is a positive number of
                 bits left) in order to eliminate a negate in cache refilling */
    const uint8_t *buffer;
    const uint8_t *end;
    unsigned int code_word;
} VpxRangeCoder;

/**
 * vp56 specific range coder implementation
 */

extern const uint8_t vpx_norm_shift[256];
void vpx_init_range_decoder(VpxRangeCoder *c, const uint8_t *buf, int buf_size);
unsigned int vpx_rac_renorm(VpxRangeCoder *c);
int vpx_rac_get_prob(VpxRangeCoder *c, uint8_t prob);
int vpx_rac_get_prob_branchy(VpxRangeCoder *c, int prob);
// rounding is different than vpx_rac_get, is vpx_rac_get wrong?
int vpx_rac_get(VpxRangeCoder *c);
int vpx_rac_get_uint(VpxRangeCoder *c, int bits);

#endif /* VPX_RAC_H */
