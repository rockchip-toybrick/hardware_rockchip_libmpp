/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_buffer"

#include <asm/ioctl.h>

#include "rk_list.h"
#include "kmpp_atomic.h"
#include "kmpp_cls.h"
#include "kmpp_env.h"
#include "kmpp_log.h"
#include "kmpp_macro.h"
#include "kmpp_mem.h"
#include "kmpp_spinlock.h"
#include "kmpp_string.h"
#include "kmpp_uaccess.h"

#include "kmpp_obj.h"
#include "kmpp_shm.h"
#include "kmpp_mem_pool.h"
#include "kmpp_buffer_impl.h"

#define BUF_DBG_FLOW                    (0x00000001)
#define BUF_DBG_INFO                    (0x00000002)
#define BUF_DBG_FILE                    (0x00000004)

#define buf_dbg(flag, fmt, ...)         kmpp_dbg(kmpp_buf_debug, flag, fmt, ## __VA_ARGS__)

#define buf_dbg_flow(fmt, ...)          buf_dbg(BUF_DBG_FLOW, fmt, ## __VA_ARGS__)
#define buf_dbg_info(fmt, ...)          buf_dbg(BUF_DBG_INFO, fmt, ## __VA_ARGS__)
#define buf_dbg_flow_info(fmt, ...)     buf_dbg(BUF_DBG_FLOW | BUF_DBG_INFO, fmt, ## __VA_ARGS__)
#define buf_dbg_file(fmt, ...)          buf_dbg(BUF_DBG_FILE, fmt, ## __VA_ARGS__)

#define get_buf_srv(caller) \
    ({ \
        KmppBufferSrv *__tmp; \
        if (srv_buf) { \
            __tmp = srv_buf; \
        } else { \
            kmpp_err_f("kmpp buf srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

#define get_buf_srv_f() get_buf_srv(__FUNCTION__)

typedef struct KmppBufferSrv_t {
    osal_spinlock       *lock;

    /* /dev/kmpp_buf device info */
    osal_cls            *cls;
    osal_dev_fops       fops;
    osal_fs_dev_mgr     *mgr;
    KmppEnvNode         env;

    osal_list_head      list_grp;
    osal_list_head      list_buf;

    rk_s32              grp_id;
    rk_s32              grp_cnt;
    rk_s32              buf_id;
    rk_s32              buf_cnt;

    /* default buffer group */
    KmppBufGrp          group_default;
    KmppBufGrpImpl      *grp;
} KmppBufferSrv;

static KmppBufferSrv *srv_buf = NULL;
static rk_u32 kmpp_buf_debug = 0;

rk_s32 kmpp_buf_init(void)
{
    KmppBufferSrv *srv = srv_buf;
    rk_s32 lock_size;
    rk_s32 size;

    if (srv)
        return rk_nok;

    lock_size = osal_spinlock_size();
    size = sizeof(KmppBufferSrv) + lock_size;
    srv = kmpp_calloc_atomic(size);
    if (!srv) {
        kmpp_err_f("srv_buf alloc size %d failed\n", size);
        return rk_nok;
    }

    osal_spinlock_assign(&srv->lock, (void *)(srv + 1), lock_size);

    OSAL_INIT_LIST_HEAD(&srv->list_grp);
    OSAL_INIT_LIST_HEAD(&srv->list_buf);

    kmpp_buf_grp_init();
    kmpp_buf_grp_cfg_init();
    kmpp_buffer_init();
    kmpp_buf_cfg_init();

    {
        KmppEnvInfo env;

        env.type = KmppEnv_u32;
        env.readonly = 0;
        env.name = "kmpp_buf_debug";
        env.val = &kmpp_buf_debug;
        env.env_show = NULL;

        kmpp_env_add(kmpp_env_debug, &srv->env, &env);
    }

    srv_buf = srv;

    {   /* setup the default buffer group */
        KmppBufGrp group = NULL;

        kmpp_buf_grp_get(&group);
        kmpp_buf_grp_setup_f(group);

        srv->group_default = group;
        srv->grp = kmpp_obj_to_entry(group);
    }

    return rk_ok;
}

rk_s32 kmpp_buf_deinit(void)
{
    KmppBufferSrv *srv = get_buf_srv_f();
    KmppBufGrpImpl *grp, *n;

    if (!srv)
        return rk_nok;

    /* remove default group from the list */
    if (srv->group_default)
        osal_list_del_init(&srv->grp->list_srv);

    osal_list_for_each_entry_safe(grp, n, &srv->list_grp, KmppBufGrpImpl, list_srv) {
        kmpp_buf_grp_put(grp);
    }

    if (srv->group_default)
        kmpp_buf_grp_put(srv->group_default);

    srv->group_default = NULL;
    srv->grp = NULL;

    osal_spinlock_deinit(&srv->lock);

    kmpp_buffer_deinit();
    kmpp_buf_cfg_deinit();
    kmpp_buf_grp_deinit();
    kmpp_buf_grp_cfg_deinit();

    kmpp_env_del(kmpp_env_debug, srv->env);

    srv_buf = NULL;
    kmpp_free(srv);

    return rk_nok;
}

static rk_s32 kmpp_buf_grp_cfg_impl_dump(void *entry)
{
    KmppBufGrpCfgImpl *impl = (KmppBufGrpCfgImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("flag             %d\n", impl->flag);
    kmpp_logi("count            %d\n", impl->count);
    kmpp_logi("size             %d\n", impl->size);
    kmpp_logi("mode             %d\n", impl->mode);
    kmpp_logi("device           %#px\n", impl->device);
    kmpp_logi("allocator [u:k]  %#llx : %#llx\n", impl->allocator.uaddr, impl->allocator.kaddr);
    kmpp_logi("name [u:k]       %#llx : %#llx\n", impl->name.uaddr, impl->name.kaddr);

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_buf_grp_cfg
#define KMPP_OBJ_INTF_TYPE          KmppBufGrpCfg
#define KMPP_OBJ_IMPL_TYPE          KmppBufGrpCfgImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_BUF_GRP_CFG_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE       KMPP_BUF_GRP_CFG_STRUCT_TABLE
#define KMPP_OBJ_FUNC_DUMP          kmpp_buf_grp_cfg_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"

static void kmpp_buf_grp_cfg_sync(KmppBufGrpImpl *impl)
{
    impl->cfg_int.grp_id    = impl->grp_id;
    impl->cfg_int.used      = impl->count_used;
    impl->cfg_int.unused    = impl->count_unused;

    impl->cfg_usr->grp_id   = impl->grp_id;
    impl->cfg_usr->used     = impl->count_used;
    impl->cfg_usr->unused   = impl->count_unused;
}

static rk_s32 kmpp_buf_grp_impl_init(void *entry, osal_fs_dev *file, const rk_u8 *caller)
{
    KmppBufGrpImpl *impl = (KmppBufGrpImpl *)entry;
    KmppBufferSrv *srv = get_buf_srv(caller);

    buf_dbg_flow("buf grp %px get enter at %s\n", impl, caller);

    if (!srv)
        return rk_nok;

    OSAL_INIT_LIST_HEAD(&impl->list_srv);
    OSAL_INIT_LIST_HEAD(&impl->list_used);
    OSAL_INIT_LIST_HEAD(&impl->list_unused);
    osal_spinlock_assign(&impl->lock, (void *)(impl + 1), osal_spinlock_size());

    if (file) {
        kmpp_buf_grp_cfg_get_share(&impl->cfg_ext, file);
    } else {
        kmpp_buf_grp_cfg_get(&impl->cfg_ext);
    }

    if (!impl->cfg_ext) {
        kmpp_loge_f("failed to create buffer group config for %px at %s\n", impl, caller);
        return rk_nok;
    }

    impl->cfg_usr = kmpp_obj_to_entry(impl->cfg_ext);
    impl->cfg_usr->grp_impl = impl;
    impl->cfg_int.grp_impl = impl;
    kmpp_obj_to_shmptr(impl->cfg_ext, &impl->cfg);

    osal_spin_lock(srv->lock);
    osal_list_add_tail(&impl->list_srv, &srv->list_grp);
    impl->grp_id = srv->grp_id++;
    srv->grp_cnt++;
    buf_dbg_info("buf grp %d init total %d\n", impl->grp_id, srv->grp_cnt);
    osal_spin_unlock(srv->lock);

    buf_dbg_flow("buf grp %px - %d get leave at %s\n", impl, impl->grp_id, caller);

    return rk_ok;
}

static rk_s32 kmpp_buf_grp_impl_deinit(void *entry, const rk_u8 *caller)
{
    KmppBufGrpImpl *impl = (KmppBufGrpImpl *)entry;
    KmppBufferSrv *srv = get_buf_srv(caller);

    buf_dbg_flow("buf grp %d put enter at %s\n", impl->grp_id, caller);

    if (!srv)
        return rk_nok;

    osal_spin_lock(srv->lock);
    if (!osal_list_empty(&impl->list_srv)) {
        osal_list_del_init(&impl->list_srv);
        srv->grp_cnt--;
    }
    buf_dbg_info("buf grp %d deinit total %d\n", impl->grp_id, srv->grp_cnt);
    osal_spin_unlock(srv->lock);

    /* TODO: check buffer list here */

    if (impl->heap) {
        kmpp_dmaheap_put(impl->heap, caller);
        impl->heap = NULL;
    }

    if (impl->cfg_ext) {
        kmpp_buf_grp_cfg_put(impl->cfg_ext);
        impl->cfg_ext = NULL;
    }

    buf_dbg_flow("buf grp %d put leave at %s\n", impl->grp_id, caller);

    return rk_ok;
}

static rk_s32 kmpp_buf_grp_impl_dump(void *entry)
{
    KmppBufGrpImpl *impl = (KmppBufGrpImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("name             %s\n", impl->str_buf + impl->name_offset);
    if (impl->name_offset)
        kmpp_logi("allocator        %s\n", impl->str_buf);
    kmpp_logi("grp id           %d\n", impl->grp_id);

    return kmpp_buf_grp_cfg_impl_dump(&impl->cfg_int);
}

KmppShmPtr *kmpp_buf_grp_get_cfg_s(KmppBufGrp group)
{
    KmppBufGrpImpl *impl = kmpp_obj_to_entry(group);

    return impl ? &impl->cfg : NULL;
}

KmppBufGrpCfg kmpp_buf_grp_get_cfg_k(KmppBufGrp group)
{
    KmppBufGrpImpl *impl = kmpp_obj_to_entry(group);

    return impl ? impl->cfg.kptr : NULL;
}

rk_u64 kmpp_buf_grp_get_cfg_u(KmppBufGrp group)
{
    KmppBufGrpImpl *impl = kmpp_obj_to_entry(group);

    return impl ? impl->cfg.uaddr : 0;
}

rk_s32 kmpp_buf_grp_get_size(KmppBufGrpImpl *grp)
{
    return grp->cfg_int.size;
}

rk_s32 kmpp_buf_grp_get_count(KmppBufGrpImpl *grp)
{
    return grp->cfg_int.count;
}

rk_s32 kmpp_buf_grp_setup(KmppBufGrp group, const rk_u8 *caller)
{
    KmppBufferSrv *srv = get_buf_srv(caller);
    KmppBufGrpImpl *impl;
    KmppBufGrpCfgImpl *cfg;
    void *device;
    rk_s32 offset;

    if (!srv)
        return rk_nok;

    if (!group) {
        kmpp_loge_f("invalid param group %px caller %s\n", group, caller);
        return rk_nok;
    }

    impl = (KmppBufGrpImpl *)kmpp_obj_to_entry(group);

    buf_dbg_flow("buf grp %d setup enter at %s\n", impl->grp_id, caller);

    cfg = impl->cfg_usr;
    /* setup cfg_set parameters here */
    if (!cfg) {
        kmpp_loge_f("invalid param cfg_set %px caller %s\n", srv, cfg, caller);
        return rk_nok;
    }

    device = cfg->device;
    /* copy allocator name */
    offset = osal_strncpy_sptr(impl->str_buf, &cfg->allocator, BUF_GRP_STR_BUF_SIZE);

    /* copy buffer group name */
    if (offset) {
        impl->name_offset = offset + 1;
        impl->str_buf[offset] = '\0';

        osal_strncpy_sptr(impl->str_buf + offset + 1, &cfg->name,
                                   BUF_GRP_STR_BUF_SIZE - offset - 1);
        buf_dbg_info("allocator %s - name %s\n", impl->str_buf, impl->str_buf + offset + 1);
    } else {
        impl->name_offset = 0;
        offset = osal_strncpy_sptr(impl->str_buf, &cfg->name,
                                   BUF_GRP_STR_BUF_SIZE - 1);
        buf_dbg_info("allocator na - name %s\n", impl->str_buf);
    }

    /*
     * three type of allocator:
     * name valid               - open heap by name
     * name NULL device NULL    - default dmaheap
     * name NULL device valid   - use device to allocate dmabuf in kmpp_dmabuf.c
     */
    if (impl->name_offset) {
        kmpp_dmaheap_get_by_name(&impl->heap, impl->str_buf, cfg->flag, caller);
        if (!impl->heap)
            kmpp_err_f("open heap %s failed\n", impl->str_buf);
    }

    if (!impl->heap) {
        if (!device) {
            kmpp_dmaheap_get(&impl->heap, cfg->flag, caller);
            if (!impl->heap)
                kmpp_err_f("open default heap failed\n");
        } else {
            kmpp_logi_f("use device %px %s to allocate dmabuf\n",
                        device, osal_device_name(device));
        }
    }

    if (!impl->heap && !device) {
        kmpp_err_f("open heap %s device %px failed\n", impl->str_buf, device);
        return rk_nok;
    }

    osal_memcpy(&impl->cfg_int, cfg, sizeof(impl->cfg_int));

    kmpp_buf_grp_cfg_sync(impl);

    buf_dbg_info("buf grp %d setup mode %d size %d count %d\n", impl->grp_id,
                 impl->cfg_int.mode, impl->cfg_int.size, impl->cfg_int.count);

    buf_dbg_flow("buf grp %d setup leave at %s\n", impl->grp_id, caller);

    return rk_ok;
}

rk_s32 kmpp_buf_grp_reset(KmppBufGrp group, const rk_u8 *caller)
{
    KmppBufferSrv *srv = get_buf_srv(caller);
    KmppBufGrpImpl *impl;
    rk_s32 ret = rk_nok;

    if (!srv)
        return ret;

    if (!group) {
        kmpp_loge_f("invalid param group %px caller %s\n", group, caller);
        return ret;
    }

    impl = (KmppBufGrpImpl *)kmpp_obj_to_entry(group);

    buf_dbg_flow("buf grp %d reset enter at %s\n", impl->grp_id, caller);

    buf_dbg_flow("buf grp %d reset leave at %s\n", impl->grp_id, caller);

    return ret;
}

static rk_s32 kmpp_buf_grp_ioc_setup(osal_fs_dev *file, KmppShmPtr *in, KmppShmPtr *out)
{
    KmppShm grp = NULL;
    rk_s32 ret = rk_nok;

    buf_dbg_flow("enter\n");

    if (!in) {
        kmpp_loge_f("invalid param in %px\n", in);
        goto done;
    }

    grp = kmpp_shm_get_kpriv(in->kptr);
    ret = kmpp_obj_check_f(grp);
    if (ret) {
        kmpp_loge_f("grp setup invalid KmppBufGrp %px\n", grp);
        goto done;
    }

    ret = kmpp_buf_grp_setup_f(grp);
    if (ret)
        kmpp_loge_f("grp setup buf grp %px failed ret %d\n", grp, ret);

done:
    buf_dbg_flow("leave ret %d\n", ret);

    return ret;
}

static rk_s32 kmpp_buf_grp_ioc_reset(osal_fs_dev *file, KmppShmPtr *in, KmppShmPtr *out)
{
    KmppShm grp = NULL;
    rk_s32 ret = rk_nok;

    buf_dbg_flow("enter\n");

    if (!in) {
        kmpp_loge_f("invalid param in %px\n", in);
        goto done;
    }

    grp = kmpp_shm_get_kpriv(in->kptr);
    ret = kmpp_obj_check_f(grp);
    if (ret) {
        kmpp_loge_f("grp reset invalid KmppBufGrp %px\n", grp);
        goto done;
    }

    ret = kmpp_buf_grp_reset_f(grp);
    if (ret)
        kmpp_loge_f("grp reset buf grp %px failed ret %d\n", grp, ret);

done:
    buf_dbg_flow("leave ret %d\n", ret);

    return ret;
}

static KmppObjIoctls kmpp_buf_grp_ioctls = {
    .count = 2,
    .funcs = {
        [0] = {
            .cmd = 0,
            .func = kmpp_buf_grp_ioc_setup,
        },
        [1] = {
            .cmd = 1,
            .func = kmpp_buf_grp_ioc_reset,
        },
    },
};

#define KMPP_OBJ_NAME               kmpp_buf_grp
#define KMPP_OBJ_INTF_TYPE          KmppBufGrp
#define KMPP_OBJ_IMPL_TYPE          KmppBufGrpImpl
#define KMPP_OBJ_EXTRA_SIZE         osal_spinlock_size()
#define KMPP_OBJ_STRUCT_TABLE       KMPP_BUF_GRP_STRUCT_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_buf_grp_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_buf_grp_impl_deinit
#define KMPP_OBJ_FUNC_IOCTL         kmpp_buf_grp_ioctls
#define KMPP_OBJ_FUNC_DUMP          kmpp_buf_grp_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"

static void kmpp_buf_cfg_sync(KmppBufferImpl *impl)
{
    /* update to both internal and external config for user access */
    impl->cfg_int.sptr.uaddr    = impl->uaddr;
    impl->cfg_int.sptr.kptr     = impl->kptr;
    impl->cfg_int.size          = impl->size;
    impl->cfg_int.grp_id        = impl->grp_id;
    impl->cfg_int.buf_gid       = impl->buf_gid;
    impl->cfg_int.buf_uid       = impl->buf_uid;

    impl->cfg_usr->sptr.uaddr   = impl->uaddr;
    impl->cfg_usr->sptr.kptr    = impl->kptr;
    impl->cfg_usr->size         = impl->size;
    impl->cfg_usr->grp_id       = impl->grp_id;
    impl->cfg_usr->buf_gid      = impl->buf_gid;
    impl->cfg_usr->buf_uid      = impl->buf_uid;
}

rk_s32 kmpp_buffer_impl_init(void *entry, osal_fs_dev *file, const rk_u8 *caller)
{
    KmppBufferSrv *srv = get_buf_srv(caller);
    KmppBufferImpl *impl = (KmppBufferImpl *)entry;

    buf_dbg_flow("buf %px get enter at %s\n", impl, caller);

    if (!srv)
        return rk_nok;

    OSAL_INIT_LIST_HEAD(&impl->list_status);
    OSAL_INIT_LIST_HEAD(&impl->list_maps);

    if (file) {
        kmpp_buf_cfg_get_share(&impl->cfg_ext, file);
    } else {
        kmpp_buf_cfg_get(&impl->cfg_ext);
    }

    if (!impl->cfg_ext) {
        kmpp_loge_f("failed to create buffer config for %px at %s\n", impl, caller);
        return rk_nok;
    }

    impl->cfg_usr = kmpp_obj_to_entry(impl->cfg_ext);
    impl->cfg_usr->buf_impl = impl;
    impl->cfg_int.buf_impl = impl;
    kmpp_obj_to_shmptr(impl->cfg_ext, &impl->cfg);

    impl->ref_cnt = 1;

    osal_spin_lock(srv->lock);
    osal_list_add_tail(&impl->list_status, &srv->list_buf);
    impl->buf_uid = srv->buf_id++;
    srv->buf_cnt++;
    buf_dbg_info("buf %d init total %d\n", impl->buf_uid, srv->buf_cnt);
    osal_spin_unlock(srv->lock);

    buf_dbg_flow("buf %px - %d get leave at %s\n", impl, impl->buf_uid, caller);

    return rk_ok;
}

rk_s32 kmpp_buffer_impl_deinit(void *entry, const rk_u8 *caller)
{
    KmppBufferSrv *srv = get_buf_srv(caller);
    KmppBufferImpl *impl = (KmppBufferImpl *)entry;
    rk_s32 ref_cnt;
    rk_s32 old;

    if (!impl) {
        kmpp_loge_f("invalid param buffer %px at %s\n", impl, caller);
        return rk_nok;
    }

    old = impl->ref_cnt;
    ref_cnt = KMPP_SUB_FETCH(&impl->ref_cnt, 1);

    buf_dbg_flow("buf %d dec ref %d -> %d at %s\n",
                 impl->buf_uid, old, ref_cnt, caller);

    if (ref_cnt < 0) {
        kmpp_loge_f("invalid negative ref_cnt %d buffer %px at %s\n",
                    ref_cnt, impl, caller);
        return rk_nok;
    }

    if (ref_cnt > 0)
        return rk_nok;

    if (impl->grp) {
        KmppBufGrpImpl *grp = impl->grp;

        osal_spin_lock(grp->lock);
        osal_list_del_init(&impl->list_status);
        grp->buf_cnt--;
        grp->count_used--;
        kmpp_buf_grp_cfg_sync(grp);
        impl->grp = NULL;
        osal_spin_unlock(grp->lock);
    }

    osal_spin_lock(srv->lock);
    if (!osal_list_empty(&impl->list_status)) {
        osal_list_del_init(&impl->list_status);
        srv->buf_cnt--;
    }
    buf_dbg_info("buf %d init total %d\n", impl->buf_uid, srv->buf_cnt);
    osal_spin_unlock(srv->lock);

    if (impl->cfg_ext)
        kmpp_buf_cfg_put(impl->cfg_ext);

    impl->cfg_ext = NULL;
    impl->cfg_usr = NULL;

    if (impl->buf) {
        kmpp_dmabuf_free(impl->buf, caller);
        impl->buf = NULL;
    }

    buf_dbg_flow("buf %d put leave at %s\n", impl->buf_uid, caller);

    return rk_ok;
}

rk_s32 kmpp_buffer_inc_ref(KmppBuffer buffer, const rk_u8 *caller)
{
    KmppBufferImpl *impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);
    rk_s32 old;

    if (!impl) {
        kmpp_loge_f("invalid param buffer %px at %s\n", buffer, caller);
        return rk_nok;
    }

    old = KMPP_FETCH_ADD(&impl->ref_cnt, 1);

    buf_dbg_flow_info("buf %d inc ref %d -> %d at %s\n",
                      impl->buf_uid, old, impl->ref_cnt, caller);

    return rk_ok;
}

rk_s32 kmpp_buffer_setup(KmppBuffer buffer, osal_fs_dev *file, const rk_u8 *caller)
{
    KmppBufferSrv *srv = get_buf_srv(caller);
    KmppBufferImpl *impl;
    KmppBufCfgImpl *buf_cfg;
    KmppBufGrpImpl *grp;
    KmppDmaBuf dmabuf;
    KmppObj obj;
    rk_s32 size;

    if (!buffer) {
        kmpp_loge_f("invalid param buffer %px at %s\n", buffer, caller);
        return rk_nok;
    }

    impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);

    buf_dbg_flow("buf %d setup enter at %s\n", impl->buf_uid, caller);

    buf_cfg = impl->cfg_usr;
    if (kmpp_obj_from_shmptr(&obj, &buf_cfg->group)) {
        kmpp_loge_f("invalid buffer group [u:k] %#llx:%#llx at %s\n",
                    buf_cfg->group.uaddr, buf_cfg->group.kaddr, caller);
        return rk_nok;
    }

    if (kmpp_obj_check_f(obj)) {
        kmpp_loge_f("invalid buffer group %px object check at %s\n", obj, caller);
        return rk_nok;
    }

    grp = (KmppBufGrpImpl *)kmpp_obj_to_entry(obj);
    size = buf_cfg->size;

    if (!grp)
        grp = srv->grp;

    if (!size)
        size = kmpp_buf_grp_get_size(grp);

    if (!grp || !size) {
        kmpp_loge_f("invalid param grp %px size %d at %s\n", grp, size, caller);
        return rk_nok;
    }

    if (!grp->heap) {
        kmpp_loge_f("invalid param grp %px heap %px at %s\n", grp, grp->heap, caller);
        return rk_nok;
    }

    kmpp_dmabuf_alloc(&dmabuf, grp->heap, size, buf_cfg->flag, caller);
    if (!dmabuf) {
        kmpp_loge_f("failed to alloc dmabuf size %d at %s\n", size, caller);
        return rk_nok;
    }

    osal_memcpy(&impl->cfg_int, buf_cfg, sizeof(impl->cfg_int));
    impl->buf = dmabuf;
    impl->size = size;
    impl->kptr = kmpp_dmabuf_get_kptr(dmabuf);

    /* userspace call can get uaddr */
    if (file)
        impl->uaddr = kmpp_dmabuf_get_uptr(dmabuf);

    kmpp_buf_cfg_sync(impl);

    osal_spin_lock(srv->lock);
    osal_list_del_init(&impl->list_status);
    osal_spin_unlock(srv->lock);

    osal_spin_lock(grp->lock);
    osal_list_add_tail(&impl->list_status, &grp->list_used);
    impl->grp = grp;
    impl->grp_id = grp->grp_id;
    impl->buf_gid = grp->buf_id++;
    grp->buf_cnt++;
    grp->count_used++;
    kmpp_buf_grp_cfg_sync(grp);
    buf_dbg_info("buf %d setup to grp %d:%d size %d [u:k] %#llx:%#px\n", impl->buf_uid,
                 impl->grp_id, impl->buf_gid, size, impl->uaddr, impl->kptr);
    osal_spin_unlock(grp->lock);

    buf_dbg_flow("buf %d [%d:%d] setup leave at %s\n",
                 impl->buf_uid, impl->grp_id, impl->buf_gid, caller);

    return rk_ok;
}

rk_s32 kmpp_buffer_impl_dump(void *entry)
{
    return rk_ok;
}

KmppShmPtr *kmpp_buffer_get_cfg_s(KmppBuffer buffer)
{
    KmppBufferImpl *impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);

    return impl ? &impl->cfg : NULL;
}

KmppBufCfg kmpp_buffer_get_cfg_k(KmppBuffer buffer)
{
    KmppBufferImpl *impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);

    return impl ? impl->cfg_ext : NULL;
}

rk_u64 kmpp_buffer_get_cfg_u(KmppBuffer buffer)
{
    KmppBufferImpl *impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);

    return impl ? impl->cfg.uaddr : 0;
}

void *kmpp_buffer_get_kptr(KmppBuffer buffer)
{
    KmppBufferImpl *impl = (KmppBufferImpl *)kmpp_obj_to_entry(buffer);

    if (!impl)
        return NULL;

    if (impl->kptr)
        return impl->kptr;

    if (impl->buf)
        impl->kptr = kmpp_dmabuf_get_kptr(impl->buf);

    return impl->kptr;
}

static rk_s32 kmpp_buffer_ioc_setup(osal_fs_dev *file, KmppShmPtr *in, KmppShmPtr *out)
{
    KmppShm buf = NULL;
    rk_s32 ret = rk_nok;

    buf_dbg_flow("enter file %px in %px out %px\n", file, in, out);

    if (!in) {
        kmpp_loge_f("invalid param in %px\n", in);
        goto done;
    }

    buf = kmpp_shm_get_kpriv(in->kptr);
    ret = kmpp_obj_check_f(buf);
    if (ret) {
        kmpp_loge_f("buf setup invalid KmppBuffer %px\n", buf);
        goto done;
    }

    ret = kmpp_buffer_setup_f(buf, file);
    if (ret)
        kmpp_loge_f("buf setup buffer %px failed ret %d\n", buf, ret);

done:
    buf_dbg_flow("leave ret %d\n", ret);

    return ret;
}

static KmppObjIoctls kmpp_buffer_ioctls = {
    .count = 1,
    .funcs = {
        [0] = {
            .cmd = 0,
            .func = kmpp_buffer_ioc_setup,
        },
    },
};

#define KMPP_OBJ_NAME               kmpp_buffer
#define KMPP_OBJ_INTF_TYPE          KmppBuffer
#define KMPP_OBJ_IMPL_TYPE          KmppBufferImpl
#define KMPP_OBJ_STRUCT_TABLE       KMPP_BUFFER_STRUCT_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_buffer_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_buffer_impl_deinit
#define KMPP_OBJ_FUNC_IOCTL         kmpp_buffer_ioctls
#define KMPP_OBJ_FUNC_DUMP          kmpp_buffer_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"


static rk_s32 kmpp_buf_cfg_impl_dump(void *entry)
{
    KmppBufCfgImpl *impl = (KmppBufCfgImpl*)entry;

    if (!impl) {
        kmpp_loge_f("invalid param frame NULL\n");
        return rk_nok;
    }

    kmpp_logi("addr [u:k]       %#llx:%#px\n", impl->sptr.uaddr, impl->sptr.kptr);
    kmpp_logi("size             %d\n", impl->size);
    kmpp_logi("offset           %d\n", impl->offset);
    kmpp_logi("flag             %d\n", impl->flag);
    kmpp_logi("fd               %d\n", impl->fd);
    kmpp_logi("index            %d\n", impl->index);

    return rk_ok;
}

#define KMPP_OBJ_NAME               kmpp_buf_cfg
#define KMPP_OBJ_INTF_TYPE          KmppBufCfg
#define KMPP_OBJ_IMPL_TYPE          KmppBufCfgImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_BUF_CFG_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE       KMPP_BUF_CFG_STRUCT_TABLE
#define KMPP_OBJ_FUNC_DUMP          kmpp_buf_cfg_impl_dump
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"


rk_s32 kmpp_buffer_read(KmppBuffer buffer, rk_u32 offset, void *data, rk_s32 size, const rk_u8 *caller)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_write(KmppBuffer buffer, rk_u32 offset, void *data, rk_s32 size, const rk_u8 *caller)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_get_iova(KmppBuffer buffer, rk_u64 *iova, osal_dev *dev)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_put_iova(KmppBuffer buffer, rk_u64 iova, osal_dev *dev)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_get_iova_by_device(KmppBuffer buffer, rk_u64 *iova, void *device)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_put_iova_by_device(KmppBuffer buffer, rk_u64 iova, void *device)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_flush_for_cpu(KmppBuffer buffer)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_flush_for_dev(KmppBuffer buffer, osal_dev *dev)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_flush_for_device(KmppBuffer buffer, void *device)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_flush_for_cpu_partial(KmppBuffer buffer, rk_u32 offset, rk_u32 size)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_flush_for_dev_partial(KmppBuffer buffer, osal_dev *dev, rk_u32 offset, rk_u32 size)
{
    return rk_nok;
}

rk_s32 kmpp_buffer_flush_for_device_partial(KmppBuffer buffer, void *device, rk_u32 offset, rk_u32 size)
{
    return rk_nok;
}


#include <linux/export.h>

EXPORT_SYMBOL(kmpp_buf_init);
EXPORT_SYMBOL(kmpp_buf_deinit);

EXPORT_SYMBOL(kmpp_buf_grp_get_cfg_s);
EXPORT_SYMBOL(kmpp_buf_grp_get_cfg_k);
EXPORT_SYMBOL(kmpp_buf_grp_get_cfg_u);
EXPORT_SYMBOL(kmpp_buf_grp_get_count);
EXPORT_SYMBOL(kmpp_buf_grp_setup);

EXPORT_SYMBOL(kmpp_buffer_inc_ref);
EXPORT_SYMBOL(kmpp_buffer_setup);
EXPORT_SYMBOL(kmpp_buffer_get_cfg_s);
EXPORT_SYMBOL(kmpp_buffer_get_cfg_k);
EXPORT_SYMBOL(kmpp_buffer_get_cfg_u);
EXPORT_SYMBOL(kmpp_buffer_get_kptr);

EXPORT_SYMBOL(kmpp_buffer_read);
EXPORT_SYMBOL(kmpp_buffer_write);
EXPORT_SYMBOL(kmpp_buffer_get_iova);
EXPORT_SYMBOL(kmpp_buffer_put_iova);
EXPORT_SYMBOL(kmpp_buffer_get_iova_by_device);
EXPORT_SYMBOL(kmpp_buffer_put_iova_by_device);

EXPORT_SYMBOL(kmpp_buffer_flush_for_cpu);
EXPORT_SYMBOL(kmpp_buffer_flush_for_dev);
EXPORT_SYMBOL(kmpp_buffer_flush_for_device);
EXPORT_SYMBOL(kmpp_buffer_flush_for_cpu_partial);
EXPORT_SYMBOL(kmpp_buffer_flush_for_dev_partial);
EXPORT_SYMBOL(kmpp_buffer_flush_for_device_partial);
