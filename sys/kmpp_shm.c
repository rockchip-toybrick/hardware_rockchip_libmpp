/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "kmpp_shm"

#include <asm/ioctl.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/export.h>

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
#define KMPP_SHM_IOC_RELEASE_INFO   _IOW(KMPP_SHM_IOC_MAGIC, 2, unsigned int)
#define KMPP_SHM_IOC_GET_SHM        _IOW(KMPP_SHM_IOC_MAGIC, 3, unsigned int)
#define KMPP_SHM_IOC_PUT_SHM        _IOW(KMPP_SHM_IOC_MAGIC, 4, unsigned int)
#define KMPP_SHM_IOC_DUMP           _IOW(KMPP_SHM_IOC_MAGIC, 5, unsigned int)

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
typedef struct KmppShmMgr_t {
    osal_spinlock   *lock;
    /* list to list_mgr in KmppShmClsImpl */
    osal_list_head  list_cls;
    /* list for list_mgr in KmppShmGrpRoot */
    osal_list_head  list_root;
    osal_fs_dev_mgr *mgr;
    const rk_u8     *name;
} KmppShmMgr;

/* kmpp share memory group for buffer fast access */
typedef struct KmppShmGrpImpl_t {
    osal_spinlock   *lock;
    osal_fs_dev     *file;
    KmppObjDef      def;

    /* list for list_shm in KmppShmGrpImpl */
    osal_list_head  list_shm;
    rk_s32          shm_count;
    /* objdef entry size */
    rk_s32          size;
    /* objdef name string offset from the share buffer base address */
    rk_s32          name_offset;
    /* objdef index in all the objdefs */
    rk_s32          index;
} KmppShmGrpImpl;

/* kmpp share memory group root each file handle can has a group root */
typedef struct KmppShmGrpRoot_t {
    osal_spinlock   *lock;
    /* list to list_shm in KmppShmMgr */
    osal_list_head  list_mgr;
    /* list for list_file in KmppShmGrpImpl */
    osal_list_head  list_grp;

    /* device file infomation link */
    osal_fs_dev     *file;
    KmppShmMgr      *mgr;

    /* object defines set info */
    KmppObjDefSet   *defs;
    osal_fs_vm      *vm_trie;
    void            *buf;
    rk_s32          buf_size;

    /* session id for pid in system */
    rk_u32          sid;
    /* pid for find valid process share memory allocator */
    rk_s32          pid;

    KmppShmGrpImpl  *grps;
} KmppShmGrpRoot;

typedef struct KmppShmImpl_t {
    KmppObjShm      ioc;

    /* reserve for userspace writing */
    rk_u64          upriv;
    rk_u64          ubase;

    /* share memory name offset from the share trie root */
    rk_u32          name_offset;
    rk_u32          reserved;

    /* kernel access address */
    void            *kpriv;
    void            *kbase;

    osal_fs_vm      *vm_shm;
    KmppShmGrpImpl  *grp;

    /* list to list_shm in KmppShmGrpImpl */
    osal_list_head  list_grp;
} KmppShmImpl;

static KmppShmClsImpl *kmpp_shm_cls = NULL;
static const rk_u8 kmpp_shm_name[] = "kmpp_shm";
static KmppEnvNode kmpp_shm_env = NULL;
static KmppShmMgr *kmpp_shm_objs = NULL;
static rk_u32 kmpp_shm_root_sid = 0;
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

void size_to_map_info(KmppShmMapSize *info, rk_s32 size)
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

rk_s32 kmpp_shm_open(osal_fs_dev *file)
{
    KmppShmMgr *mgr = (KmppShmMgr *)file->drv_data;
    KmppShmGrpRoot *root = (KmppShmGrpRoot *)file->priv_data;
    KmppObjDefSet *defs = NULL;

    shm_dbg_flow("enter - %s\n", mgr->name);

    kmpp_objdef_get_shared(&defs);

    if (!defs) {
        kmpp_loge_f("failed to get shared objdef set\n");
        return rk_nok;
    }

    osal_spinlock_assign(&root->lock, (void *)(root + 1), osal_spinlock_size());

    root->sid = kmpp_shm_root_sid++;
    root->pid = osal_getpid();

    OSAL_INIT_LIST_HEAD(&root->list_mgr);
    OSAL_INIT_LIST_HEAD(&root->list_grp);

    root->file = file;
    root->mgr = mgr;

    root->defs = defs;
    root->buf = kmpp_trie_get_node_root(defs->trie);
    root->buf_size = defs->buf_size;

    if (defs->count) {
        KmppShmGrpImpl *grps = kmpp_calloc(defs->count * sizeof(KmppShmGrpImpl));
        KmppTrieInfo *info;
        rk_s32 i;

        if (!grps) {
            kmpp_loge_f("failed to alloc shm group array\n");
            return rk_nok;
        }

        root->grps = grps;
        info = kmpp_trie_get_info_first(defs->trie);

        for (i = 0; i < defs->count; i++) {
            KmppShmGrpImpl *grp = &grps[i];
            const rk_u8 *name = kmpp_trie_info_name(info);

            grp->lock = root->lock;
            grp->file = file;
            grp->def = defs->defs[i];
            OSAL_INIT_LIST_HEAD(&grp->list_shm);
            grp->size = kmpp_objdef_get_entry_size(grp->def);
            grp->index = i;
            grp->name_offset = name - (rk_u8 *)root->buf;

            info = kmpp_trie_get_info_next(defs->trie, info);
        }
    }

    if (kmpp_shm_cls) {
        osal_spin_lock(kmpp_shm_cls->lock);
        osal_list_add_tail(&root->list_mgr, &mgr->list_root);
        osal_spin_unlock(kmpp_shm_cls->lock);
    }

    shm_dbg_flow("leave - %s sid %d\n", mgr->name, root->sid);

    return rk_ok;
}

static rk_s32 kmpp_shm_grp_root_deinit(KmppShmGrpRoot *root)
{
    KmppObjDefSet *defs = root->defs;
    KmppShmMgr *mgr = root->mgr;

    osal_spin_lock(mgr->lock);
    osal_list_del_init(&root->list_mgr);
    osal_spin_unlock(mgr->lock);

    if (root->vm_trie) {
        osal_fs_vm_munmap(root->vm_trie);
        root->vm_trie = NULL;
    }

    if (defs) {
        KmppShmGrpImpl *grp;
        KmppShmImpl *shm, *n;
        rk_s32 i;

        for (i = 0; i < defs->count; i++) {
            OSAL_LIST_HEAD(list);

            grp = &root->grps[i];

            osal_spin_lock(root->lock);
            osal_list_for_each_entry_safe(shm, n, &grp->list_shm, KmppShmImpl, list_grp) {
                osal_list_move(&shm->list_grp, &list);
            }
            osal_spin_unlock(root->lock);

            osal_list_for_each_entry_safe(shm, n, &list, KmppShmImpl, list_grp) {
                kmpp_shm_put(shm);
            }
        }

        kmpp_objdef_put_shared(defs);
    }

    kmpp_free(root->grps);

    osal_spinlock_deinit(&root->lock);

    return rk_ok;
}

rk_s32 kmpp_shm_release(osal_fs_dev *file)
{
    KmppShmMgr *mgr = (KmppShmMgr *)file->drv_data;
    KmppShmGrpRoot *root = (KmppShmGrpRoot *)file->priv_data;
    rk_s32 ret;

    shm_dbg_flow("enter - %s root %px\n", mgr->name, root);

    ret = kmpp_shm_grp_root_deinit(root);

    shm_dbg_flow("leave - %s\n", mgr->name);

    return ret;
}

static rk_s32 get_usr_str(rk_u8 *dst, rk_s32 size, void *src, const rk_u8 *log)
{
    rk_u64 name_uaddr;
    rk_s32 ret;

    ret = osal_copy_from_user(&name_uaddr, src, sizeof(name_uaddr));
    if (ret) {
        kmpp_loge_f("by %s copy userspace address %px fail ret %d\n", log, src, ret);
        return ret;
    }

    ret = osal_strncpy_from_user(dst, (void *)(uintptr_t)name_uaddr, size);
    if (ret <= 0) {
        kmpp_loge_f("by %s copy userspace string from %#llx len %d fail ret %d\n",
                    log, name_uaddr, size, ret);
        return ret;
    }

    return ret;
}

static KmppShmGrpImpl *get_shm_grp(osal_fs_dev *file, const rk_u8 *name, const rk_u8 *log)
{
    KmppShmGrpRoot *root = (KmppShmGrpRoot *)file->priv_data;
    KmppObjDefSet *defs = root->defs;
    KmppTrieInfo *info;

    if (!defs || !defs->trie) {
        kmpp_loge_f("by %s invalid grp root %px defs %px trie %px\n",
                    log, root, defs, defs ? defs->trie : NULL);
        return NULL;
    }

    info = kmpp_trie_get_info(defs->trie, name);
    if (!info) {
        kmpp_loge_f("by %s can not found valid objdef %s\n", log, name);
        return NULL;
    }

    if (info->index >= defs->count) {
        kmpp_loge_f("by %s invalid objdef index %d max %d\n",
                    log, info->index, defs->count);
        return NULL;
    }

    return &root->grps[info->index];
}

rk_s32 kmpp_shm_ioctl(osal_fs_dev *file, rk_s32 cmd, void *arg)
{
    KmppShmMgr *mgr = (KmppShmMgr *)file->drv_data;
    rk_s32 ret = rk_nok;

    shm_dbg_flow("enter - %s cmd %#x arg %px\n", mgr->name, cmd, arg);

    switch (cmd) {
    case KMPP_SHM_IOC_QUERY_INFO : {
        KmppShmGrpRoot *root = (KmppShmGrpRoot *)file->priv_data;
        rk_u64 uaddr;

        if (!root->vm_trie) {
            rk_s32 buf_size = PAGE_ALIGN(root->buf_size);

            ret = osal_fs_vm_mmap(&root->vm_trie, file, root->buf,
                                  buf_size, KMPP_VM_PROT_RO);
            if (ret || !root->vm_trie) {
                kmpp_loge_f("vm_mmap fail ret %d %px\n", ret, root->vm_trie);
                break;
            }
        }

        uaddr = (rk_u64)root->vm_trie->uaddr;

        shm_dbg_ioctl("query obj trie root %#llx\n", uaddr);

        ret = osal_copy_to_user(arg, &uaddr, sizeof(uaddr));
        if (ret)
            kmpp_loge_f("KMPP_SHM_IOC_QUERY_INFO osal_copy_to_user fail ret %d\n", ret);
    } break;
    case KMPP_SHM_IOC_RELEASE_INFO : {
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

        for (i = 0; i < ioc.count; i++, arg += sizeof(KmppShmPtr)) {
            rk_u8 name[64];
            KmppShmGrpImpl *grp = NULL;
            KmppShmPtr sptr;
            KmppObj obj = NULL;

            shm_dbg_ioctl("get shm ioc %d\n", i);

            ret = get_usr_str(name, sizeof(name) - 1, arg, "KMPP_SHM_IOC_GET_SHM");
            if (ret < 0)
                break;

            grp = get_shm_grp(file, name, "KMPP_SHM_IOC_GET_SHM");
            if (!grp) {
                kmpp_loge_f("slot %d get_shm_grp %s failed ret %d\n", i, name, ret);
                ret = rk_nok;
                break;
            }

            ret = kmpp_obj_get_share_f(&obj, grp->def, file);
            if (ret || !obj)
                kmpp_loge_f("slot %d kmpp_obj_get_share fail ret %d\n", i, ret);

            if (obj && (kmpp_shm_debug & SHM_DBG_DUMP))
                kmpp_obj_dump(obj, "KMPP_SHM_IOC_GET_SHM");

            kmpp_obj_to_shmptr(obj, &sptr);

            shm_dbg_ioctl("slot %d get shm [u:k] %#llx :%#px\n", i, sptr.uaddr, sptr.kptr);

            if (osal_copy_to_user(arg, &sptr, sizeof(sptr))) {
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

            ret = osal_copy_from_user(&shm, (void *)(uintptr_t)uaddr, sizeof(shm));
            if (ret) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_PUT_SHM osal_copy_from_user KmppObjShm fail\n", i);
                break;
            }

            impl = (KmppShmImpl *)(uintptr_t)shm.kobj_kaddr;
            if (!impl || impl != impl->kbase || impl->ubase != shm.kobj_uaddr) {
                kmpp_loge_f("slot %d KMPP_SHM_IOC_PUT_SHM invalid shm %px - kbase %px ubase %#llx - %#llx\n",
                            i, impl, impl ? impl->kbase : NULL,
                            impl ? impl->ubase : 0, shm.kobj_uaddr);
                break;
            }

            shm_dbg_ioctl("slot %d put shm k:%px u:%#llx\n", i, impl->kbase, impl->ubase);

            if (impl->kpriv) {
                if (kmpp_shm_debug & SHM_DBG_DUMP)
                    kmpp_obj_dump(impl->kpriv, "KMPP_SHM_IOC_PUT_SHM");

                ret = kmpp_obj_put_f(impl->kpriv);
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
            KmppShmImpl *impl = (KmppShmImpl *)(uintptr_t)ioc.kobj_kaddr;

            if (!impl || impl != impl->kbase) {
                kmpp_loge_f("KMPP_SHM_IOC_DUMP invalid shm %px\n", impl);
                break;
            }

            if (impl->kpriv)
                ret = kmpp_obj_dump(impl->kpriv, "KMPP_SHM_IOC_DUMP");
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

static rk_s32 kmpp_shm_grp_root_size(void)
{
    return sizeof(KmppShmGrpRoot) + osal_spinlock_size();
}

rk_s32 kmpp_shm_mgr_put(KmppShmMgr *mgr)
{
    KmppShmGrpRoot *root, *n;
    OSAL_LIST_HEAD(list_root);

    if (!mgr) {
        kmpp_loge_f("invalid param mgr %px\n", mgr);
        return rk_nok;
    }

    if (kmpp_shm_cls) {
        osal_spin_lock(kmpp_shm_cls->lock);
        osal_list_del_init(&mgr->list_cls);
        osal_spin_unlock(kmpp_shm_cls->lock);
    } else {
        osal_list_del_init(&mgr->list_cls);
    }

    osal_spin_lock(mgr->lock);
    osal_list_for_each_entry_safe(root, n, &mgr->list_root, KmppShmGrpRoot, list_mgr) {
        osal_list_move_tail(&root->list_mgr, &list_root);
    }
    osal_spin_unlock(mgr->lock);

    osal_list_for_each_entry_safe(root, n, &list_root, KmppShmGrpRoot, list_mgr) {
        kmpp_shm_grp_root_deinit(root);
    }

    osal_spinlock_deinit(&mgr->lock);

    if (mgr->mgr) {
        osal_fs_dev_mgr_deinit(mgr->mgr);
        mgr->mgr = NULL;
    }

    kmpp_free(mgr);

    return rk_ok;
}

rk_s32 kmpp_shm_mgr_get(KmppShmMgr **mgr, const rk_u8 *name)
{
    KmppShmMgr *impl = NULL;
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
    cfg.priv_size = kmpp_shm_grp_root_size();
    cfg.buf = (void *)((rk_u8 *)(impl + 1) + lock_size);
    cfg.size = mgr_size;

    ret = osal_fs_dev_mgr_init(&impl->mgr, &cfg);
    if (ret || !impl->mgr) {
        kmpp_loge_f("dev mgr %s init failed\n", name);
        goto done;
    } else {
        impl->name = cfg.name;

        OSAL_INIT_LIST_HEAD(&impl->list_cls);
        OSAL_INIT_LIST_HEAD(&impl->list_root);

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
    KmppShmMgr *mgr, *n;
    OSAL_LIST_HEAD(list);

    if (!cls)
        return rk_ok;

    osal_spin_lock(cls->lock);
    osal_list_for_each_entry_safe(mgr, n, &cls->list_mgr, KmppShmMgr, list_cls) {
        osal_list_move_tail(&mgr->list_cls, &list);
    }
    osal_spin_unlock(cls->lock);

    osal_list_for_each_entry_safe(mgr, n, &list, KmppShmMgr, list_cls) {
        kmpp_shm_mgr_put(mgr);
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

rk_s32 kmpp_shm_add_trie_info(KmppTrie trie)
{
    rk_s32 val = sizeof(KmppShmImpl);

    kmpp_trie_add_info(trie, "__offset", &val, sizeof(val));

    /* add userspace private info position */
    val = offsetof(KmppShmImpl, upriv);
    kmpp_trie_add_info(trie, "__priv", &val, sizeof(val));

    /* add name offset from the objdef share root */
    val = offsetof(KmppShmImpl, name_offset);
    kmpp_trie_add_info(trie, "__name_offset", &val, sizeof(val));

    return rk_ok;
}

static inline rk_s32 kmpp_shm_entry_offset(void)
{
    return sizeof(KmppShmImpl);
}

static inline KmppShmGrpRoot *get_grp_root(KmppShmGrp grp)
{
    KmppShmGrpImpl *impl = (KmppShmGrpImpl *)grp;
    osal_fs_dev *file = impl ? impl->file : NULL;
    KmppShmGrpRoot *root = file ? (KmppShmGrpRoot *)file->priv_data : NULL;

    return root;
}

osal_fs_dev *kmpp_shm_get_objs_file(void)
{
    KmppShmClsImpl *cls = kmpp_shm_cls;
    KmppShmMgr *mgr = kmpp_shm_objs;
    osal_fs_dev *file = NULL;

    if (cls && mgr) {
        KmppShmGrpRoot *root, *n;
        rk_s32 pid = osal_getpid();

        osal_spin_lock(cls->lock);
        osal_list_for_each_entry_safe(root, n, &mgr->list_root, KmppShmGrpRoot, list_mgr) {
            if (root->pid == pid) {
                file = root->file;
                kmpp_logi_f("find root %px pid %d file %px\n", root, pid, file);
                break;
            }
        }
        osal_spin_unlock(cls->lock);
    }

    return file;
}

rk_s32 kmpp_shm_get(KmppShm *shm, osal_fs_dev *file, const rk_u8 *name)
{
    KmppShmImpl *impl;
    rk_s32 entry_offset;
    rk_s32 shm_size;
    rk_s32 ret = rk_nok;
    KmppShmGrpImpl *grp = get_shm_grp(file, name, __func__);;

    if (!shm || !grp || !name) {
        kmpp_loge_f("invalid param shm %px grp %px name %s\n", shm, grp, name);
        return ret;
    }

    *shm = NULL;
    entry_offset = kmpp_shm_entry_offset();
    shm_size = entry_offset + grp->size;
    impl = kmpp_malloc_share(shm_size);
    if (!impl) {
        kmpp_loge_f("malloc KmppShm failed\n");
        return ret;
    }

    ret = osal_fs_vm_mmap(&impl->vm_shm, grp->file,
                          impl, shm_size, KMPP_VM_PROT_RW);
    if (ret) {
        kmpp_loge_f("mmap %px size %d failed ret %d\n", impl, shm_size, ret);
        kmpp_free_share(impl);
        return ret;
    }

    OSAL_INIT_LIST_HEAD(&impl->list_grp);
    impl->grp = grp;
    impl->kpriv = NULL;
    impl->kbase = impl;
    impl->ubase = impl->vm_shm->uaddr;
    impl->name_offset = grp->name_offset;

    impl->ioc.kobj_uaddr = impl->ubase;
    impl->ioc.kobj_kaddr = (__u64)(uintptr_t)impl;

    shm_dbg_detail("shm %px kaddr [%px:%px] uaddr [%#lx:%#llx]\n",
                   impl, impl->kbase, impl->kbase + entry_offset,
                   impl->ubase, impl->ubase + entry_offset);

    osal_spin_lock(grp->lock);
    osal_list_add_tail(&impl->list_grp, &grp->list_shm);
    osal_spin_unlock(grp->lock);

    *shm = impl;

    return rk_ok;
}

rk_s32 kmpp_shm_put(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;
    KmppShmGrpImpl *grp = impl ? impl->grp : NULL;
    KmppShmGrpRoot *root = get_grp_root(grp);
    rk_s32 ret = rk_nok;

    if (!impl || ! grp || !root) {
        kmpp_loge_f("invalid param shm %px grp %px root %px\n", impl, grp, root);
        return ret;
    }

    osal_spin_lock(root->lock);
    osal_list_del_init(&impl->list_grp);
    osal_spin_unlock(root->lock);

    shm_dbg_detail("shm %px kaddr [%px:%px] uaddr [%#lx:%#llx]\n",
                   impl, impl->kbase, impl->kbase + kmpp_shm_entry_offset(),
                   impl->ubase, impl->ubase + kmpp_shm_entry_offset());

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

KmppShmPtr *kmpp_shm_to_shmptr(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    shm_dbg_detail("shm %px to shmptr [u:k] %#lx:%#llx\n",
                   impl, impl->ioc.kobj_uaddr, impl->ioc.kobj_kaddr);

    return impl ? (KmppShmPtr *)&impl->ioc : NULL;
}

KmppShm kmpp_shm_from_shmptr(KmppShmPtr *sptr)
{
    return sptr ? (KmppShm)sptr->kptr : NULL;
}

void *kmpp_shm_get_kbase(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->kbase : NULL;
}

void *kmpp_shm_get_kaddr(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl && impl->kbase ? impl->kbase + kmpp_shm_entry_offset() : NULL;
}

rk_ul kmpp_shm_get_ubase(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->ubase : 0;
}

rk_u64 kmpp_shm_get_uaddr(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl && impl->kbase ? impl->ubase + kmpp_shm_entry_offset() : 0;
}

rk_s32 kmpp_shm_set_kpriv(KmppShm shm, void *kpriv)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    if (impl) {
        impl->kpriv = kpriv;
        return rk_ok;
    }

    return rk_nok;
}

void *kmpp_shm_get_kpriv(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? impl->kpriv : NULL;
}

rk_u64 kmpp_shm_get_upriv(KmppShm shm)
{
    KmppShmImpl *impl = (KmppShmImpl *)shm;

    return impl ? (rk_u64)impl->upriv : 0;
}

rk_s32 kmpp_shm_get_entry_size(KmppShm shm)
{
    if (shm) {
        KmppShmImpl *impl = (KmppShmImpl *)shm;

        if (impl->grp)
            return impl->grp->size;
    }

    kmpp_loge_f("invalid param shm %px\n", shm);

    return 0;
}

EXPORT_SYMBOL(kmpp_shm_get);
EXPORT_SYMBOL(kmpp_shm_put);

EXPORT_SYMBOL(kmpp_shm_to_shmptr);
EXPORT_SYMBOL(kmpp_shm_from_shmptr);

EXPORT_SYMBOL(kmpp_shm_get_kbase);
EXPORT_SYMBOL(kmpp_shm_get_kaddr);
EXPORT_SYMBOL(kmpp_shm_get_ubase);
EXPORT_SYMBOL(kmpp_shm_get_uaddr);

EXPORT_SYMBOL(kmpp_shm_set_kpriv);
EXPORT_SYMBOL(kmpp_shm_get_kpriv);
EXPORT_SYMBOL(kmpp_shm_get_upriv);
EXPORT_SYMBOL(kmpp_shm_get_entry_size);
