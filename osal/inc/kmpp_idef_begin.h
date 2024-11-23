/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#if defined(KMPP_IDEF_BEGIN_EXIST)
#error "kmpp_idef_begin.h already included"
#endif

#define KMPP_IDEF_BEGIN_EXIST

#if defined(KMPP_OBJ_FUNC_PREFIX) && defined(KMPP_OBJ_INTF_TYPE)

#define KMPP_OBJ_FUNC(x, y, z)  x##y##z

#define ENTRY_TO_DECLARE(ftype, type, f1) \
    rk_s32 KMPP_OBJ_FUNC(KMPP_OBJ_FUNC_PREFIX, _set_, f1)(KMPP_OBJ_INTF_TYPE p, type  val); \
    rk_s32 KMPP_OBJ_FUNC(KMPP_OBJ_FUNC_PREFIX, _get_, f1)(KMPP_OBJ_INTF_TYPE p, type* val);

#else

#define ENTRY_TO_DECLARE(ftype, type, f1)

#error "use kmpp_idef_begin.h need macro KMPP_OBJ_FUNC_PREFIX and KMPP_OBJ_INTF_TYPE as input"

#endif