/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_IMPL_H__
#define __KMPP_OBJ_IMPL_H__

#include "kmpp_obj.h"

rk_s32 kmpp_obj_impl_set_s32(KmppLocTbl *tbl, void *cfg, rk_s32 val);
rk_s32 kmpp_obj_impl_get_s32(KmppLocTbl *tbl, void *cfg, rk_s32 *val);
rk_s32 kmpp_obj_impl_set_u32(KmppLocTbl *tbl, void *cfg, rk_u32 val);
rk_s32 kmpp_obj_impl_get_u32(KmppLocTbl *tbl, void *cfg, rk_u32 *val);
rk_s32 kmpp_obj_impl_set_s64(KmppLocTbl *tbl, void *cfg, rk_s64 val);
rk_s32 kmpp_obj_impl_get_s64(KmppLocTbl *tbl, void *cfg, rk_s64 *val);
rk_s32 kmpp_obj_impl_set_u64(KmppLocTbl *tbl, void *cfg, rk_u64 val);
rk_s32 kmpp_obj_impl_get_u64(KmppLocTbl *tbl, void *cfg, rk_u64 *val);
rk_s32 kmpp_obj_impl_set_kobj(KmppLocTbl *tbl, void *cfg, KmppObj val);
rk_s32 kmpp_obj_impl_get_kobj(KmppLocTbl *tbl, void *cfg, KmppObj *val);
rk_s32 kmpp_obj_impl_set_kptr(KmppLocTbl *tbl, void *cfg, void *val);
rk_s32 kmpp_obj_impl_get_kptr(KmppLocTbl *tbl, void *cfg, void **val);
rk_s32 kmpp_obj_impl_set_kfp(KmppLocTbl *tbl, void *cfg, void *val);
rk_s32 kmpp_obj_impl_get_kfp(KmppLocTbl *tbl, void *cfg, void **val);
rk_s32 kmpp_obj_impl_set_uobj(KmppLocTbl *tbl, void *cfg, rk_u64 val);
rk_s32 kmpp_obj_impl_get_uobj(KmppLocTbl *tbl, void *cfg, rk_u64 *val);
rk_s32 kmpp_obj_impl_set_uptr(KmppLocTbl *tbl, void *cfg, rk_u64 val);
rk_s32 kmpp_obj_impl_get_uptr(KmppLocTbl *tbl, void *cfg, rk_u64 *val);
rk_s32 kmpp_obj_impl_set_ufp(KmppLocTbl *tbl, void *cfg, rk_u64 val);
rk_s32 kmpp_obj_impl_get_ufp(KmppLocTbl *tbl, void *cfg, rk_u64 *val);
rk_s32 kmpp_obj_impl_set_shm(KmppLocTbl *tbl, void *cfg, KmppShmPtr *val);
rk_s32 kmpp_obj_impl_get_shm(KmppLocTbl *tbl, void *cfg, KmppShmPtr *val);

/* internal userspace access function */
rk_s32 kmpp_obj_set_uobj(KmppObj obj, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_obj_get_uobj(KmppObj obj, const rk_u8 *name, rk_u64 *val);
rk_s32 kmpp_obj_set_uptr(KmppObj obj, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_obj_get_uptr(KmppObj obj, const rk_u8 *name, rk_u64 *val);
rk_s32 kmpp_obj_set_ufp(KmppObj obj, const rk_u8 *name, rk_u64 val);
rk_s32 kmpp_obj_get_ufp(KmppObj obj, const rk_u8 *name, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_uobj(KmppObj obj, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_uobj(KmppObj obj, KmppLocTbl *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_uptr(KmppObj obj, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_uptr(KmppObj obj, KmppLocTbl *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_ufp(KmppObj obj, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_ufp(KmppObj obj, KmppLocTbl *tbl, rk_u64 *val);

#endif /* __KMPP_OBJ_IMPL_H__ */