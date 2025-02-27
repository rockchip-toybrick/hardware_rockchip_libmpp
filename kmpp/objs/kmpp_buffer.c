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

#define BUF_DBG_FLOW                (0x00000001)
#define BUF_DBG_INFO                (0x00000002)
#define BUF_DBG_FILE                (0x00000004)

#define buf_dbg(flag, fmt, ...)     kmpp_dbg(kmpp_buf_debug, flag, fmt, ## __VA_ARGS__)

#define buf_dbg_flow(fmt, ...)      buf_dbg(BUF_DBG_FLOW, fmt, ## __VA_ARGS__)
#define buf_dbg_info(fmt, ...)      buf_dbg(BUF_DBG_INFO, fmt, ## __VA_ARGS__)
#define buf_dbg_file(fmt, ...)      buf_dbg(BUF_DBG_FILE, fmt, ## __VA_ARGS__)

#define KMPP_BUF_IOC_MAGIC          'b'

/* get kernel buffer group entry for userspace */
#define KMPP_BUF_IOC_GRP_GET        _IOW(KMPP_BUF_IOC_MAGIC, 1, unsigned int)
/* put kernel buffer group entry to release */
#define KMPP_BUF_IOC_GRP_PUT        _IOW(KMPP_BUF_IOC_MAGIC, 2, unsigned int)
/* setup kernel buffer group entry by share config and update */
#define KMPP_BUF_IOC_GRP_SETUP      _IOW(KMPP_BUF_IOC_MAGIC, 3, unsigned int)
/* dump kernel buffer group entry info */
#define KMPP_BUF_IOC_GRP_DUMP       _IOW(KMPP_BUF_IOC_MAGIC, 4, unsigned int)

/* get kernel buffer entry for userspace */
#define KMPP_BUF_IOC_BUF_GET        _IOW(KMPP_BUF_IOC_MAGIC, 8, unsigned int)
/* put kernel buffer entry to release */
#define KMPP_BUF_IOC_BUF_PUT        _IOW(KMPP_BUF_IOC_MAGIC, 9, unsigned int)
/* setup kernel buffer entry by share config and updata */
#define KMPP_BUF_IOC_BUF_SETUP      _IOW(KMPP_BUF_IOC_MAGIC, 10, unsigned int)
/* dump kernel buffer entry info */
#define KMPP_BUF_IOC_BUF_DUMP       _IOW(KMPP_BUF_IOC_MAGIC, 11, unsigned int)


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

    osal_list_head      list_file;
    osal_list_head      list_grp;
    osal_list_head      list_buf;

    rk_s32              file_id;
    rk_s32              file_cnt;
    rk_s32              grp_id;
    rk_s32              grp_cnt;
    rk_s32              buf_id;
    rk_s32              buf_cnt;

    /* default buffer group */
    KmppBufGrp          group_default;
    KmppBufGrpImpl      *grp;
} KmppBufferSrv;

typedef struct KmppBufGrpMgr_t {
    osal_list_head      list_srv;
    osal_list_head      list_grp;
    osal_list_head      list_buf;
    rk_s32              file_id;
} KmppBufGrpMgr;

static KmppBufferSrv *srv_buf = NULL;
static rk_u32 kmpp_buf_debug = 0;
static const rk_u8 kmpp_buf_name[] = "kmpp_buf_srv"; /* /dev/kmpp_buf_srv */

static rk_s32 kmpp_buf_open(osal_fs_dev *file)
{
    KmppBufferSrv *srv = get_buf_srv_f();
    KmppBufGrpMgr *mgr = (KmppBufGrpMgr *)file->priv_data;

    OSAL_INIT_LIST_HEAD(&mgr->list_srv);
    OSAL_INIT_LIST_HEAD(&mgr->list_grp);
    OSAL_INIT_LIST_HEAD(&mgr->list_buf);

    osal_spin_lock(srv->lock);
    osal_list_add_tail(&mgr->list_srv, &srv->list_file);
    mgr->file_id = srv->file_id++;
    srv->file_cnt++;
    osal_spin_unlock(srv->lock);

    buf_dbg_file("file %d opened cnt %d\n", mgr->file_id, srv->file_cnt);
    return rk_ok;
}

static rk_s32 kmpp_buf_release(osal_fs_dev *file)
{
    KmppBufferSrv *srv = get_buf_srv_f();
    KmppBufGrpMgr *mgr = (KmppBufGrpMgr *)file->priv_data;

    osal_spin_lock(srv->lock);
    osal_list_del_init(&mgr->list_srv);
    srv->file_cnt--;
    osal_spin_unlock(srv->lock);

    buf_dbg_file("file %d closed cnt %d\n", mgr->file_id, srv->file_cnt);
    return rk_ok;
}

static rk_s32 kmpp_buf_ioctl(osal_fs_dev *file, rk_s32 cmd, void *arg)
{
    KmppBufGrpMgr *mgr = (KmppBufGrpMgr *)file->priv_data;
    KmppShmPtr sptr;
    rk_s32 ret = rk_nok;

    buf_dbg_file("file %d ioctl cmd %x arg %px enter\n", mgr->file_id, cmd, arg);

    switch (cmd) {
    case KMPP_BUF_IOC_GRP_GET : {
        KmppBufGrp grp = NULL;
        osal_fs_dev *objs;

        if (!arg) {
            kmpp_err_f("invalid NULL arg\n");
            break;
        }

        objs = kmpp_shm_get_objs_file();
        if (!objs) {
            kmpp_err_f("grp get can not get valid objs file at pid %d\n", osal_getpid());
            break;
        }

        ret = kmpp_buf_grp_get_share(&grp, objs);
        if (ret) {
            kmpp_err_f("grp get share buf grp failed ret %d\n", ret);
            break;
        }

        kmpp_obj_to_shmptr(grp, &sptr);

        kmpp_logi_f("grp get shm ptr [u:k] %llx:%px\n", sptr.uaddr, sptr.kptr);

        ret = osal_copy_to_user(arg, &sptr, sizeof(sptr));
        if (ret)
            kmpp_loge_f("grp get copy KmppShmPtr failed ret %d\n", ret);
    } break;
    case KMPP_BUF_IOC_GRP_PUT : {
        KmppBufGrp grp;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("grp put copy KmppShmPtr failed ret %d\n", ret);
            break;
        }

        grp = sptr.kptr;
        ret = kmpp_obj_check_f(grp);
        if (ret) {
            kmpp_loge_f("grp put invalid KmppBufGrp %px\n", grp);
            break;
        }

        ret = kmpp_buf_grp_put(grp);
        if (ret)
            kmpp_loge_f("grp put buf grp %px failed ret %d\n", grp, ret);
    } break;
    case KMPP_BUF_IOC_GRP_SETUP : {
        KmppShm grp;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("grp setup copy KmppShmPtr failed ret %d\n", ret);
            break;
        }

        grp = kmpp_shm_get_kpriv(sptr.kptr);
        ret = kmpp_obj_check_f(grp);
        if (ret) {
            kmpp_loge_f("grp setup invalid KmppBufGrp %px\n", grp);
            break;
        }

        ret = kmpp_buf_grp_setup_f(grp);
        if (ret)
            kmpp_loge_f("grp setup buf grp %px failed ret %d\n", grp, ret);
    } break;
    case KMPP_BUF_IOC_GRP_DUMP : {
        KmppBufGrp grp;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("grp dump copy KmppShmPtr failed ret %d\n", ret);
            break;
        }

        grp = sptr.kptr;
        ret = kmpp_obj_check_f(grp);
        if (ret) {
            kmpp_loge_f("grp dump invalid KmppBufGrp %px\n", grp);
            break;
        }

        kmpp_buf_grp_dump(grp, __FUNCTION__);
    } break;
    case KMPP_BUF_IOC_BUF_GET : {
        KmppBuffer buf = NULL;
        osal_fs_dev *objs;

        if (!arg) {
            kmpp_err_f("invalid NULL arg\n");
            break;
        }

        objs = kmpp_shm_get_objs_file();
        if (!objs) {
            kmpp_err_f("buf get can not get valid objs file at pid %d\n", osal_getpid());
            break;
        }

        ret = kmpp_buffer_get_share(&buf, objs);
        if (ret) {
            kmpp_err_f("buf get share buffer failed ret %d\n", ret);
            break;
        }

        kmpp_obj_to_shmptr(buf, &sptr);

        kmpp_logi_f("buf get shm ptr [u:k] %llx:%px\n", sptr.uaddr, sptr.kptr);

        ret = osal_copy_to_user(arg, &sptr, sizeof(sptr));
        if (ret)
            kmpp_loge_f("buf get copy KmppShmPtr failed ret %d\n", ret);
    } break;
    case KMPP_BUF_IOC_BUF_PUT : {
        KmppBuffer buf;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("buf put copy KmppShmPtr failed ret %d\n", ret);
            break;
        }

        buf = sptr.kptr;
        ret = kmpp_obj_check_f(buf);
        if (ret) {
            kmpp_loge_f("buf put invalid KmppBuffer %px\n", buf);
            break;
        }

        ret = kmpp_buffer_put(buf);
        if (ret)
            kmpp_loge_f("buf put buf %px failed ret %d\n", buf, ret);
    } break;
    case KMPP_BUF_IOC_BUF_SETUP : {
        KmppBuffer buf;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("buf setup copy KmppShmPtr failed ret %d\n", ret);
            break;
        }

        buf = sptr.kptr;
        ret = kmpp_obj_check_f(buf);
        if (ret) {
            kmpp_loge_f("buf setup invalid KmppBuffer %px\n", buf);
            break;
        }

        ret = kmpp_buffer_setup_f(buf);
        if (ret)
            kmpp_loge_f("buf setup buf %px failed ret %d\n", buf, ret);
    } break;
    case KMPP_BUF_IOC_BUF_DUMP : {
        KmppBuffer buf;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("buf dump copy KmppShmPtr failed ret %d\n", ret);
            break;
        }

        buf = sptr.kptr;
        ret = kmpp_obj_check_f(buf);
        if (ret) {
            kmpp_loge_f("buf dump invalid KmppBuffer %px\n", buf);
            break;
        }

        kmpp_buffer_dump(buf, __FUNCTION__);
    } break;
    default : {
        kmpp_loge_f("invalid cmd %x\n", cmd);
    } break;
    }

    buf_dbg_file("file %d ioctl cmd %x arg %px leave ret %d\n",
                 mgr->file_id, cmd, arg, ret);

    return ret;
}

static rk_s32 kmpp_buf_read(osal_fs_dev *file, rk_s32 offset, void **buf, rk_s32 *size)
{
    KmppBufGrpMgr *mgr = (KmppBufGrpMgr *)file->priv_data;

    buf_dbg_file("file %d read\n", mgr->file_id);
    return rk_ok;
}

rk_s32 kmpp_buf_init(void)
{
    KmppBufferSrv *srv = srv_buf;
    rk_s32 class_size;
    rk_s32 lock_size;
    rk_s32 mgr_size;
    rk_s32 size;

    if (srv)
        return rk_nok;

    class_size = osal_class_size(kmpp_buf_name);
    lock_size = osal_spinlock_size();
    mgr_size = osal_fs_dev_mgr_size(kmpp_buf_name);
    size = sizeof(KmppBufferImpl) + lock_size + class_size + mgr_size;
    srv = kmpp_calloc_atomic(size);
    if (!srv) {
        kmpp_err_f("srv_buf alloc size %d failed\n", size);
        return rk_nok;
    }

    osal_spinlock_assign(&srv->lock, (void *)(srv + 1), lock_size);
    osal_class_assign(&srv->cls, kmpp_buf_name, (rk_u8 *)srv->lock + lock_size, class_size);

    srv->fops.open = kmpp_buf_open;
    srv->fops.release = kmpp_buf_release;
    srv->fops.ioctl = kmpp_buf_ioctl;
    srv->fops.read = kmpp_buf_read;

    {
        osal_fs_dev_cfg cfg;

        cfg.cls = srv->cls;
        cfg.name = kmpp_buf_name;
        cfg.fops = &srv->fops;
        cfg.drv_data = srv;
        cfg.priv_size = sizeof(KmppBufGrpMgr);
        cfg.buf = (rk_u8 *)srv->cls + class_size;
        cfg.size = mgr_size;

        osal_fs_dev_mgr_init(&srv->mgr, &cfg);
        if (!srv->mgr) {
            kmpp_err_f("kmpp_buf mgr init failed\n");
            goto failed;
        }
    }

    OSAL_INIT_LIST_HEAD(&srv->list_file);
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
failed:
    if (srv) {
        osal_fs_dev_mgr_deinit(srv->mgr);
        osal_class_deinit(srv->cls);
        osal_spinlock_deinit(&srv->lock);

        kmpp_free(srv);
    }

    return rk_nok;
}

rk_s32 kmpp_buf_deinit(void)
{
    KmppBufferSrv *srv = get_buf_srv_f();
    KmppBufGrpImpl *grp, *n;

    if (!srv)
        return rk_nok;

    /* remove default group from the list */
    if (srv->group_default) {
        kmpp_logi_f("srv %px grp %px - %px\n", srv, srv->group_default, srv->grp);
        kmpp_logi_f("list %px prev %px next %px\n",
                    &srv->grp->list_srv,
                    srv->grp->list_srv.prev,
                    srv->grp->list_srv.next);
        osal_list_del_init(&srv->grp->list_srv);
        kmpp_logi_f("done\n");
    }

    osal_list_for_each_entry_safe(grp, n, &srv->list_grp, KmppBufGrpImpl, list_srv) {
        kmpp_buf_grp_put(grp);
    }

    if (srv->group_default)
        kmpp_buf_grp_put(srv->group_default);

    srv->group_default = NULL;
    srv->grp = NULL;

    osal_fs_dev_mgr_deinit(srv->mgr);
    osal_class_deinit(srv->cls);
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

    impl->cfg_set = kmpp_obj_to_entry(impl->cfg_ext);
    kmpp_obj_to_shmptr(impl->cfg_ext, &impl->cfg);

    osal_spin_lock(srv->lock);
    osal_list_add_tail(&impl->list_srv, &srv->list_grp);
    impl->grp_id = srv->grp_id++;
    srv->grp_cnt++;
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

    kmpp_logi_f("group %px\n", impl);

    return rk_ok;
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
    return grp->cfg_impl.size;
}

rk_s32 kmpp_buf_grp_get_count(KmppBufGrpImpl *grp)
{
    return grp->cfg_impl.count;
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

    cfg = impl->cfg_set;
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

    osal_memcpy(&impl->cfg_impl, cfg, sizeof(impl->cfg_impl));

    buf_dbg_flow("buf grp %d setup leave at %s\n", impl->grp_id, caller);

    return rk_ok;
}

rk_s32 kmpp_buf_grp_reset(KmppBufGrp group, const rk_u8 *caller)
{
    return rk_ok;
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

static KmppObjIoctls kmpp_buf_grp_ioctls = {
    .count = 1,
    .funcs = {
        [0] = {
            .cmd = 0,
            .func = kmpp_buf_grp_ioc_setup,
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
    kmpp_logi("device           %px\n", impl->device);
    kmpp_logi("allocator [u:k]  %#llx:%#llx\n", impl->allocator.uaddr, impl->allocator.kaddr);
    kmpp_logi("name [u:k]       %#llx:%#llx\n", impl->name.uaddr, impl->name.kaddr);

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

    impl->cfg_set = kmpp_obj_to_entry(impl->cfg_ext);
    kmpp_obj_to_shmptr(impl->cfg_ext, &impl->cfg);

    buf_dbg_info("buf cfg [u:k] %#llx:%#llx\n", impl->cfg.uaddr, impl->cfg.kaddr);

    impl->ref_cnt = 1;

    osal_spin_lock(srv->lock);
    osal_list_add_tail(&impl->list_status, &srv->list_buf);
    impl->srv_buf_id = srv->buf_id++;
    srv->buf_cnt++;
    osal_spin_unlock(srv->lock);

    buf_dbg_flow("buf %px - %d get leave at %s\n", impl, impl->srv_buf_id, caller);

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
                 impl->srv_buf_id, old, ref_cnt, caller);

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
        impl->grp = NULL;
        osal_spin_unlock(grp->lock);
    }

    osal_spin_lock(srv->lock);
    osal_list_del_init(&impl->list_status);
    srv->buf_cnt--;
    osal_spin_unlock(srv->lock);

    if (impl->cfg_ext)
        kmpp_buf_cfg_put(impl->cfg_ext);

    impl->cfg_ext = NULL;
    impl->cfg_set = NULL;

    if (impl->buf) {
        kmpp_dmabuf_free(impl->buf, caller);
        impl->buf = NULL;
    }

    buf_dbg_flow("buf %d put leave at %s\n", impl->srv_buf_id, caller);

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

    buf_dbg_flow("buf %d inc ref %d -> %d at %s\n",
                 impl->srv_buf_id, old, impl->ref_cnt, caller);

    return rk_ok;
}

rk_s32 kmpp_buffer_setup(KmppBuffer buffer, const rk_u8 *caller)
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

    buf_dbg_flow("buf %d setup enter at %s\n", impl->srv_buf_id, caller);

    buf_cfg = impl->cfg_set;
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

    osal_memcpy(&impl->cfg_impl, buf_cfg, sizeof(impl->cfg_impl));
    impl->buf = dmabuf;
    impl->kptr = kmpp_dmabuf_get_kptr(dmabuf);

    osal_spin_lock(srv->lock);
    osal_list_del_init(&impl->list_status);
    osal_spin_unlock(srv->lock);

    osal_spin_lock(grp->lock);
    osal_list_add_tail(&impl->list_status, &grp->list_used);
    impl->grp = grp;
    impl->grp_id = grp->grp_id;
    impl->buf_id = grp->buf_id++;
    grp->buf_cnt++;
    grp->count_used++;
    osal_spin_unlock(grp->lock);

    impl->grp_id = grp->grp_id;

    buf_dbg_flow("buf %d [%d:%d] setup leave at %s\n",
                 impl->srv_buf_id, impl->grp_id, impl->buf_id, caller);

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

    ret = kmpp_buffer_setup_f(buf);
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