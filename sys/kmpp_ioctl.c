/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_ioc"

#include <asm/ioctl.h>

#include "rk-mpp-kobj.h"
#include "rk_list.h"

#include "kmpp_cls.h"
#include "kmpp_env.h"
#include "kmpp_log.h"
#include "kmpp_mem.h"
#include "kmpp_spinlock.h"
#include "kmpp_uaccess.h"

#include "kmpp_obj.h"
#include "kmpp_shm.h"
#include "kmpp_ioctl_impl.h"

#define IOC_DBG_FLOW                (0x00000001)
#define IOC_DBG_INFO                (0x00000002)
#define IOC_DBG_FILE                (0x00000004)

#define ioc_dbg(flag, fmt, ...)     kmpp_dbg(kmpp_ioc_debug, flag, fmt, ## __VA_ARGS__)

#define ioc_dbg_flow(fmt, ...)      ioc_dbg(IOC_DBG_FLOW, fmt, ## __VA_ARGS__)
#define ioc_dbg_info(fmt, ...)      ioc_dbg(IOC_DBG_INFO, fmt, ## __VA_ARGS__)
#define ioc_dbg_file(fmt, ...)      ioc_dbg(IOC_DBG_FILE, fmt, ## __VA_ARGS__)

#define get_ioc_srv(caller) \
    ({ \
        KmppIocSrv *__tmp; \
        if (srv_ioc) { \
            __tmp = srv_ioc; \
        } else { \
            kmpp_err_f("kmpp buf srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

#define get_ioc_srv_f() get_ioc_srv(__FUNCTION__)

typedef struct KmppIocSrv_t {
    osal_spinlock       *lock;

    /* /dev/kmpp_ioc device info */
    osal_cls            *cls;
    osal_dev_fops       fops;
    osal_fs_dev_mgr     *mgr;
    KmppEnvNode         env;

    osal_list_head      list_file;

    rk_s32              file_id;
    rk_s32              file_cnt;
} KmppIocSrv;

typedef struct KmppIocMgr_t {
    osal_list_head      list_srv;
    KmppObjDefSet       *defs;
    rk_s32              file_id;
} KmppIocMgr;

static KmppIocSrv *srv_ioc = NULL;
static rk_u32 kmpp_ioc_debug = 0;
static const rk_u8 kmpp_ioc_name[] = "kmpp_ioctl"; /* /dev/kmpp_ioctl */

static rk_s32 kmpp_ioc_open(osal_fs_dev *file)
{
    KmppIocSrv *srv = get_ioc_srv_f();
    KmppIocMgr *mgr = (KmppIocMgr *)file->priv_data;
    KmppObjDefSet *defs = NULL;

    OSAL_INIT_LIST_HEAD(&mgr->list_srv);

    kmpp_objdefset_get(&defs);

    if (!defs) {
        kmpp_loge_f("failed to get shared objdef set\n");
        return rk_nok;
    }

    mgr->defs = defs;

    osal_spin_lock(srv->lock);
    osal_list_add_tail(&mgr->list_srv, &srv->list_file);
    mgr->file_id = srv->file_id++;
    srv->file_cnt++;
    osal_spin_unlock(srv->lock);

    ioc_dbg_file("file %d opened cnt %d\n", mgr->file_id, srv->file_cnt);
    return rk_ok;
}

static rk_s32 kmpp_ioc_release(osal_fs_dev *file)
{
    KmppIocSrv *srv = get_ioc_srv_f();
    KmppIocMgr *mgr = (KmppIocMgr *)file->priv_data;

    if (mgr->defs) {
        kmpp_objdefset_put(mgr->defs);
        mgr->defs = NULL;
    }

    osal_spin_lock(srv->lock);
    osal_list_del_init(&mgr->list_srv);
    srv->file_cnt--;
    osal_spin_unlock(srv->lock);

    ioc_dbg_file("file %d closed cnt %d\n", mgr->file_id, srv->file_cnt);
    return rk_ok;
}

static rk_s32 kmpp_ioc_ioctl(osal_fs_dev *file, rk_s32 cmd, void *arg)
{
    KmppIocMgr *mgr = (KmppIocMgr *)file->priv_data;
    KmppObjDefSet *defs = mgr->defs;
    rk_u32 defs_count = defs->count;
    KmppObjIocArg ioc;
    rk_s32 ret = rk_nok;
    rk_u32 i;

    ioc_dbg_flow("file %d ioctl cmd %x arg %px enter\n", mgr->file_id, cmd, arg);

    if (!arg) {
        kmpp_loge_f("invalid NULL arg\n");
        return rk_nok;
    }

    ret = osal_copy_from_user(&ioc, arg, sizeof(ioc));
    if (ret) {
        kmpp_loge_f("copy ioc KmppShmPtr failed ret %d\n", ret);
        return rk_nok;
    }

    if (!ioc.count) {
        kmpp_loge_f("invalid ioc count %d\n", ioc.count);
        return rk_nok;
    }

    arg += sizeof(ioc);

    for (i = 0; i < ioc.count; i++, arg += sizeof(KmppObjShm)) {
        KmppShmPtr sptr;
        KmppIocImpl *impl;
        KmppShm shm;
        KmppObjDef def;

        ret = osal_copy_from_user(&sptr, arg, sizeof(sptr));
        if (ret) {
            kmpp_loge_f("slot %d ioctl copy_from_user KmppObjShm fail\n", i);
            continue;
        }

        shm = kmpp_shm_from_shmptr_f(&sptr);
        if (!shm) {
            kmpp_loge_f("slot %d ioctl get shm from sptr [u:k] %llx : %llx fail\n",
                        i, sptr.uaddr, sptr.kaddr);
            continue;
        }

        impl = (KmppIocImpl *)kmpp_shm_get_kaddr(shm);
        if (!impl) {
            kmpp_loge_f("slot %d ioctl get impl from sptr [u:k] %llx : %llx fail\n",
                        i, sptr.uaddr, sptr.kaddr);
            continue;
        }

        if (impl->def >= defs_count) {
            kmpp_loge_f("slot %d ioctl invalid def %d while max %d\n",
                        i, impl->def, defs_count);
            continue;
        }

        def = defs->defs[impl->def];
        ret = kmpp_objdef_ioctl(def, file, impl);
        ioc_dbg_info("objdef %d ioctl %px ret %d\n", impl->def, arg, ret);
    }

    ioc_dbg_file("file %d ioctl cmd %x arg %px leave ret %d\n",
                 mgr->file_id, cmd, arg, ret);

    return ret;
}

static rk_s32 kmpp_ioc_read(osal_fs_dev *file, rk_s32 offset, void **buf, rk_s32 *size)
{
    KmppIocMgr *mgr = (KmppIocMgr *)file->priv_data;

    ioc_dbg_file("file %d read\n", mgr->file_id);
    return rk_ok;
}

rk_s32 kmpp_ioctl_init(void)
{
    KmppIocSrv *srv = srv_ioc;
    rk_s32 class_size;
    rk_s32 lock_size;
    rk_s32 mgr_size;
    rk_s32 size;

    if (srv)
        return rk_nok;

    class_size = osal_class_size(kmpp_ioc_name);
    lock_size = osal_spinlock_size();
    mgr_size = osal_fs_dev_mgr_size(kmpp_ioc_name);
    size = sizeof(KmppIocSrv) + lock_size + class_size + mgr_size;
    srv = kmpp_calloc_atomic(size);
    if (!srv) {
        kmpp_err_f("srv_ioc alloc size %d failed\n", size);
        return rk_nok;
    }

    osal_spinlock_assign(&srv->lock, (void *)(srv + 1), lock_size);
    osal_class_assign(&srv->cls, kmpp_ioc_name, (rk_u8 *)srv->lock + lock_size, class_size);

    srv->fops.open = kmpp_ioc_open;
    srv->fops.release = kmpp_ioc_release;
    srv->fops.ioctl = kmpp_ioc_ioctl;
    srv->fops.read = kmpp_ioc_read;

    {
        osal_fs_dev_cfg cfg;

        cfg.cls = srv->cls;
        cfg.name = kmpp_ioc_name;
        cfg.fops = &srv->fops;
        cfg.drv_data = srv;
        cfg.priv_size = sizeof(KmppIocMgr);
        cfg.buf = (rk_u8 *)srv->cls + class_size;
        cfg.size = mgr_size;

        osal_fs_dev_mgr_init(&srv->mgr, &cfg);
        if (!srv->mgr) {
            kmpp_err_f("kmpp_ioc mgr init failed\n");
            goto failed;
        }
    }

    OSAL_INIT_LIST_HEAD(&srv->list_file);

    kmpp_ioc_init();

    {
        KmppEnvInfo env;

        env.type = KmppEnv_u32;
        env.readonly = 0;
        env.name = "kmpp_ioc_debug";
        env.val = &kmpp_ioc_debug;
        env.env_show = NULL;

        kmpp_env_add(kmpp_env_debug, &srv->env, &env);
    }

    srv_ioc = srv;

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

rk_s32 kmpp_ioctl_deinit(void)
{
    KmppIocSrv *srv = get_ioc_srv_f();

    if (!srv)
        return rk_nok;

    osal_fs_dev_mgr_deinit(srv->mgr);
    osal_class_deinit(srv->cls);
    osal_spinlock_deinit(&srv->lock);

    kmpp_ioc_deinit();

    kmpp_env_del(kmpp_env_debug, srv->env);

    srv_ioc = NULL;
    kmpp_free(srv);

    return rk_nok;
}

#define KMPP_OBJ_NAME               kmpp_ioc
#define KMPP_OBJ_INTF_TYPE          KmppIoc
#define KMPP_OBJ_IMPL_TYPE          KmppIocImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_IOC_ENTRY_TABLE
#include "kmpp_obj_helper.h"
