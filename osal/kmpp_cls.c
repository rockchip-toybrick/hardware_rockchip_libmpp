/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_class"

#include <linux/cdev.h>
#include <linux/of_platform.h>

#include "kmpp_macro.h"
#include "kmpp_env.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_dev_impl.h"
#include "kmpp_cls.h"
#include "kmpp_string.h"

#define KMPP_CLS_DBG_FLOW               (0x00000001)
#define KMPP_CLS_DBG_FOPS               (0x00000002)
#define KMPP_CLS_DBG_DEV                (0x00000004)
#define KMPP_CLS_DBG_DETAIL             (0x00000008)

#define kmpp_cls_dbg(flag, fmt, ...)    kmpp_dbg(osal_cls_debug, flag, fmt, ## __VA_ARGS__)
#define kmpp_cls_dbg_f(flag, fmt, ...)  kmpp_dbg_f(osal_cls_debug, flag, fmt, ## __VA_ARGS__)

#define kmpp_cls_dbg_flow(fmt, ...)     kmpp_cls_dbg_f(KMPP_CLS_DBG_FLOW, fmt, ## __VA_ARGS__)
#define kmpp_cls_dbg_fops(fmt, ...)     kmpp_cls_dbg_f(KMPP_CLS_DBG_FOPS, fmt, ## __VA_ARGS__)
#define kmpp_cls_dbg_dev(fmt, ...)      kmpp_cls_dbg_f(KMPP_CLS_DBG_DEV, fmt, ## __VA_ARGS__)
#define kmpp_cls_dbg_detail(fmt, ...)   kmpp_cls_dbg_f(KMPP_CLS_DBG_DETAIL, fmt, ## __VA_ARGS__)

//static rk_u32 osal_cls_debug = KMPP_CLS_DBG_FOPS | KMPP_CLS_DBG_FLOW;
static rk_u32 osal_cls_debug = 0;

rk_s32 osal_env_class_init(void)
{
    KmppEnvInfo info = {
        .type       = KmppEnv_u32,
        .readonly   = 0,
        .name       = "osal_cls_debug",
        .val        = &osal_cls_debug,
        .env_show   = NULL,
    };

    return kmpp_env_add(kmpp_env_debug, NULL, &info);
}

rk_s32 osal_class_size(const rk_u8 *name)
{
    return KMPP_ALIGN(osal_strlen(name) + 1, sizeof(rk_u64)) + sizeof(osal_cls_impl);
}
EXPORT_SYMBOL(osal_class_size);

rk_s32 osal_class_init(osal_cls **cls, const rk_u8 *name)
{
    osal_cls_impl *impl = NULL;
    rk_s32 ret = rk_nok;

    if (!cls || !name) {
        kmpp_loge_f("invalid param cls %px name %pn\n", cls, name);
        return ret;
    }

    do {
        rk_s32 str_len = ALIGN(osal_strlen(name) + 1, sizeof(rk_u64));
        rk_u8 *str_buf;

        impl = osal_kcalloc(sizeof(*impl) + str_len, osal_gfp_normal);
        if (!impl) {
            kmpp_loge_f("alloc fs class impl size %d failed\n", sizeof(*impl) + str_len);
            break;
        }

        impl->need_free = 1;
        str_buf = (rk_u8 *)(impl + 1);
        osal_strcpy(str_buf, name);
        impl->cls.name = str_buf;
        impl->class = class_create(THIS_MODULE, str_buf);
        if (PTR_ERR_OR_ZERO(impl->class)) {
            impl->class = NULL;
            kmpp_loge_f("create kmpp class failed\n");
            break;
        }

        ret = rk_ok;
    } while (0);

    if (ret && impl) {
        osal_kfree(impl);
        impl = NULL;
    }

    *cls = &impl->cls;

    return rk_ok;
}
EXPORT_SYMBOL(osal_class_init);

rk_s32 osal_class_assign(osal_cls **cls, const rk_u8 *name, void *buf, rk_s32 size)
{
    osal_cls_impl *impl = NULL;
    rk_s32 require_size;
    rk_s32 ret = rk_nok;

    if (!cls || !name || !buf || size <= 0) {
        kmpp_loge_f("invalid param cls %px name %px buf %px size %d\n",
                    cls, name, buf, size);
        return ret;
    }

    require_size = osal_class_size(name);
    if (size < require_size) {
        kmpp_loge_f("invalid param size %d require size %d\n", size, require_size);
        return ret;
    }

    impl = buf;

    do {
        rk_u8 *str_buf = (rk_u8 *)(impl + 1);

        osal_strcpy(str_buf, name);
        impl->cls.name = str_buf;
        impl->class = class_create(THIS_MODULE, str_buf);
        if (PTR_ERR_OR_ZERO(impl->class)) {
            impl->class = NULL;
            kmpp_loge_f("create kmpp class failed\n");
            break;
        }

        ret = rk_ok;
    } while (0);

    *cls = ret ? NULL : &impl->cls;

    return ret;
}
EXPORT_SYMBOL(osal_class_assign);

rk_s32 osal_class_deinit(osal_cls *cls)
{
    if (cls) {
        osal_cls_impl *impl = cls_to_impl(cls);

        class_destroy(impl->class);

        if (impl->need_free)
            osal_kfree(impl);
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_class_deinit);

/* memory layout: osal_fs_dev_impl is before struct cdev */
static int osal_fs_open(struct inode *inode, struct file *file)
{
    osal_fs_dev_mgr *mgr = ((void *)inode->i_cdev) - sizeof(*mgr);
    osal_fs_dev *dev = NULL;
    rk_s32 ret = rk_nok;

    dev = kmpp_calloc(sizeof(osal_fs_dev) + mgr->priv_size);
    if (!dev) {
        kmpp_loge_f("failed to create private_data size %d\n", mgr->priv_size);
        return rk_nok;
    }

    dev->name = mgr->name;
    dev->mgr = mgr;
    dev->drv_data = mgr->drv_data;
    dev->file_data = file;
    dev->priv_data = (void *)(dev + 1);

    kmpp_cls_dbg_fops("%s mgr %px drv_data %px priv %px\n",
                      dev->name, dev->drv_data, dev->priv_data);

    if (mgr->fops.open) {
        ret = mgr->fops.open(dev);
        if (ret) {
            kmpp_free(dev);
            return ret;
        }
    }

    file->private_data = dev;

    return nonseekable_open(inode, file);
}

static int osal_fs_release(struct inode *inode, struct file *file)
{
    osal_fs_dev *dev = file->private_data;
    osal_fs_dev_mgr *mgr = dev->mgr;
    rk_s32 ret = rk_nok;

    file->private_data = NULL;

    kmpp_cls_dbg_fops("%s mgr %px drv_data %px priv %px\n", dev->name,
                      mgr, mgr->drv_data, dev->priv_data);

    if (mgr->fops.release)
        ret = mgr->fops.release(dev);

    kmpp_free(dev);

    return ret;
}

static long osal_fs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    osal_fs_dev *dev = file->private_data;
    osal_fs_dev_mgr *mgr = dev->mgr;
    rk_s32 ret = rk_nok;

    kmpp_cls_dbg_fops("%s priv %px cmd %x arg %px\n",
                      dev->name, dev->priv_data, cmd, arg);

    if (mgr->fops.ioctl)
        ret = mgr->fops.ioctl(dev, cmd, (void *)arg);

    return ret;
}

static int osal_fs_mmap(struct file *file, struct vm_area_struct *vma)
{
    osal_fs_dev *dev = file->private_data;
    osal_fs_dev_mgr *mgr = dev->mgr;
    rk_s32 ret = rk_nok;

    kmpp_cls_dbg_fops("%s priv %px start %lx end %lx\n",
                      dev->name, dev->priv_data, vma->vm_start, vma->vm_end);

    if (mgr->fops.mmap)
        ret = mgr->fops.mmap(dev, vma->vm_start, vma->vm_end);

    return ret;
}

static struct file_operations osal_fs_dev_fops = {
    .owner          = THIS_MODULE,
    .open           = osal_fs_open,
    .release        = osal_fs_release,
    .unlocked_ioctl = osal_fs_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = osal_fs_ioctl,
#endif
    .mmap           = osal_fs_mmap,
};

rk_s32 osal_fs_dev_mgr_size(const rk_u8 *name)
{
    return sizeof(osal_fs_dev_mgr) + sizeof(struct cdev) +
           KMPP_ALIGN(osal_strlen(name) + 1, sizeof(rk_u64));
}
EXPORT_SYMBOL(osal_fs_dev_mgr_size);

rk_s32 osal_fs_dev_mgr_init(osal_fs_dev_mgr **mgr, osal_fs_dev_cfg *cfg)
{
    osal_fs_dev_mgr *impl = NULL;
    osal_dev_fops *fops = NULL;
    const rk_u8 *name = NULL;
    osal_cls *cls = NULL;
    rk_s32 total_size = 0;
    rk_u8 *name_buf = NULL;
    rk_s32 ret = rk_nok;

    if (!mgr || !cfg || !cfg->cls || !cfg->name) {
        kmpp_loge_f("can not init fs device with NULL class or NULL name\n");
        goto done;
    }

    cls = cfg->cls;
    name = cfg->name;
    fops = cfg->fops;

    total_size = osal_fs_dev_mgr_size(name);
    if (!cfg->buf || cfg->size < total_size) {
        impl = kmpp_calloc(total_size);
        if (!impl) {
            kmpp_loge_f("alloc dev fs size %d failed\n", total_size);
            goto done;
        }
        impl->need_free = 1;
    } else {
        impl = (osal_fs_dev_mgr *)cfg->buf;
    }

    impl->cls = cls_to_impl(cls)->class;
    impl->cdev_size = sizeof(struct cdev);
    impl->cdev = (struct cdev *)(impl + 1);
    impl->fops = *fops;
    impl->drv_data = cfg->drv_data;
    impl->priv_size = cfg->priv_size;
    name_buf = (rk_u8 *)(impl->cdev + 1);
    cfg->name = name_buf;

    ret = alloc_chrdev_region(&impl->dev_id, 0, 1, name);
    if (ret) {
        kmpp_loge_f("alloc dev id failed\n");
        goto done;
    }

    osal_strcpy(name_buf, name);
    impl->name = name_buf;

    cdev_init(impl->cdev, &osal_fs_dev_fops);
    impl->cdev->owner = THIS_MODULE;

    ret = cdev_add(impl->cdev, impl->dev_id, 1);
    if (ret) {
        kmpp_loge_f("add cdev failed\n");
        goto done;
    }

    impl->dev = device_create(impl->cls, NULL, impl->dev_id, NULL, name_buf);
    if (PTR_ERR_OR_ZERO(impl->dev)) {
        kmpp_loge_f("device create failed\n");
        goto done;
    }

done:
    if (ret && impl) {
        osal_fs_dev_mgr_deinit(impl);
        impl = NULL;
    }

    *mgr = impl;

    kmpp_cls_dbg_flow("name %s ret %px\n", cfg->name, impl);

    return ret;
}
EXPORT_SYMBOL(osal_fs_dev_mgr_init);

rk_s32 osal_fs_dev_mgr_deinit(osal_fs_dev_mgr *mgr)
{
    if (mgr) {
        if (mgr->cdev) {
            cdev_del(mgr->cdev);
            mgr->cdev = NULL;
        }

        if (mgr->dev_id) {
            device_destroy(mgr->cls, mgr->dev_id);
            unregister_chrdev_region(mgr->dev_id, 1);
        }

        if (mgr->need_free)
            osal_kfree(mgr);
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_fs_dev_mgr_deinit);
