/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME must be defined on using kmpp_idef_begin.h"
#endif

#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE must be defined on using kmpp_idef_begin.h"
#endif

#ifndef KMPP_OBJ_ENTRY_TABLE
#error "KMPP_OBJ_ENTRY_TABLE must be defined on using kmpp_idef_begin.h"
#endif

#include "kmpp_shm.h"

#ifndef KMPP_OBJ_CONCAT2
#define KMPP_OBJ_CONCAT2(x, y)      x##_##y
#endif
#ifndef KMPP_OBJ_CONCAT3
#define KMPP_OBJ_CONCAT3(x, y, z)   x##_##y##_##z
#endif

#define ENTRY_TO_DECLARE(prefix, ftype, type, f1) \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, set, f1)(KMPP_OBJ_INTF_TYPE p, type  val); \
    rk_s32 KMPP_OBJ_CONCAT3(prefix, get, f1)(KMPP_OBJ_INTF_TYPE p, type* val);

#define KMPP_OBJ_FUNC_DEFINE(prefix) \
rk_s32 KMPP_OBJ_CONCAT2(prefix, init)(void); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, deinit)(void); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, get)(KMPP_OBJ_INTF_TYPE *p); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, get_share)(KMPP_OBJ_INTF_TYPE *p, KmppShmGrp grp); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, assign)(KMPP_OBJ_INTF_TYPE *p, void *buf, rk_s32 size); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, put)(KMPP_OBJ_INTF_TYPE p); \
rk_s32 KMPP_OBJ_CONCAT2(prefix, dump)(KMPP_OBJ_INTF_TYPE p, const rk_u8 *caller);

KMPP_OBJ_FUNC_DEFINE(KMPP_OBJ_NAME)
KMPP_OBJ_ENTRY_TABLE(ENTRY_TO_DECLARE, KMPP_OBJ_NAME)

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_ENTRY_TABLE
#undef KMPP_OBJ_FUNC_DEFINE
#undef ENTRY_TO_DECLARE
