/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_dmabuf"

#include <linux/dma-buf.h>
#include <linux/scatterlist.h>

#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_dmabuf.h"
#include "kmpp_dev_impl.h"

typedef struct osal_dmabuf_impl_t {
    osal_dmabuf buf;
    osal_list_head list_srv;
    osal_list_head list_dev;
    struct sg_table *sgt;
    osal_dev *orig_dev;
    rk_s32 is_attachment;
} osal_dmabuf_impl;

#define osal_dmabuf_to_impl(dmabuf) container_of(dmabuf, osal_dmabuf_impl, buf)

rk_s32 osal_dmabuf_mgr_init(void)
{
    return rk_ok;
}

rk_s32 osal_dmabuf_mgr_deinit(void)
{
    return rk_ok;
}

rk_s32 osal_dmabuf_alloc(osal_dmabuf **dmabuf, osal_dev *dev, rk_s64 size, rk_u32 flag)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    osal_dmabuf_impl *buf = NULL;
    struct sg_table *sgt = NULL;
    dma_addr_t daddr = 0;
    void *kaddr = NULL;
    rk_s32 ret;

    buf = osal_kcalloc(sizeof(osal_dmabuf_impl) + sizeof(struct sg_table), GFP_KERNEL);
    if (!buf) {
        kmpp_loge_f("alloc dmabuf struct failed\n");
        goto failed;
    }

    sgt = (struct sg_table *)(buf + 1);

    kaddr = dma_alloc_attrs(impl->device, size, &daddr, GFP_KERNEL, flag | DMA_ATTR_NO_WARN);
    if (IS_ERR_OR_NULL(kaddr)) {
        kmpp_loge_f("alloc dmabuf size %d failed %d\n", size, PTR_ERR(kaddr));
        goto failed;
    }

    ret = dma_get_sgtable_attrs(impl->device, sgt, kaddr, daddr, size, 0);
    if (ret) {
        kmpp_loge_f("get sgt for %px:%pad size %d failed %d\n", kaddr, &daddr, size, ret);
        goto failed;
    }

    OSAL_INIT_LIST_HEAD(&buf->list_srv);
    OSAL_INIT_LIST_HEAD(&buf->list_dev);

    buf->buf.kaddr = kaddr;
    buf->buf.daddr = daddr;
    buf->buf.size = size;
    buf->buf.dev = dev;
    buf->sgt = sgt;
    buf->orig_dev = dev;
    buf->is_attachment = 0;

    *dmabuf = &buf->buf;

    return rk_ok;

failed:
    if (buf)
        osal_kfree(buf);

    *dmabuf = NULL;

    return rk_nok;
}
EXPORT_SYMBOL(osal_dmabuf_alloc);

rk_s32 osal_dmabuf_free(osal_dmabuf *dmabuf)
{
    if (dmabuf) {
        osal_dmabuf_impl *buf = osal_dmabuf_to_impl(dmabuf);
        osal_dev_impl *dev = dev_to_impl(dmabuf->dev);

        dma_free_attrs(dev->device, dmabuf->size, dmabuf->kaddr, dmabuf->daddr, DMA_ATTR_NO_WARN);
        osal_kfree(buf);
        return rk_ok;
    }
    return rk_nok;
}
EXPORT_SYMBOL(osal_dmabuf_free);

rk_s32 osal_dmabuf_sync_for_cpu(osal_dmabuf *dmabuf, osal_dma_direction dir)
{
    if (dmabuf) {
        osal_dmabuf_impl *buf = osal_dmabuf_to_impl(dmabuf);
        osal_dev_impl *impl = dev_to_impl(dmabuf->dev);

        dma_sync_sg_for_cpu(impl->device, buf->sgt->sgl, buf->sgt->nents, (enum dma_data_direction)dir);
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_dmabuf_sync_for_cpu);

rk_s32 osal_dmabuf_sync_for_dev(osal_dmabuf *dmabuf, osal_dma_direction dir)
{
    if (dmabuf) {
        osal_dmabuf_impl *buf = osal_dmabuf_to_impl(dmabuf);
        osal_dev_impl *impl = dev_to_impl(dmabuf->dev);

        dma_sync_sg_for_device(impl->device, buf->sgt->sgl, buf->sgt->nents, (enum dma_data_direction)dir);
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_dmabuf_sync_for_dev);

rk_s32 osal_dmabuf_attach(osal_dmabuf **dst, osal_dmabuf *src, osal_dev *dev)
{
    osal_dev_impl *impl_dev;
    osal_dmabuf_impl *impl_dst;
    osal_dmabuf_impl *impl_src;
    struct sg_table *sgt;
    rk_s32 ret;
    rk_s32 i;

    if (!dst || !src || !dev) {
        kmpp_loge_f("invalid param dst %px src %px dev %px\n", dst, src, dev);
        return rk_nok;
    }

    impl_src = osal_dmabuf_to_impl(src);
    impl_dev = dev_to_impl(dev);

    *dst = NULL;
    if (src->dev == dev) {
        kmpp_loge_f("can not attach buffer %px to same device %s again\n", src, dev->name);
        return rk_nok;
    }

    impl_dst = osal_kcalloc(sizeof(osal_dmabuf_impl) + sizeof(struct sg_table), GFP_KERNEL);
    if (!impl_dst) {
        kmpp_loge_f("alloc dmabuf struct failed\n");
        goto failed;
    }

    impl_dst->sgt = (struct sg_table *)(impl_dst + 1);
    sgt = impl_dst->sgt;

    ret = sg_alloc_table(sgt, impl_src->sgt->orig_nents, GFP_KERNEL);
    if (ret) {
        kmpp_loge_f("alloc sg table failed\n");
        goto failed;
    }

    {
        struct scatterlist *sg, *new_sg;

        new_sg = sgt->sgl;
        for_each_sgtable_sg(sgt, sg, i) {
                sg_set_page(new_sg, sg_page(sg), sg->length, sg->offset);
                new_sg = sg_next(new_sg);
        }
    }

    ret = dma_map_sgtable(impl_dev->device, sgt, DMA_BIDIRECTIONAL, 0);
    if (ret) {
        kmpp_loge_f("dma_map_sgtable failed %d\n", ret);
        goto failed;
    }

    impl_dst->buf.kaddr = impl_src->buf.kaddr;
    impl_dst->buf.daddr = sg_dma_address(sgt->sgl);;
    impl_dst->buf.size = impl_src->buf.size;
    impl_dst->buf.dev = dev;
    impl_dst->orig_dev = impl_src->orig_dev;
    impl_dst->is_attachment = 1;

    *dst = &impl_dst->buf;

    return rk_ok;
failed:
    if (impl_dst) {
        sg_free_table(impl_dst->sgt);
        osal_kfree(impl_dst);
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_dmabuf_attach);

rk_s32 osal_dmabuf_detach(osal_dmabuf *dmabuf)
{
    if (dmabuf) {
        osal_dmabuf_impl *buf = osal_dmabuf_to_impl(dmabuf);
        osal_dev_impl *dev = dev_to_impl(dmabuf->dev);

        if (buf->is_attachment) {
            dma_unmap_sgtable(dev->device, buf->sgt, DMA_BIDIRECTIONAL, 0);
            sg_free_table(buf->sgt);
            osal_kfree(buf);
            return rk_ok;
        } else {
            kmpp_loge_f("can not detach buffer %px which is not attachment\n", dmabuf);
        }
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_dmabuf_detach);