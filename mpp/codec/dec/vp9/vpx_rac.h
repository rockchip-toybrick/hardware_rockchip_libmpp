/*
*
* Copyright 2015 Rockchip Electronics Co. LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

/**
 * VPX RangeCoder dec (common features)
 */

#ifndef VPX_RAC_H
#define VPX_RAC_H

#include <stdint.h>
#include <stdlib.h>

#include "mpp_common.h"

#define DECLARE_ALIGNED(n,t,v)      t v

typedef struct Vpxmv {
    DECLARE_ALIGNED(4, int16_t, x);
    rk_s16 y;
} Vpxmv;

typedef struct VpxRangeCoder {
    rk_s32 high;
    rk_s32 bits; /* stored negated (i.e. negative "bits" is a positive number of
                 bits left) in order to eliminate a negate in cache refilling */
    const rk_u8 *buffer;
    const rk_u8 *end;
    rk_u32 code_word;
} VpxRangeCoder;

/**
 * vp56 specific range coder implementation
 */

extern const uint8_t vpx_norm_shift[256];
void vpx_init_range_decoder(VpxRangeCoder *c, const uint8_t *buf, int buf_size);
rk_u32 vpx_rac_renorm(VpxRangeCoder *c);
rk_s32 vpx_rac_get_prob(VpxRangeCoder *c, uint8_t prob);
rk_s32 vpx_rac_get_prob_branchy(VpxRangeCoder *c, int prob);
// rounding is different than vpx_rac_get, is vpx_rac_get wrong?
rk_s32 vpx_rac_get(VpxRangeCoder *c);
rk_s32 vpx_rac_get_uint(VpxRangeCoder *c, int bits);

#endif /* VPX_RAC_H */
