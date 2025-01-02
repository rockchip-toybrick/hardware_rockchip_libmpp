/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_shm"

#include <asm/ioctl.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>

#include "rk_list.h"
#include "rk-mpp-kobj.h"

#include "kmpp_osal.h"

#include "kmpp_shm.h"
#include "kmpp_obj.h"

#define SHM_DBG_FLOW                (0x00000001)
#define SHM_DBG_DETAIL              (0x00000002)
#define SHM_DBG_IOCTL               (0x00000004)
#define SHM_DBG_DUMP                (0x00000008)

#define shm_dbg(flag, fmt, ...)     kmpp_dbg(kmpp_shm_debug, flag, fmt, ## __VA_ARGS__)
#define shm_dbg_f(flag, fmt, ...)   kmpp_dbg_f(kmpp_shm_debug, flag, fmt, ## __VA_ARGS__)

#define shm_dbg_flow(fmt, ...)      shm_dbg_f(SHM_DBG_FLOW, fmt, ## __VA_ARGS__)
#define shm_dbg_detail(fmt, ...)    shm_dbg_f(SHM_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define shm_dbg_ioctl(fmt, ...)     shm_dbg(SHM_DBG_IOCTL, fmt, ## __VA_ARGS__)

#define KMPP_SHM_IOC_MAGIC          'm'
#define KMPP_SHM_IOC_QUERY_INFO     _IOW(KMPP_SHM_IOC_MAGIC, 1, unsigned int)
#define KMPP_SHM_IOC_GET_SHM        _IOW(KMPP_SHM_IOC_MAGIC, 2, unsigned int)
#define KMPP_SHM_IOC_PUT_SHM        _IOW(KMPP_SHM_IOC_MAGIC, 3, unsigned int)
#define KMPP_SHM_IOC_DUMP           _IOW(KMPP_SHM_IOC_MAGIC, 4, unsigned int)

/* kmpp share memory class */
typedef struct KmppShmClsImpl_t {
    osal_spinlock   *lock;
    osal_cls        *cls;
    osal_dev_fops   fops;
    osal_list_head  list_mgr;
} KmppShmClsImpl;

typedef struct KmppShmMapSize_t {
    rk_s32          size_aligned;
    rk_s32          size_map;
    rk_s32          pg_num;
    rk_s32          pg_den;
} KmppShmMapSize;

/* kmpp share memory manager (kmpp_shm_mgr) each file class has a manager */
typedef struct KmppShmMgrImpl_t {
    osal_spinlock   *lock;
    /* list to list_mgr in KmppShmClsImpl */
    osal_list_head  list_cls;
    /* list for list_grp in KmppShmGrpImpl */
    osal_list_head  list_grp;
    osal_fs_dev_mgr *mgr;
    const rk_u8     *name;
    rk_s32          size;
    KmppShmMapSize  map_size;
    rk_u32          bn_mgr_id;
    KmppObjDef      def;

    /* multi objdef search list */
    osal_list_head  list_def;
    rk_s32          def_count;
} KmppShmMgrImpl;

typedef struct KmppObjDefNode_t {
    osal_list_head  list_mgr;
    osal_list_head  list_grp;
    KmppObjDef      def;
    KmppTrie        trie;
    const rk_u8     *name;
    KmppShmMapSize  map_size;
    rk_s32          size;
    rk_s32          grp_count;
} KmppObjDefNode;

/*
 * kmpp share memory group (kmpp_shm_grp) each file handle can has a group
 * for buffer fast access
 */
typedef struct KmppShmGrpImpl_t {
    osal_spinlock   *lock;
    /* list to list_shm in KmppShmMgrImpl */
    osal_list_head  list_mgr;
    /* list to list_grp in KmppObjDefNode */
    osal_list_head  list_node;
    /* list to list_file in KmppShmGrpImpl inside file private data */
    osal_list_head  list_file;
    /* list for list_shm in KmppShmGrpImpl */
    osal_list_head  list_shm;

    /* device file infomation link */
    osal_fs_dev     *file;
    KmppShmMgrImpl  *mgr;
    KmppObjDefNode  *node;
    KmppObjDef      def;
    rk_s32          size;
    rk_s32          free_on_put;

	/* session id for pid in system */
	rk_u32          sid;

    /* buffer node allocator */
    osal_fs_vm      *vm_trie;
    void            *trie_buf;
    rk_s32          trie_size;
} KmppShmGrpImpl;

typedef struct KmppShmImpl_t {
    KmppObjShm      ioc;

    osal_fs_vm      *vm_shm;

    /* list to list_shm in KmppShmGrpImpl */
    osal_list_head  list_grp;
    KmppShmGrpImpl  *grp;
    KmppObj         obj;

    void            *kbase;
    void            *kaddr;
    rk_ul           ubase;
    rk_u64          uaddr;
} KmppShmImpl;

static KmppShmClsImpl *kmpp_shm_cls = NULL;
static const rk_u8 kmpp_shm_name[] = "kmpp_shm";
static KmppEnvNode kmpp_shm_env = NULL;
static KmppShmMgr kmpp_shm_objs = NULL;
static rk_u32 kmpp_shm_sid = 0;
static rk_u32 kmpp_shm_debug = 0;

static inline rk_s32 align_to_power_of_two(rk_s32 n)
{
    if (n <= 0)
        return 2;

    n -= 1;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n + 1;
}

static void size_to_map_info(KmppShmMapSize *info, rk_s32 size)
{
    if (size < PAGE_SIZE) {
        info->size_aligned = align_to_power_of_two(size);
        info->size_map = PAGE_SIZE;
        info->pg_num = 1;
        info->pg_den = PAGE_SIZE / info->size_aligned;
    } else {
        info->size_aligned = PAGE_ALIGN(size);
        info->size_map = info->size_aligned;
        info->pg_num = info->size_aligned / PAGE_SIZE;
        info->pg_den = 1;
    }
}

static KmppShmGrpImpl *get_shm_grp(osal_fs_dev *file, void *buf, rk_s32 size, KmppObjDef def)
{
    KmppShmMgrImpl *mgr = (KmppShmMgrImpl *)file->drv_data;
    KmppShmGrpImpl *grp = NULL;
    KmppShmGrpCfg cfg;

    /* NOTE: init preallocated group */
    cfg.mgr = mgr;
    cfg.file = file;
    cfg.buf = buf;
    cfg.size = size;

    if (kmpp_shm_grp_get((KmppShmGrp *)&grp, &cfg)) {
        kmpp_loge_f("failed to get group %s\n", mgr->name);
        return NULL;
    }

    if (def) {
        KmppTrie trie = kmpp_objdef_get_trie(def);
        rk_s32 trie_size = kmpp_trie_get_buf_size(trie);
        rk_s32 aligned_size = PAGE_ALIGN(trie_size);
        void *share_buf = kmpp_malloc_share(aligned_size);

        grp->trie_size = aligned_size;
        grp->trie_buf = share_buf;
        grp->def = def;

        osal_memcpy(share_buf, kmpp_trie_get_node_root(trie), trie_size);

        shm_dbg_detail("trie_size %d -> %d buf %px\n",
                       trie_size, aligned_size, buf);
    }

    shm_dbg_flow("%s sid %d\n", mgr->name, grp->sid);

    return grp;
}

static rk_s32 put_shm_grp(KmppShmGrpImpl *grp)
{
    rk_s32 ret = rk_nok;

    if (grp) {
        KmppShmMgrImpl *mgr = grp->mgr;

        if (grp->vm_trie)
            osal_fs_vm_munmap(grp->vm_trie);

        kmpp_free_share(grp->trie_buf);
        grp->trie_size = 0;

        shm_dbg_flow("%s sid %d\n", mgr->name, grp->sid);

        ret = kmpp_shm_grp_put(grp);
    }

    return ret;
}

rk_s32 kmpp_shm_open(osal_fs_dev *file)
{
    KmppShmMgrImpl *mgr = (KmppShmMgrImpl *)file->drv_data;
    KmppShmGrpImpl *grp = NULL;

    shm_dbg_flow("enter - %s size %d\n", mgr->name, mgr->size);

    grp = get_shm_grp(file, file->priv_data, file->priv_size, mgr->def);

    shm_dbg_flow("leave - %s sid %d\n", mgr->name, grp ? grp->sid : -1);

    return grp ? rk_ok : rk_nok;
}

rk_s32 kmpp_shm_release(osal_fs_dev *file)
{
    KmppShmMgrImpl *mgr = (KmppShmMgrImpl *)file->drv_data;
    KmppShmGrpImpl *grp = (KmppShmGrpImpl *)file->priv_data;
    KmppShmGrpImpl *pos, *n;
    OSAL_LIST_HEAD(list);
    rk_s32 ret = rk_ok;

    shm_dbg_flow("enter - %s grp %px\n", mgr->name, grp);

    osal_spin_lock(mgr->lock);
    osal_list_for_each_entry_safe(pos, n, &grp->list_file, KmppShmGrpImpl, list_file) {
        osal_list_move_tail(&pos->list_file, &list);
        osal_list_del_init(&pos->list_node);
        if (pos->node)
            pos->node->grp_count--;
        else
            kmpp_loge_f("grp %d node is NULL\n", pos->sid);
    }
    osal_spin_unlock(mgr->lock);

    osal_list_for_each_entry_safe(pos, n, &list, KmppShmGrpImpl, list_file) {
        ret = put_shm_grp(pos);
        if (ret)
            kmpp_loge_f("put_shm_grp %d fail ret %d\n", pos->sid, ret);
    }

    ret = put_shm_grp(grp);
    if (ret)
        kmpp_loge_f("put_shm_grp %d in file fail ret %d\n", pos->sid, ret);

    shm_dbg_flow("leave - %s\n", mgr->name);

    return ret;
}

static KmppShmGrpImpl *get_shm_grp_by_arg(osal_fs_dev *file, void *arg, const rk_u8 *log)
{
    KmppShmMgrImpl *mgr = (KmppShmMgrImpl *)file->drv_data;
    KmppShmGrpImpl *fgrp = (KmppShmGrpImpl *)file->priv_data;
    KmppObjDefNode *node = NULL;
    KmppShmGrpImpl *grp = NULL;
    KmppObjDefNode *pos, *n;
    KmppShmGrpImpl *gpos, *gn;
    rk_u64 name_uaddr;
    rk_u8 name[64];
    rk_s32 ret = rk_nok;

    if (mgr->def)
        return fgrp;

    ret = osal_copy_from_user(&name_uaddr, arg, sizeof(name_uaddr));
    if (ret) {
        kmpp_loge_f("by %s osal_copy_from_user %s fail ret %d\n", log, ret);
        return NULL;
    }

    ret = strncpy_from_user(name, (void *)name_uaddr, sizeof(name));
    if (ret <= 0) {
        kmpp_loge_f("by %s strncpy_from_user fail ret %d\n", log, ret);
        return NULL;
    }

    osal_spin_lock(mgr->lock);
    osal_list_for_each_entry_safe(pos, n, &mgr->list_def, KmppObjDefNode, list_mgr) {
        if (!strncmp(pos->name, name, sizeof(name))) {
            node = pos;
            osal_list_for_each_entry_safe(gpos, gn, &pos->list_grp, KmppShmGrpImpl, list_node) {
                if (gpos->file == file) {
                    grp = gpos;
                    break;
                }
            }
            break;
        }
    }
    osal_spin_unlock(mgr->lock);

    if (!node || !node->def) {
        kmpp_loge_f("by %s can not found valid obj %s\n", log, name);
        return NULL;
    }

    /* TODO: fix race on two get_shm_grp */
    if (!grp) {
        grp = get_shm_grp(file, NULL, 0, node->def);
        osal_spin_lock(mgr->lock);
        grp->node = node;
        osal_list_add_tail(&grp->list_node, &node->list_grp);
        osal_list_add_tail(&grp->list_file, &fgrp->list_file);
        node->grp_count++;
        osal_spin_unlock(mgr->lock);
    }

    return grp;
}

rk_s32 kmpp_shm_ioctl(osal_fs_dev *file, rk_s32 cmd, void *arg)
{
    KmppShmMgrImpl *mgr = (KmppShmMgrImpl *)file->drv_data;
    rk_s32 ret = rk_nok;

    shm_dbg_flow("enter - %s cmd %#x arg %px\n", mgr->name, cmd, arg);

    switch (cmd) {
    case KMPP_SHM_IOC_QUERY_INFO : {
        KmppShmGrpImpl *grp = NULL;
        rk_u64 uaddr;

        grp = get_shm_grp_by_arg(file, arg, "KMPP_SHM_IOC_QUERY_INFO");
        if (!grp)
            break;

        if (!grp->vm_trie) {
            ret = osal_fs_vm_mmap(&grp->vm_trie, file, grp->trie_buf,
                                grp->trie_size, KMPP_VM_PROT_RO);
            if (ret || !grp->vm_trie) {
                kmpp_loge_f("vm_mmap fail ret %d %px\n", ret, grp->vm_trie);
                break;
            }
        }

        uaddr = (rk_u64)grp->vm_trie->uaddr;

        shm_dbg_ioctl("query obj trie root %#llx\n", uaddr);

        ret = osal_copy_to_user(arg, &uaddr, sizeof(uaddr));
        if (ret)
            kmpp_loge_f("KMPP_SHM_IOC_QUERY_INFO osal_copy_to_user fail ret %d\n", ret);
    } break;
    case KMPP_SHM_IOC_GET_SHM : {
        KmppObjIocArg ioc;
        rk_u32 i;

        ret = osal_copy_from_user(&ioc, arg, sizeof(ioc));
        if (ret) {
            kmpp_loge_f("KMPP_SHM_IOC_GET_SHM osal_copy_from_user KmppObjIocArg fail\n");
            break;
        }

        shm_dbg_ioctl("put shm %d\n", ioc.count);

        if (ioc.count > KMPP_SHM_IOC_MAX_COUNT) {
            kmpp_loge_f("KMPP_SHM_IOC_GET_SHM count %d over max %d\n",
                        ioc.count, KMPP_SHM_IOC_MAX_COUNT);
            break;
        }

        arg += sizeof(ioc);

        for (i = 0; i < ioc.count; i++, arg += sizeof(rk_u64)) {
            KmppShmGrpImpl *grp = NULL;
            KmppObj obj = NULL;
            KmppShm shm = NULL;
            void *kbase = NULL;
            rk_u64 ubase = 0;

            shm_dbg_ioctl("get shm ioc %d\n", i);

            grp = get_shm_grp_by_arg(file, arg, "KMPP_SHM_IOC_GET_SHM");
            if (!grp || !grp->def) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_GET_SHM get_shm_grp_by_arg failed\n", i);
                break;
            }

            ret = kmpp_obj_get_share(&obj, grp->def, grp);
            if (ret || !obj)
                kmpp_loge_f("slot %d kmpp_obj_get_share fail ret %d\n", i, ret);

            if (obj && (kmpp_shm_debug & SHM_DBG_DUMP))
                kmpp_obj_dump(obj, "KMPP_SHM_IOC_GET_SHM");

            shm = kmpp_obj_get_shm(obj);
            ubase = kmpp_shm_get_ubase(shm);
            kbase = kmpp_shm_get_kbase(shm);

            shm_dbg_ioctl("slot %d get shm k:%px u:%#llx\n", i, kbase, ubase);

            if (osal_copy_to_user(arg, &ubase, sizeof(ubase))) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_GET_SHM osal_copy_to_user fail\n", i);
                ret = rk_nok;
            }
        }
    } break;
    case KMPP_SHM_IOC_PUT_SHM : {
        KmppObjIocArg ioc;
        rk_u32 i;

        ret = osal_copy_from_user(&ioc, arg, sizeof(ioc));
        if (ret) {
            kmpp_loge_f("KMPP_SHM_IOC_GET_SHM osal_copy_from_user KmppObjIocArg fail\n");
            break;
        }

        shm_dbg_ioctl("put shm %d\n", ioc.count);

        if (ioc.count > KMPP_SHM_IOC_MAX_COUNT) {
            kmpp_loge_f("KMPP_SHM_IOC_GET_SHM count %d over max %d\n",
                        ioc.count, KMPP_SHM_IOC_MAX_COUNT);
            break;
        }

        arg += sizeof(ioc);

        for (i = 0; i < ioc.count; i++, arg += sizeof(rk_u64)) {
            rk_u64 uaddr;
            KmppObjShm shm;
            KmppShmImpl *impl;

            ret = osal_copy_from_user(&uaddr, arg, sizeof(uaddr));
            if (ret) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_PUT_SHM osal_copy_from_user uaddr fail\n", i);
                break;
            }

            ret = osal_copy_from_user(&shm, (void *)uaddr, sizeof(shm));
            if (ret) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_PUT_SHM osal_copy_from_user KmppObjShm fail\n", i);
                break;
            }

            impl = (KmppShmImpl *)shm.kobj_kaddr;
            if (!impl || impl != impl->kbase) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_PUT_SHM invalid shm %px\n", i, impl);
                break;
            }

            shm_dbg_ioctl("slot %d put shm k:%px u:%#llx\n", i, impl->kbase, impl->ubase);

            if (impl->obj) {
                if (kmpp_shm_debug & SHM_DBG_DUMP)
                    kmpp_obj_dump(impl->obj, "KMPP_SHM_IOC_PUT_SHM");

                ret = kmpp_obj_put(impl->obj);
            } else {
                ret = kmpp_shm_put(impl);
            }
        }
    } break;
    case KMPP_SHM_IOC_DUMP : {
        KmppObjShm ioc;

        ret = osal_copy_from_user(&ioc, arg, sizeof(ioc));
        if (ret) {
            kmpp_loge_f("KMPP_SHM_IOC_DUMP osal_copy_from_user fail\n");
        } else {
            KmppShmImpl *impl = (KmppShmImpl *)ioc.kobj_kaddr;

            if (!impl || impl != impl->kbase) {
                kmpp_loge_f("KMPP_SHM_IOC_DUMP invalid shm %px\n", impl);
                break;
            }

            if (impl->obj)
                ret = kmpp_obj_dump(impl->obj, "KMPP_SHM_IOC_DUMP");
        }
    } break;
    default : {
        kmpp_loge_f("KMPP_SHM_IOC_GET_SHM unknown cmd %x\n", cmd);
    } break;
    }

    shm_dbg_flow("leave - %s cmd %x\n", mgr->name, cmd);

    return ret;
}

rk_s32 kmpp_shm_read(osal_fs_dev *file, rk_s32 offset, void **buf, rk_s32 *size)
{
    kmpp_objdef_dump_all();
    return 0;
}

rk_s32 kmpp_shm_init(void)
{
    KmppEnvInfo env;
    rk_s32 lock_size = osal_spinlock_size();
    rk_s32 class_size = osal_class_size(kmpp_shm_name);
    rk_u8 *buf;

    if (kmpp_shm_cls) {
        kmpp_loge_f("kmpp_shm_cls is already inited\n");
        return rk_nok;
    }

    buf = kmpp_calloc(sizeof(KmppShmClsImpl) + lock_size + class_size);
    if (!buf) {
        kmpp_loge_f("kmpp_shm_cls malloc failed\n");
        return rk_nok;
    }

    kmpp_shm_cls = (KmppShmClsImpl *)buf;

    osal_spinlock_assign(&kmpp_shm_cls->lock, buf + sizeof(KmppShmClsImpl), lock_size);
    osal_class_assign(&kmpp_shm_cls->cls, kmpp_shm_name,
                      buf + sizeof(KmppShmClsImpl) + lock_size, class_size);

    kmpp_shm_cls->fops.open = kmpp_shm_open;
    kmpp_shm_cls->fops.release = kmpp_shm_release;
    kmpp_shm_cls->fops.ioctl = kmpp_shm_ioctl;
    kmpp_shm_cls->fops.read = kmpp_shm_read;

    OSAL_INIT_LIST_HEAD(&kmpp_shm_cls->list_mgr);

    env.type = KmppEnv_u32;
    env.readonly = 0;
    env.name = "kmpp_shm_debug";
    env.val = &kmpp_shm_debug;
    env.env_show = NULL;

    kmpp_env_add(kmpp_env_debug, &kmpp_shm_env, &env);

    kmpp_shm_mgr_get(&kmpp_shm_objs, "kmpp_objs");

    return rk_ok;
}

rk_s32 kmpp_shm_deinit(void)
{
    KmppShmClsImpl *cls = KMPP_FETCH_AND(&kmpp_shm_cls, NULL);
    KmppShmMgrImpl *mgr, *n;
    OSAL_LIST_HEAD(list);

    if (!cls)
        return rk_ok;

    osal_spin_lock(cls->lock);
    osal_list_for_each_entry_safe(mgr, n, &cls->list_mgr, KmppShmMgrImpl, list_cls) {
        osal_list_move_tail(&mgr->list_cls, &list);
    }
    osal_spin_unlock(cls->lock);

    osal_list_for_each_entry_safe(mgr, n, &cls->list_mgr, KmppShmMgrImpl, list_cls) {
        kmpp_shm_mgr_put(mgr);
    }

    if (kmpp_shm_objs) {
        kmpp_shm_mgr_put(kmpp_shm_objs);
        kmpp_shm_objs = NULL;
    }

    if (cls) {
        osal_spinlock_deinit(&cls->lock);
        osal_class_deinit(cls->cls);
        kmpp_free(cls);
    }

    if (kmpp_shm_env) {
        kmpp_env_del(kmpp_env_debug, kmpp_shm_env);
        kmpp_shm_env = NULL;
    }

    return rk_ok;
}

KmppShmMgr *kmpp_shm_get_objs_mgr(void)
{
    return kmpp_shm_objs;
}

rk_s32 kmpp_shm_entry_offset(void)
{
    return sizeof(KmppShmImpl);
}

rk_s32 kmpp_shm_mgr_get(KmppShmMgr *mgr, const rk_u8 *name)
{
    KmppShmMgrImpl *impl = NULL;
    osal_fs_dev_cfg cfg;
    rk_s32 lock_size = osal_spinlock_size();
    rk_s32 mgr_size;
    rk_s32 ret = rk_nok;

    if (!mgr || !name) {
        kmpp_loge_f("invalid param mgr %px name %s size %d\n", mgr, name);
        return ret;
    }

    *mgr = NULL;
    lock_size = osal_spinlock_size();
    mgr_size = osal_fs_dev_mgr_size(name);
    impl = kmpp_calloc(sizeof(*impl) + lock_size + mgr_size);
    if (!impl) {
        kmpp_loge_f("%s malloc size %d failed\n", name, mgr_size);
        goto done;
    }

    osal_spinlock_assign(&impl->lock, impl + 1, lock_size);

    cfg.cls = kmpp_shm_cls->cls;
    cfg.name = name;
    cfg.fops = &kmpp_shm_cls->fops;
    cfg.drv_data = impl;
    cfg.priv_size = kmpp_shm_grp_size();
    cfg.buf = (void *)((rk_u8 *)(impl + 1) + lock_size);
    cfg.size = mgr_size;

    ret = osal_fs_dev_mgr_init(&impl->mgr, &cfg);
    if (ret || !impl->mgr) {
        kmpp_loge_f("dev mgr %s init failed\n", name);
        goto done;
    } else {
        impl->name = cfg.name;
        impl->size = 0;

        OSAL_INIT_LIST_HEAD(&impl->list_cls);
        OSAL_INIT_LIST_HEAD(&impl->list_grp);
        OSAL_INIT_LIST_HEAD(&impl->list_def);

        if (kmpp_shm_cls) {
            osal_spin_lock(kmpp_shm_cls->lock);
            osal_list_add_tail(&impl->list_cls, &kmpp_shm_cls->list_mgr);
            osal_spin_unlock(kmpp_shm_cls->lock);
        }

        ret = rk_ok;
    }

done:
    if (ret && impl) {
        kmpp_shm_mgr_put(impl);
        impl = NULL;
    }

    *mgr = impl;

    return ret;
}

rk_s32 kmpp_shm_mgr_bind_objdef(KmppShmMgr mgr, KmppObjDef def)
{
    KmppShmMgrImpl *impl = (KmppShmMgrImpl *)mgr;
    KmppObjDefNode *node = NULL;

    if (!impl || !def) {
        kmpp_loge_f("invalid param mgr %px def %px\n", impl, def);
        return rk_nok;
    }

    node = kmpp_calloc(sizeof(*node) + kmpp_shm_grp_size());
    if (!node) {
        kmpp_loge_f("malloc node failed\n");
        return rk_nok;
    }

    OSAL_INIT_LIST_HEAD(&node->list_mgr);
    OSAL_INIT_LIST_HEAD(&node->list_grp);
    node->def = def;
    node->trie = kmpp_objdef_get_trie(def);
    node->name = kmpp_objdef_get_name(def);
    node->size = kmpp_objdef_get_entry_size(def);
    size_to_map_info(&node->map_size, node->size);

    osal_spin_lock(impl->lock);
    osal_list_add_tail(&node->list_mgr, &impl->list_def);
    impl->def_count++;
    osal_spin_unlock(impl->lock);

    return rk_ok;
}

rk_s32 kmpp_shm_mgr_unbind_objdef(KmppShmMgr mgr, KmppObjDef def)
{
    KmppShmMgrImpl *impl = (KmppShmMgrImpl *)mgr;
    KmppObjDefNode *node = NULL;
    KmppObjDefNode *pos, *n;

    if (!impl || !def) {
        kmpp_loge_f("invalid param mgr %px def %px\n", impl, def);
        return rk_nok;
    }

    osal_spin_lock(impl->lock);
    osal_list_for_each_entry_safe(pos, n, &impl->list_def, KmppObjDefNode, list_mgr) {
        if (pos->def == def) {
            osal_list_del_init(&pos->list_mgr);
            node = pos;
            impl->def_count--;
            break;
        }
    }
    osal_spin_unlock(impl->lock);

    kmpp_free(node);

    return rk_ok;
}

rk_s32 kmpp_shm_mgr_put(KmppShmMgr mgr)
{
    KmppShmMgrImpl *impl = (KmppShmMgrImpl *)mgr;
    KmppShmGrpImpl *grp, *n;
    OSAL_LIST_HEAD(list);

    if (!mgr) {
        kmpp_loge_f("invalid param mgr %px\n", mgr);
        return rk_nok;
    }

    if (kmpp_shm_cls) {
        osal_spin_lock(kmpp_shm_cls->lock);
        osal_list_del_init(&impl->list_cls);
        osal_spin_unlock(kmpp_shm_cls->lock);
    }

    osal_spin_lock(impl->lock);
    osal_list_for_each_entry_safe(grp, n, &impl->list_grp, KmppShmGrpImpl, list_mgr) {
        osal_list_move_tail(&grp->list_mgr, &list);
    }
    osal_spin_unlock(impl->lock);

    osal_list_for_each_entry_safe(grp, n, &list, KmppShmGrpImpl, list_mgr) {
        kmpp_shm_grp_put(grp);
    }

    {
        KmppObjDefNode *pos, *n;

        osal_spin_lock(impl->lock);
        osal_list_for_each_entry_safe(pos, n, &impl->list_def, KmppObjDefNode, list_mgr) {
            osal_list_move_tail(&pos->list_mgr, &list);
            impl->def_count--;
        }
        osal_spin_unlock(impl->lock);

        osal_list_for_each_entry_safe(pos, n, &list, KmppObjDefNode, list_mgr) {
            kmpp_objdef_deinit(pos->def);
            kmpp_free(pos);
        }
    }

    osal_spinlock_deinit(&impl->lock);

    if (impl->mgr) {
        osal_fs_dev_mgr_deinit(impl->mgr);
        impl->mgr = NULL;
    }

    kmpp_free(impl);

    return rk_ok;
}

rk_s32 kmpp_shm_mgr_query(KmppShmMgr *mgr, const rk_u8 *name)
{
    KmppShmMgrImpl *ret, *n;

    if (!mgr || !name) {
        kmpp_loge_f("invalid param mgr %px name %s\n", mgr, name);
        return rk_nok;
    }

    *mgr = NULL;

    if (!kmpp_shm_cls) {
        kmpp_loge_f("kmpp_shm_cls not init\n");
        return rk_nok;
    }

    osal_spin_lock(kmpp_shm_cls->lock);
    osal_list_for_each_entry_safe(ret, n, &kmpp_shm_cls->list_mgr, KmppShmMgrImpl, list_cls) {
        if (!osal_strcmp(ret->name, name)) {
            *mgr = ret;
            osal_spin_unlock(kmpp_shm_cls->lock);
            return rk_ok;
        }
    }
    osal_spin_unlock(kmpp_shm_cls->lock);

    return rk_nok;
}

rk_s32 kmpp_shm_grp_size(void)
{
    return sizeof(KmppShmGrpImpl) + osal_spinlock_size();
}

rk_s32 kmpp_shm_grp_get(KmppShmGrp *grp, KmppShmGrpCfg *cfg)
{
    KmppShmMgrImpl *mgr;
    KmppShmGrpImpl *impl;
    osal_fs_dev *file;
    void *buf;
    rk_s32 buf_size;

    if (!grp) {
        kmpp_loge_f("invalid param gourp %px\n", grp);
        return rk_nok;
    }

    *grp = NULL;
    mgr = (KmppShmMgrImpl *)(cfg ? cfg->mgr : NULL);
    file = cfg ? cfg->file : NULL;
    buf = cfg ? cfg->buf : NULL;
    buf_size = cfg ? cfg->size : 0;

    if (!mgr) {
        kmpp_loge_f("invalid param manager %px\n", mgr);
        return rk_nok;
    }

    if (!buf || buf_size < kmpp_shm_grp_size()) {
        impl = kmpp_calloc(sizeof(*impl));
        if (!impl) {
            kmpp_loge_f("malloc KmppShmGrp failed\n");
            return rk_nok;
        }
        impl->free_on_put = 1;
    } else {
        impl = (KmppShmGrpImpl *)buf;
        impl->free_on_put = 0;
    }

    impl->mgr = mgr;
    impl->file = file;
    impl->size = mgr->size;
    impl->sid = kmpp_shm_sid++;

    OSAL_INIT_LIST_HEAD(&impl->list_mgr);
    OSAL_INIT_LIST_HEAD(&impl->list_node);
    OSAL_INIT_LIST_HEAD(&impl->list_file);
    OSAL_INIT_LIST_HEAD(&impl->list_shm);

    osal_spinlock_assign(&impl->lock, (void *)(impl + 1), osal_spinlock_size());

    if (kmpp_shm_cls) {
        osal_spin_lock(kmpp_shm_cls->lock);
        osal_list_add_tail(&impl->list_mgr, &mgr->list_grp);
        osal_spin_unlock(kmpp_shm_cls->lock);
    }

    *grp = impl;

    return rk_ok;
}

rk_s32 kmpp_shm_grp_put(KmppShmGrp grp)
{
    KmppShmGrpImpl *impl = (KmppShmGrpImpl *)grp;
    KmppShmImpl *shm, *tmp;
    OSAL_LIST_HEAD(list);

    if (!impl) {
        kmpp_loge_f("invalid param grp %px\n", impl);
        return rk_nok;
    }

    if (kmpp_shm_cls) {
        osal_spin_lock(kmpp_shm_cls->lock);
        osal_list_del_init(&impl->list_mgr);
        osal_spin_unlock(kmpp_shm_cls->lock);
    }

    osal_list_for_each_entry_safe(shm, tmp, &impl->list_shm, KmppShmImpl, list_grp) {
        kmpp_logi_f("found still mapped shm %px\n", shm);
        kmpp_shm_put(shm);
    }

    osal_spinlock_deinit(&impl->lock);

    if (impl->free_on_put)
        kmpp_free(impl);

    return rk_ok;
}

rk_s32 kmpp_shm_grp_check(KmppShmGrp grp, KmppShmMgr mgr)
{
    KmppShmGrpImpl *impl = (KmppShmGrpImpl *)grp;

    if (!grp || !mgr) {
        kmpp_loge_f("invalid param grp %px mgr %px\n", grp, mgr);
        return rk_nok;
    }

    if (impl->mgr != mgr) {
        kmpp_loge_f("group %px is not belong to manager %px\n",
                    grp, mgr);
        return rk_nok;
    }

    return rk_ok;
}

rk_s32 kmpp_shm_get(KmppShm *shm, KmppShmGrp grp, KmppObj obj)
{
    KmppShmGrpImpl *impl_grp = (KmppShmGrpImpl *)grp;
    KmppShmImpl *impl_shm;
    rk_s32 entry_offset = kmpp_shm_entry_offset();
    rk_s32 shm_size;
    rk_s32 ret = rk_nok;

    if (!grp || !shm) {
        kmpp_loge_f("invalid param mgr %px shm %px\n", grp, shm);
        return ret;
    }

    *shm = NULL;
    shm_size = entry_offset + impl_grp->size;
    impl_shm = kmpp_malloc_share(shm_size);
    if (!impl_shm) {
        kmpp_loge_f("malloc KmppShm failed\n");
        return ret;
    }

    ret = osal_fs_vm_mmap(&impl_shm->vm_shm, impl_grp->file,
                          impl_shm, shm_size, KMPP_VM_PROT_RW);
    if (ret) {
        kmpp_loge_f("mmap %px size %d failed ret %d\n", impl_shm, shm_size, ret);
        kmpp_free_share(impl_shm);
        return ret;
    }

    OSAL_INIT_LIST_HEAD(&impl_shm->list_grp);
    impl_shm->grp = impl_grp;
    impl_shm->obj = obj;
    impl_shm->kbase = impl_shm;
    impl_shm->kaddr = (void *)(impl_shm + 1);
    impl_shm->ubase = impl_shm->vm_shm->uaddr;
    impl_shm->uaddr = impl_shm->ubase + entry_offset;

    impl_shm->ioc.kobj_uaddr = impl_shm->uaddr;
    impl_shm->ioc.kobj_kaddr = (__u64)impl_shm;

    shm_dbg_detail("shm %px kaddr [%px:%px] uaddr [%#lx:%#llx]\n",
                   impl_shm, impl_shm->kbase, impl_shm->kaddr,
                   impl_shm->ubase, impl_shm->uaddr);

    osal_spin_lock(impl_grp->lock);
    osal_list_add_tail(&impl_shm->list_grp, &impl_grp->list_shm);
    osal_spin_unlock(impl_grp->lock);

    *shm = impl_shm;

    return rk_ok;
}

rk_s32 kmpp_shm_put(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;
    KmppShmGrpImpl *grp;
    rk_s32 ret = rk_nok;

    if (!impl || !impl->grp) {
        kmpp_loge_f("invalid param shm %px grp %px\n", impl, impl ? impl->grp : NULL);
        return ret;
    }

    grp = impl->grp;
    osal_spin_lock(grp->lock);
    osal_list_del_init(&impl->list_grp);
    osal_spin_unlock(grp->lock);

    shm_dbg_detail("shm %px kaddr [%px:%px] uaddr [%#lx:%#llx]\n",
                   impl, impl->kbase, impl->kaddr,
                   impl->ubase, impl->uaddr);

    /* NOTE: when fd is closing do on munmap shm memory again */
    ret = osal_fs_vm_munmap(impl->vm_shm);
    if (ret) {
        kmpp_loge_f("munmap %px failed ret %d\n", impl->vm_shm, ret);
        return ret;
    }

    osal_memset(impl, 0, sizeof(*impl));

    kmpp_free_share(impl);

    return rk_ok;
}

void *kmpp_shm_get_kbase(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->kbase : NULL;
}

void *kmpp_shm_get_kaddr(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->kaddr : NULL;
}

rk_ul kmpp_shm_get_ubase(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->ubase : 0;
}

rk_u64 kmpp_shm_get_uaddr(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->uaddr : 0;
}

EXPORT_SYMBOL(kmpp_shm_mgr_get);
EXPORT_SYMBOL(kmpp_shm_mgr_bind_objdef);
EXPORT_SYMBOL(kmpp_shm_mgr_unbind_objdef);
EXPORT_SYMBOL(kmpp_shm_mgr_put);
EXPORT_SYMBOL(kmpp_shm_mgr_query);

EXPORT_SYMBOL(kmpp_shm_grp_size);
EXPORT_SYMBOL(kmpp_shm_grp_get);
EXPORT_SYMBOL(kmpp_shm_grp_put);
EXPORT_SYMBOL(kmpp_shm_grp_check);

EXPORT_SYMBOL(kmpp_shm_get);
EXPORT_SYMBOL(kmpp_shm_put);
EXPORT_SYMBOL(kmpp_shm_get_kbase);
EXPORT_SYMBOL(kmpp_shm_get_kaddr);
EXPORT_SYMBOL(kmpp_shm_get_ubase);
EXPORT_SYMBOL(kmpp_shm_get_uaddr);
