/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_dmaheap"

#include <linux/version.h>
#include <linux/mman.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dma-buf-cache.h>
#include <linux/scatterlist.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "rk_list.h"
#include "kmpp_dev.h"
#include "kmpp_log.h"
#include "kmpp_mem.h"
#include "kmpp_atomic.h"
#include "kmpp_dmaheap_impl.h"
#include "kmpp_string.h"
#include "kmpp_spinlock.h"
#include "kmpp_macro.h"

#define MAX_DMAHEAP_NUM                 8
#define CACHE_LINE_SIZE                 (64)
#define KMPP_INVALID_IOVA               (~(dma_addr_t)0)

#define DMABUF_DBG_FLOW                 (0x00000001)
#define DMABUF_DBG_HEAPS                (0x00000002)
#define DMABUF_DBG_BUF                  (0x00000004)
#define DMABUF_DBG_IOCTL                (0x00000008)
#define DMABUF_DBG_DUMP                 (0x00000010)

#define dmabuf_dbg(flag, fmt, ...)      kmpp_dbg(kmpp_dmabuf_debug, flag, fmt, ## __VA_ARGS__)
#define dmabuf_dbg_f(flag, fmt, ...)    kmpp_dbg_f(kmpp_dmabuf_debug, flag, fmt, ## __VA_ARGS__)

#define dmabuf_dbg_flow(fmt, ...)       dmabuf_dbg_f(DMABUF_DBG_FLOW, fmt, ## __VA_ARGS__)
#define dmabuf_dbg_heaps(fmt, ...)      dmabuf_dbg(DMABUF_DBG_HEAPS, fmt, ## __VA_ARGS__)
#define dmabuf_dbg_buf(fmt, ...)        dmabuf_dbg(DMABUF_DBG_BUF, fmt, ## __VA_ARGS__)
#define dmabuf_dbg_ioctl(fmt, ...)      dmabuf_dbg(DMABUF_DBG_IOCTL, fmt, ## __VA_ARGS__)

#define get_dmaheaps_srv(caller) \
    ({ \
        KmppDmaHeapsSrv *__tmp; \
        if (srv_dmaheaps) { \
            __tmp = srv_dmaheaps; \
        } else { \
            kmpp_err_f("kmpp dmaheaps srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

typedef void *(*HeapFindFunc)(const char *name);
typedef void (*HeapPutFunc)(void *);

typedef struct dma_buf *(*HeapAllocFunc)(void *heap, size_t len,
    unsigned int fd_flags, unsigned int heap_flags, const char *name);

typedef struct KmppDmaHeapInfo_t {
    const char      *name;
    const char      *heap_find;
    const char      *heap_put;
    const char      *heap_alloc;
    const char      *heap_names[MAX_DMAHEAP_NUM];
} KmppDmaHeapInfo;

typedef struct KmppDmaHeapImpl_t    KmppDmaHeapImpl;
typedef struct KmppDmaHeaps_t       KmppDmaHeaps;

struct KmppDmaHeapImpl_t {
    /* pointer to global heaps */
    KmppDmaHeaps        *heaps;
    /* handle for (struct dma_heap *) or (struct rk_dma_heap *) */
    void                *handle;
    /* list for KmppDmaBuf list */
    osal_list_head      list_used;
    rk_u32              heap_id;
    rk_u32              buf_id;
    rk_s32              buf_count;
    /* flag value for the heap */
    rk_s32              index;
    rk_s32              valid;
    rk_s32              ref_cnt;
};

struct KmppDmaHeaps_t {
    osal_list_head      list;
    KmppDmaHeapInfo     *info;
    osal_spinlock       *lock;
    HeapFindFunc        find;
    HeapPutFunc         put;
    HeapAllocFunc       alloc;
    KmppDmaHeapImpl     heaps[MAX_DMAHEAP_NUM];
};

typedef struct KmppDmaBufIova_t {
    /* list to list_iova in KmppDmaBufImpl */
    osal_list_head      list;
    /* buf - KmppDmaBufImpl pointer */
    void                *buf;
    osal_dev            *dev;
    struct device       *device;
    struct dma_buf      *dmabuf;
    struct dma_buf_attachment *attach;
    struct sg_table     *sgt;
    rk_u64              iova;
    rk_s32              size;
    rk_s32              sg_size;
} KmppDmaBufIova;

typedef struct KmppDmaBufImpl_t {
    /* list to list_used in KmppDmaHeapImpl */
    osal_list_head      list;
    /* list for list in KmppDmaBufIova */
    osal_list_head      list_iova;
    osal_spinlock       *lock_iova;
    /* pointer to global heaps */
    KmppDmaHeapImpl     *heap;

    struct dma_buf      *dma_buf;
    void                *priv;
    struct sg_table     *sgt;

    void                *kptr;
    rk_u64              uptr;
    rk_u32              heap_id;
    rk_u32              buf_id;
    rk_s32              fd;
    rk_s32              size;
    rk_u32              flag;
} KmppDmaBufImpl;

typedef struct KmppDmaHeapsSrv_t {
    /* default heaps */
    KmppDmaHeaps            *heaps;

    /* the reset heaps */
    osal_list_head          list_heap;

    rk_u32                  heap_id;
    rk_s32                  heap_cnt;

    /* external buffer for import */
    osal_list_head          list_ext_buf;
    rk_u32                  ext_buf_id;
    rk_s32                  ext_buf_cnt;

    /* dummy device for buf attachment of kernel double range address vmap */
    struct device           *dev;
    struct platform_device  *pdev;
} KmppDmaHeapsSrv;

static KmppDmaHeapInfo dmaheap_infos[] = {
    {    /* default system heap */
        .name       = "default dma heap",
        .heap_find  = "dma_heap_find",
        .heap_put   = "dma_heap_put",
        .heap_alloc = "dma_heap_buffer_alloc",
        .heap_names = {
            "system-uncached",          /*                     DEFAULT */
            "system-uncached-dma32",    /*                     DMA32   */
            "system",                   /*          CACHABLE           */
            "system-dma32",             /*          CACHABLE | DMA32   */
            "cma-uncached",             /* CONTIG                      */
            "cma-uncached-dma32",       /* CONTIG            | DMA32   */
            "cma",                      /* CONTIG | CACHABLE           */
            "cma-dma32",                /* CONTIG | CACHABLE | DMA32   */
        },
    },
    {    /* rk dma heap */
        .name       = "rk dma heap",
        .heap_find  = "rk_dma_heap_find",
        .heap_put   = "rk_dma_heap_put",
        .heap_alloc = "rk_dma_heap_buffer_alloc",
        .heap_names = {                 /* cma heap only */
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
            "rk-dma-heap-cma",
        },
    },
};

static KmppDmaHeapsSrv *srv_dmaheaps = NULL;
static rk_u32 kmpp_dmabuf_debug = 0;

void kmpp_dmabuf_dev_init(KmppDmaHeapsSrv *srv)
{
    struct platform_device *pdev;
    struct platform_device_info dev_info = {
        .name       = "kmpp_dmabuf",
        .id         = PLATFORM_DEVID_NONE,
        .dma_mask   = DMA_BIT_MASK(64),
    };

    pdev = platform_device_register_full(&dev_info);
    if (!IS_ERR(pdev)) {
        dma_set_max_seg_size(&pdev->dev, (unsigned int)DMA_BIT_MASK(64));
    } else {
        kmpp_loge_f("register kmpp_dmabuf platform device failed\n");
    }

    srv->pdev = pdev;
    srv->dev = &pdev->dev;
}

void kmpp_dmabuf_dev_deinit(KmppDmaHeapsSrv *srv)
{
    if (srv) {
        platform_device_unregister(srv->pdev);
        srv->pdev = NULL;
        srv->dev = NULL;
    }
}

void kmpp_dmaheap_init(void)
{
    KmppDmaHeapsSrv *srv = srv_dmaheaps;
    rk_s32 lock_size = osal_spinlock_size();
    rk_s32 i;

    if (srv) {
        kmpp_dmaheap_deinit();
        srv = NULL;
    }

    srv = kmpp_calloc_atomic(sizeof(KmppDmaHeapsSrv) + lock_size);
    if (!srv) {
        kmpp_loge_f("create dmaheaps srv failed\n");
        return;
    }

    srv_dmaheaps = srv;

    OSAL_INIT_LIST_HEAD(&srv->list_heap);
    OSAL_INIT_LIST_HEAD(&srv->list_ext_buf);

    for (i = 0; i < sizeof(dmaheap_infos) / sizeof(dmaheap_infos[0]); i++) {
        KmppDmaHeapInfo *info = &dmaheap_infos[i];
        HeapFindFunc find = (HeapFindFunc)__symbol_get(info->heap_find);
        HeapPutFunc put = (HeapPutFunc)__symbol_get(info->heap_put);
        HeapAllocFunc alloc = (HeapAllocFunc)__symbol_get(info->heap_alloc);

        /* NOTE: put may not exist */
        if (find && alloc) {
            KmppDmaHeaps *heaps = NULL;
            void *prev_valid = NULL;
            rk_s32 j;
            rk_u8 find_valid = 0;

            heaps = (KmppDmaHeaps *)kmpp_calloc_atomic(sizeof(KmppDmaHeaps) + lock_size);
            if (!heaps) {
                kmpp_loge_f("create heap impl failed\n");
                return;
            }

            osal_spinlock_assign(&heaps->lock, (void *)(heaps + 1), lock_size);

            heaps->info = info;
            heaps->find = find;
            heaps->put = put;
            heaps->alloc = alloc;

            kmpp_logi("probing %s\n", info->name);

            for (j = 0; j < MAX_DMAHEAP_NUM; j++) {
                KmppDmaHeapImpl *impl = &heaps->heaps[j];
                const char *name = info->heap_names[j];
                void *curr = find(name);

                if (curr && prev_valid != curr) {
                    kmpp_logi("find heap %s\n", name);
                    prev_valid = curr;
                    impl->valid = 1;
                    find_valid = 1;
                } else {
                    /* use previous valid heap */
                    curr = prev_valid;
                    impl->valid = 0;
                }

                impl->heaps = heaps;
                impl->handle = curr;
                impl->index = j;
                OSAL_INIT_LIST_HEAD(&impl->list_used);
                impl->heap_id = srv->heap_id++;
                srv->heap_cnt++;
            }

            if (!srv->heaps && find_valid) {
                srv->heaps = heaps;
                kmpp_logi("set %s as default\n", info->name);
            } else {
                kmpp_logi("add %s\n", info->name);
            }
            osal_list_add_tail(&heaps->list, &srv->list_heap);
        } else {
            kmpp_logi("probe heap %s failed\n", info->name);
        }
    }

    kmpp_dmabuf_dev_init(srv);
}

static void __kmpp_dmaheap_deinit(KmppDmaHeapsSrv *srv, KmppDmaHeaps *heaps)
{
    KmppDmaHeapInfo *info = heaps->info;
    rk_s32 i;

    osal_list_del_init(&heaps->list);

    for (i = MAX_DMAHEAP_NUM - 1; i >= 0; i--) {
        KmppDmaHeapImpl *impl = &heaps->heaps[i];
        KmppDmaBufImpl *buf, *n;
        rk_s32 once = 1;

        osal_list_for_each_entry_safe(buf, n, &impl->list_used, KmppDmaBufImpl, list) {
            if (once) {
                kmpp_logi("releasing leaked used dmabuf:\n", buf);
                once = 0;
            }
            osal_list_del_init(&buf->list);

            kmpp_logi("buf %d:%d - %#px\n", buf->heap_id, buf->buf_id, buf);
            kmpp_dmabuf_free_f(buf);
        }

        if (impl->valid && heaps->put)
            heaps->put(impl->handle);

        impl->handle = NULL;
        impl->valid = 0;
        srv->heap_cnt--;
    }

    if (info) {
        if (heaps->find)
            __symbol_put(info->heap_find);
        if (heaps->put)
            __symbol_put(info->heap_put);
        if (heaps->alloc)
            __symbol_put(info->heap_alloc);
    }

    osal_spinlock_deinit(&heaps->lock);
    kmpp_free(heaps);
}

void kmpp_dmaheap_deinit(void)
{
    KmppDmaHeapsSrv *srv = srv_dmaheaps;
    KmppDmaHeaps *heaps, *n;

    if (!srv)
        return;

    if (srv->heaps) {
        __kmpp_dmaheap_deinit(srv, srv->heaps);
        srv->heaps = NULL;
    }

    osal_list_for_each_entry_safe(heaps, n, &srv->list_heap, KmppDmaHeaps, list) {
        __kmpp_dmaheap_deinit(srv, heaps);
    }

    if (srv->heap_cnt)
        kmpp_loge_f("found %d heaps not deinited\n", srv->heap_cnt);

    kmpp_dmabuf_dev_deinit(srv);
    kmpp_free(srv);

    srv_dmaheaps = NULL;
}

rk_s32 kmpp_dmaheap_get(KmppDmaHeap *heap, rk_u32 flag, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    KmppDmaHeapImpl *impl;
    KmppDmaHeaps *heaps;
    rk_s32 old;

    if (!srv)
        return rk_nok;

    if (!heap) {
        kmpp_loge_f("invalid NULL heap\n");
        return rk_nok;
    }

    if (flag >= MAX_DMAHEAP_NUM) {
        kmpp_loge_f("suppoort dmaheap flag %x\n", flag);
        *heap = NULL;
        return rk_nok;
    }

    heaps = srv->heaps;
    if (!heaps) {
        kmpp_loge_f("found NULL default dmaheaps\n");
        return rk_nok;
    }

    impl = &heaps->heaps[flag];
    if (!impl->handle) {
        kmpp_loge_f("heap %s flag %#x invalid NULL heap handle at %s\n",
                    impl->heaps->info->name, flag, caller);
        *heap = NULL;
        return rk_nok;
    }

    old = KMPP_FETCH_ADD(&impl->ref_cnt, 1);
    *heap = impl;

    dmabuf_dbg_heaps("heap %s - %d ref %d -> %d\n", heaps->info->name,
                     impl->index, old, impl->ref_cnt);

    return rk_ok;
}

rk_s32 kmpp_dmaheap_get_by_name(KmppDmaHeap *heap, const rk_u8 *name, rk_u32 flag, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    KmppDmaHeaps *heaps, *n;

    if (!heap) {
        kmpp_loge_f("invalid NULL heap\n");
        return rk_nok;
    }

    *heap = NULL;

    if (flag >= MAX_DMAHEAP_NUM) {
        kmpp_loge_f("suppoort dmaheap flag %x\n", flag);
        return rk_nok;
    }

    osal_list_for_each_entry_safe(heaps, n, &srv->list_heap, KmppDmaHeaps, list) {
        dmabuf_dbg_heaps("checking %s for %s\n", heaps->info->name, name);
        if (!osal_strcmp(heaps->info->name, name)) {
            KmppDmaHeapImpl *impl = &heaps->heaps[flag];
            rk_s32 old;

            old = KMPP_FETCH_ADD(&impl->ref_cnt, 1);
            *heap = impl;

            dmabuf_dbg_heaps("heap %s - %d ref %d -> %d\n", impl->heaps->info->name,
                              flag, old, impl->ref_cnt);

            return rk_ok;
        }
    }

    return rk_nok;
}

rk_s32 kmpp_dmaheap_put(KmppDmaHeap heap, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    KmppDmaHeapImpl *impl;
    KmppDmaHeaps *heaps;
    rk_s32 old;

    if (!srv)
        return rk_nok;

    if (!heap) {
        kmpp_loge_f("invalid NULL heap\n");
        return rk_nok;
    }

    heaps = srv->heaps;
    if (!heaps) {
        kmpp_loge_f("found NULL default dmaheaps\n");
        return rk_nok;
    }

    impl = (KmppDmaHeapImpl *)heap;
    old = KMPP_FETCH_SUB(&impl->ref_cnt, 1);

    dmabuf_dbg_heaps("heap %s - %d ref %d -> %d\n", impl->heaps->info->name,
                      impl->index, old, impl->ref_cnt);

    if (impl->ref_cnt < 0)
        kmpp_loge_f("invalid ref_cnt %d at %s\n", impl->ref_cnt, caller);

    return rk_ok;
}

static void *dmabuf_double_vmap(struct dma_buf *dma_buf, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    struct dma_buf_attachment *attach;
    void *vaddr = NULL;

    attach = dma_buf_attach(dma_buf, srv->dev);
    if (IS_ERR_OR_NULL(attach)) {
        kmpp_loge_f("failed to attach dma_buf %px to %px at %s\n",
                    dma_buf, srv->dev, caller);
    } else {
        struct sg_table *sgt;

        sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
        if (IS_ERR(sgt)) {
            kmpp_loge_f("dma_buf_map_attachment failed ret %d at %s\n",
                        PTR_ERR(sgt), caller);
        } else {
            struct sg_table *table = sgt;
            rk_s32 npages = PAGE_ALIGN(dma_buf->size) / PAGE_SIZE;
            struct page **pages = kmpp_calloc(sizeof(struct page *) * npages * 2);
            struct page **tmp = pages;
            struct sg_page_iter piter;

            for_each_sgtable_page(table, &piter, 0) {
                // WARN_ON(tmp - pages >= npages);
                *tmp++ = sg_page_iter_page(&piter);
            }
            table = sgt;
            for_each_sgtable_page(table, &piter, 0) {
                // WARN_ON(tmp - pages >= npages);
                *tmp++ = sg_page_iter_page(&piter);
            }

            vaddr = vmap(pages, 2 * npages, VM_MAP, PAGE_KERNEL);

            dma_buf_unmap_attachment(attach, sgt, DMA_BIDIRECTIONAL);
            kmpp_free(pages);
        }

        dma_buf_detach(dma_buf, attach);
    }

    return vaddr;
}


static KmppDmaBufIova *attach_iova(KmppDmaBuf buf, osal_dev *dev, struct device *device, const rk_u8 *caller)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    KmppDmaBufIova *node = NULL;
    struct dma_buf_attachment *attach;
    struct dma_buf *dmabuf;
    struct sg_table *sgt;
    struct scatterlist *sg = NULL;
    rk_u32 i;

    node = kmpp_calloc(sizeof(*node));
    if (!node) {
        kmpp_loge_f("create buffer iova node failed at \n", caller);
        return NULL;
    }

    dmabuf = impl->dma_buf;

    attach = dma_buf_attach(dmabuf, device);
    if (IS_ERR_OR_NULL(attach)) {
        kmpp_loge_f("dma_buf_attach dmabuf %px:%px failed ret %d at %s\n",
                    buf, dmabuf, PTR_ERR(attach), caller);
        goto failed;
    }

    sgt = dma_buf_map_attachment(attach, DMA_BIDIRECTIONAL);
    if (IS_ERR_OR_NULL(sgt)) {
        kmpp_loge_f("dma_buf_map_attachment dmabuf %px:%x failed ret %d at %s\n",
                    buf, dmabuf, PTR_ERR(sgt), caller);
        goto failed;
    }

    node->buf = buf;
    node->dev = dev;
    node->device = device;
    node->iova = sg_dma_address(sgt->sgl);
    node->size = impl->size;
    node->dmabuf = dmabuf;
    node->attach = attach;
    node->sgt = sgt;

    for_each_sgtable_sg(sgt, sg, i) {
        node->sg_size += sg_dma_len(sg);
    }

    OSAL_INIT_LIST_HEAD(&node->list);
    osal_spin_lock(impl->lock_iova);
    osal_list_add_tail(&node->list, &impl->list_iova);
    osal_spin_unlock(impl->lock_iova);

    return node;

failed:
    if (dmabuf && attach) {
        dma_buf_detach(dmabuf, attach);
        attach = NULL;
    }

    kmpp_free(node);
    return NULL;
}

static void detach_iova(KmppDmaBufIova *node)
{
    dma_buf_unmap_attachment(node->attach, node->sgt, DMA_BIDIRECTIONAL);
    dma_buf_detach(node->dmabuf, node->attach);
    kmpp_free(node);
}

#define attach_iova_f(buf, dev, device) attach_iova(buf, dev, device, __func__)

rk_s32 kmpp_dmabuf_alloc(KmppDmaBuf *buf, KmppDmaHeap heap, rk_s32 size, rk_u32 flag, const rk_u8 *caller)
{
    KmppDmaHeapImpl *impl_heap = (KmppDmaHeapImpl *)heap;
    KmppDmaBufImpl *impl;
    KmppDmaHeaps *heaps;
    HeapAllocFunc func;
    struct dma_buf *dma_buf;
    rk_s32 lock_size;

    if (!buf || !heap || size <= 0 || flag >= MAX_DMAHEAP_NUM) {
        kmpp_loge_f("invalid buf %px heap %px size %d flag %x at %s\n",
                    buf, heap, size, flag, caller);
        return rk_nok;
    }

    if (!impl_heap->heaps || !impl_heap->heaps->alloc) {
        kmpp_loge_f("invalid heaps %px at %s\n", impl_heap->heaps, caller);
        return rk_nok;
    }

    *buf = NULL;
    lock_size = osal_spinlock_size();
    impl = kmpp_calloc(sizeof(KmppDmaBufImpl) + lock_size);
    if (!impl) {
        kmpp_loge_f("failed to create KmppDmaBufImpl\n");
        return rk_nok;
    }

    heaps = impl_heap->heaps;
    func = impl_heap->heaps->alloc;

    dma_buf = (struct dma_buf *)func(impl_heap->handle, size, O_CLOEXEC | O_RDWR, 0, "kmpp");
    if (IS_ERR_OR_NULL(dma_buf)) {
        kmpp_loge_f("failed to alloc dma_buf %d bytes ret %d\n", size, PTR_ERR(dma_buf));
        kmpp_free(impl);
        return rk_nok;
    }

    OSAL_INIT_LIST_HEAD(&impl->list);
    OSAL_INIT_LIST_HEAD(&impl->list_iova);
    osal_spinlock_assign(&impl->lock_iova, impl + 1, lock_size);

    impl->heap = impl_heap;
    impl->dma_buf = dma_buf;
    impl->size = size;
    impl->flag = flag;
    impl->fd = -1;

    osal_spin_lock(heaps->lock);
    osal_list_add_tail(&impl->list, &impl_heap->list_used);
    impl->heap_id = impl_heap->heap_id;
    impl->buf_id = impl_heap->buf_id++;
    impl_heap->buf_count++;
    osal_spin_unlock(heaps->lock);

    dmabuf_dbg_buf("dmabuf %d:%d size %d flag %x alloc\n",
                   impl->heap_id, impl->buf_id, size, flag);

    *buf = impl;

    return rk_ok;
}

rk_s32 kmpp_dmabuf_free(KmppDmaBuf buf, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    if (!srv)
        return rk_nok;

    if (!impl) {
        kmpp_loge_f("invalid NULL buf\n");
        return rk_nok;
    }

    if (impl->heap) {
        KmppDmaHeapImpl *heap = impl->heap;
        KmppDmaHeaps *heaps = heap->heaps;

        osal_spin_lock(heaps->lock);
        osal_list_del_init(&impl->list);
        heap->buf_count--;
        osal_spin_unlock(heaps->lock);
    } else {
        KmppDmaHeaps *heaps = srv->heaps;

        osal_spin_lock(heaps->lock);
        osal_list_del_init(&impl->list);
        srv->ext_buf_cnt--;
        osal_spin_unlock(heaps->lock);
    }

    {   /* remove all iova attachment */
        KmppDmaBufIova *iova, *n;
        OSAL_LIST_HEAD(list_tmp);

        osal_spin_lock(impl->lock_iova);
        osal_list_for_each_entry_safe(iova, n, &impl->list_iova, KmppDmaBufIova, list) {
            osal_list_move_tail(&iova->list, &list_tmp);
        }
        osal_spin_unlock(impl->lock_iova);

        osal_list_for_each_entry_safe(iova, n, &list_tmp, KmppDmaBufIova, list) {
            osal_list_del_init(&iova->list);
            detach_iova(iova);
        }
    }

    if (impl->kptr) {
        if (impl->flag & KMPP_DMABUF_FLAGS_DUP_MAP) {
            vunmap(impl->kptr);
        } else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 01, 0)
            dma_buf_vunmap(impl->dma_buf, impl->kptr);
#else
            struct iosys_map map = IOSYS_MAP_INIT_VADDR(impl->kptr);

            dma_buf_vunmap(impl->dma_buf, &map);
#endif
        }
        impl->kptr = NULL;
    }

    /*
     * If the current process killed, the current mm will be recycle first,
     * so it does not need to vm_mnunmap uaddr here
     */
    if (impl->uptr && current->mm) {
        rk_u32 size = impl->size;

        if (impl->flag & KMPP_DMABUF_FLAGS_DUP_MAP)
            size *= 2;

        vm_munmap((rk_ul)impl->uptr, size);
    }

    if (impl->dma_buf) {
        if (impl->fd >= 0) {
            dma_buf_put(impl->dma_buf);
            impl->fd = -1;
        }

        dma_buf_put(impl->dma_buf);
        impl->dma_buf = NULL;
    }

    dmabuf_dbg_buf("dmabuf %d:%d size %d flag %x free\n",
                   impl->heap_id, impl->buf_id, impl->size, impl->flag);

    osal_spinlock_deinit(&impl->lock_iova);
    kmpp_free(impl);

    return rk_ok;
}

rk_s32 kmpp_dmabuf_import_fd(KmppDmaBuf *buf, rk_s32 fd, rk_u32 flag, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    struct dma_buf *dma_buf;
    KmppDmaBufImpl *impl;
    rk_s32 lock_size;

    if (!srv)
        return rk_nok;

    if (!buf || fd < 0) {
        kmpp_loge_f("invalid buf %px fd %d at %s\n", buf, fd, caller);
        return rk_nok;
    }

    *buf = NULL;
    dma_buf = dma_buf_get(fd);
    if (IS_ERR_OR_NULL(dma_buf)) {
        kmpp_loge_f("failed to dma_buf_get fd %d ret %d at %s\n",
                    fd, PTR_ERR(dma_buf), caller);
        return rk_nok;
    }

    lock_size = osal_spinlock_size();
    impl = kmpp_calloc(sizeof(KmppDmaBufImpl) + lock_size);
    if (!impl) {
        kmpp_loge_f("failed to create KmppDmaBufImpl\n");
        dma_buf_put(dma_buf);
        return rk_nok;
    }

    OSAL_INIT_LIST_HEAD(&impl->list);
    OSAL_INIT_LIST_HEAD(&impl->list_iova);
    osal_spinlock_assign(&impl->lock_iova, impl + 1, lock_size);
    impl->dma_buf = dma_buf;
    impl->fd = fd;
    impl->size = dma_buf->size;
    impl->flag = flag;
    get_dma_buf(dma_buf);

    if (srv->heaps) {
        KmppDmaHeaps *heaps = srv->heaps;

        osal_spin_lock(heaps->lock);
        osal_list_add_tail(&impl->list, &srv->list_ext_buf);
        srv->ext_buf_cnt++;
        impl->heap_id = (rk_u32)-1;
        impl->buf_id = srv->ext_buf_id++;
        osal_spin_unlock(heaps->lock);
    }

    dmabuf_dbg_buf("dmabuf %d:%d size %d flag %x import by fd %d\n",
                   impl->heap_id, impl->buf_id, impl->size, impl->flag, impl->fd);

    *buf = impl;

    return rk_ok;
}

rk_s32 kmpp_dmabuf_import_ctx(KmppDmaBuf *buf, void *ctx, rk_u32 flag, const rk_u8 *caller)
{
    KmppDmaHeapsSrv *srv = get_dmaheaps_srv(caller);
    struct dma_buf *dma_buf = (struct dma_buf *)ctx;
    KmppDmaBufImpl *impl;
    rk_s32 lock_size;

    if (!srv)
        return rk_nok;

    if (!buf || !ctx) {
        kmpp_loge_f("invalid buf %px ctx %px at %s\n", buf, ctx, caller);
        return rk_nok;
    }

    *buf = NULL;
    lock_size = osal_spinlock_size();
    impl = kmpp_calloc(sizeof(KmppDmaBufImpl) + lock_size);
    if (!impl) {
        kmpp_loge_f("failed to create KmppDmaBufImpl\n");
        return rk_nok;
    }

    OSAL_INIT_LIST_HEAD(&impl->list);
    OSAL_INIT_LIST_HEAD(&impl->list_iova);
    osal_spinlock_assign(&impl->lock_iova, impl + 1, lock_size);
    impl->dma_buf = dma_buf;
    impl->fd = -1;
    impl->size = dma_buf->size;
    impl->flag = flag;
    get_dma_buf(dma_buf);

    if (srv->heaps) {
        KmppDmaHeaps *heaps = srv->heaps;

        osal_spin_lock(heaps->lock);
        osal_list_add_tail(&impl->list, &srv->list_ext_buf);
        srv->ext_buf_cnt++;
        impl->heap_id = (rk_u32)-1;
        impl->buf_id = srv->ext_buf_id++;
        osal_spin_unlock(heaps->lock);
    }

    dmabuf_dbg_buf("dmabuf %d:%d size %d flag %x import by ctx %#px\n",
                   impl->heap_id, impl->buf_id, impl->size, impl->flag, dma_buf);

    *buf = impl;

    return rk_ok;
}

void *kmpp_dmabuf_get_kptr(KmppDmaBuf buf)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    struct dma_buf *dma_buf;
    void *ptr;

    if (!impl)
        return NULL;

    if (impl->kptr)
        return impl->kptr;

    dma_buf = impl->dma_buf;
    if (impl->flag & KMPP_DMABUF_FLAGS_DUP_MAP) {
        ptr = dmabuf_double_vmap(dma_buf, __FUNCTION__);
        dmabuf_dbg_buf("dmabuf %d get kptr %#px with double range\n", impl->buf_id, ptr);
    } else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 01, 0)
        ptr = dma_buf_vmap(dma_buf);
#else
        {
            struct iosys_map map;
            dma_buf_vmap(dma_buf, &map);
            ptr = map.vaddr;
        }
#endif
        dmabuf_dbg_buf("dmabuf %d get kptr %#px\n", impl->buf_id, ptr);
    }

    if (IS_ERR_OR_NULL(ptr)) {
        kmpp_loge_f("buf %px vmap failed ret %d\n", PTR_ERR(ptr));
        ptr = NULL;
    }

    impl->kptr = ptr;

    return ptr;
}

rk_u64 kmpp_dmabuf_get_uptr(KmppDmaBuf buf)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    struct dma_buf *dma_buf;
    KmppDmaHeaps *heaps;
    struct file *file;
    rk_s32 size;
    rk_ul uptr;

    if (!impl || !current->mm)
        return 0;

    if (impl->uptr)
        return impl->uptr;

    heaps = impl->heap->heaps;
    dma_buf = impl->dma_buf;
    file = dma_buf->file;
    size = impl->size;
    uptr = 0;

    dmabuf_dbg_heaps("dmabuf %px file %px size %d at mm %px\n",
                      dma_buf, file, size, current->mm);

    osal_spin_lock(heaps->lock);
    if (impl->flag & KMPP_DMABUF_FLAGS_DUP_MAP) {
        rk_ul uptr0;
        rk_ul uptr1;

        do {
            uptr0 = get_unmapped_area(file, 0, size * 2, 0, MAP_SHARED);
            if (IS_ERR_VALUE(uptr0)) {
                kmpp_loge_f("get_unmapped_area failed ret %d\n", uptr0);
                break;
            }

            uptr1 = vm_mmap(file, uptr0 + size, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0);
            if (IS_ERR_VALUE(uptr)) {
                kmpp_loge_f("vm_mmap size %d failed ret %d\n", size, uptr);
                break;
            }

            if (uptr1 != (uptr0 + size)) {
                kmpp_loge_f("vm_mmap double size %d -> %d mismatch uptr seg0 %lx seg1 %lx\n",
                            size, size * 2, uptr1, uptr0 + size);
                vm_munmap(uptr1, size);
                uptr = 0;
                continue;
            }

            uptr = vm_mmap(file, uptr0, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0);
            if (IS_ERR_VALUE(uptr)) {
                kmpp_loge_f("vm_mmap double size %d -> %d failed ret %d\n",
                            size, size * 2, uptr1);
                vm_munmap(uptr0, size);
                uptr = 0;
                break;
            } else if (uptr1 != (uptr + size)) {
                kmpp_loge_f("vm_mmap double size %d -> %d mismatch uptr base %lx seg0 %lx seg1 %lx\n",
                            size, size * 2, uptr0, uptr, uptr1);
                vm_munmap(uptr, size);
                vm_munmap(uptr1, size);
                uptr = 0;
                continue;
            }

            break;
        } while (1);
    } else {
        uptr = vm_mmap(file, 0, size, PROT_READ | PROT_WRITE, MAP_SHARED, 0);
        if (IS_ERR_VALUE(uptr)) {
            kmpp_loge_f("vm_mmap size %d failed ret %d\n", size, uptr);
            uptr = 0;
        }
    }
    osal_spin_unlock(heaps->lock);

    dmabuf_dbg_buf("dmabuf %d get uptr %#lx\n", impl->buf_id, uptr);

    impl->uptr = (rk_u64)uptr;

    return impl->uptr;
}

rk_s32 kmpp_dmabuf_get_size(KmppDmaBuf buf)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    return impl ? impl->size : 0;
}

rk_u32 kmpp_dmabuf_get_flag(KmppDmaBuf buf)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    return impl ? impl->flag : 0;
}

rk_s32 kmpp_dmabuf_get_iova(KmppDmaBuf buf, rk_u64 *iova, osal_dev *dev)
{
    KmppDmaBufIova *node;

    if (!buf || !iova || !dev) {
        kmpp_loge_f("invalid buf %px iova %px dev %px\n", buf, iova, dev);
        return rk_nok;
    }

    node = attach_iova_f(buf, dev, osal_dev_get_device(dev));
    if (!node)
        return rk_nok;

    dmabuf_dbg_buf("dmabuf %d get %s iova %#llx\n", ((KmppDmaBufImpl *)buf)->buf_id,
                   dev->name, node->iova);

    *iova = node->iova;
    return rk_ok;
}

rk_s32 kmpp_dmabuf_put_iova(KmppDmaBuf buf, rk_u64 iova, osal_dev *dev)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    KmppDmaBufIova *node;
    KmppDmaBufIova *pos, *n;

    if (!impl || iova == KMPP_INVALID_IOVA || !dev) {
        kmpp_loge_f("invalid buf %px iova %llx dev %px\n", buf, iova, dev);
        return rk_nok;
    }

    node = NULL;
    osal_spin_lock(impl->lock_iova);
    osal_list_for_each_entry_safe(pos, n, &impl->list_iova, KmppDmaBufIova, list) {
        if (pos->iova == iova && pos->dev == dev) {
            osal_list_del_init(&pos->list);
            node = pos;
            break;
        }
    }
    osal_spin_unlock(impl->lock_iova);

    if (!node) {
        kmpp_loge_f("not found buffer iova node %px iova %llx dev %px\n",
                    buf, iova, dev);
        return rk_nok;
    }

    dmabuf_dbg_buf("dmabuf %d put %s iova %#llx\n", ((KmppDmaBufImpl *)buf)->buf_id,
                   dev->name, node->iova);

    detach_iova(node);
    return rk_ok;
}

rk_s32 kmpp_dmabuf_get_iova_by_device(KmppDmaBuf buf, rk_u64 *iova, void *device)
{
    KmppDmaBufIova *node;

    if (!buf || !iova || !device) {
        kmpp_loge_f("invalid buf %px iova %px dev %px\n", buf, iova, device);
        return rk_nok;
    }

    node = attach_iova_f(buf, NULL, device);
    if (!node)
        return rk_nok;

    dmabuf_dbg_buf("dmabuf %d get %s iova %#llx\n", ((KmppDmaBufImpl *)buf)->buf_id,
                   osal_device_name(device), node->iova);

    *iova = node->iova;
    return rk_ok;
}

rk_s32 kmpp_dmabuf_put_iova_by_device(KmppDmaBuf buf, rk_u64 iova, void *device)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    KmppDmaBufIova *node;
    KmppDmaBufIova *pos, *n;

    if (!impl || iova == KMPP_INVALID_IOVA || !device) {
        kmpp_loge_f("invalid buf %px iova %llx device %px\n", buf, iova, device);
        return rk_nok;
    }

    node = NULL;
    osal_spin_lock(impl->lock_iova);
    osal_list_for_each_entry_safe(pos, n, &impl->list_iova, KmppDmaBufIova, list) {
        if (pos->iova == iova && pos->device == device) {
            osal_list_del_init(&pos->list);
            node = pos;
            break;
        }
    }
    osal_spin_unlock(impl->lock_iova);

    if (!node) {
        kmpp_loge_f("not found buffer iova node %px iova %llx device %px\n",
                    buf, iova, device);
        return rk_nok;
    }

    dmabuf_dbg_buf("dmabuf %d put %s iova %#llx\n", ((KmppDmaBufImpl *)buf)->buf_id,
                   osal_device_name(device), node->iova);

    detach_iova(node);
    return rk_ok;
}

rk_u64 kmpp_invalid_iova(void)
{
    return (rk_u64)(~(dma_addr_t)0);
}

void kmpp_dmabuf_set_priv(KmppDmaBuf buf, void *priv)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    if (impl)
        impl->priv = priv;
}

void *kmpp_dmabuf_get_priv(KmppDmaBuf buf)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    return impl ? impl->priv : NULL;
}

rk_s32 kmpp_dmabuf_flush_for_cpu(KmppDmaBuf buf)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    if (!impl) {
        kmpp_loge_f("invalid NULL buf\n");
        return rk_nok;
    }

    dma_buf_begin_cpu_access(impl->dma_buf, DMA_FROM_DEVICE);

    return 0;
}

rk_s32 kmpp_dmabuf_flush_for_dev(KmppDmaBuf buf, osal_dev *dev)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;

    if (!impl) {
        kmpp_loge_f("invalid NULL buf\n");
        return rk_nok;
    }

    dma_buf_end_cpu_access(impl->dma_buf, DMA_TO_DEVICE);

    return 0;
}

#ifndef CONFIG_DMABUF_PARTIAL
#define dma_buf_begin_cpu_access_partial(dma_buf, dir, offset, size) \
    dma_buf_begin_cpu_access(dma_buf, dir)

#define dma_buf_end_cpu_access_partial(dma_buf, dir, offset, size) \
    dma_buf_end_cpu_access(dma_buf, dir)
#endif

rk_s32 kmpp_dmabuf_flush_for_cpu_partial(KmppDmaBuf buf, rk_u32 offset, rk_u32 size)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    rk_u32 _offset, _size;

    if (!impl) {
        kmpp_loge_f("invalid NULL buf\n");
        return rk_nok;
    }

    _offset = KMPP_ALIGN_DOWN(offset, CACHE_LINE_SIZE);
    _size = KMPP_ALIGN(size + offset - _offset, CACHE_LINE_SIZE);

    dma_buf_begin_cpu_access_partial(impl->dma_buf, DMA_FROM_DEVICE, _offset, _size);

    return 0;
}

rk_s32 kmpp_dmabuf_flush_for_dev_partial(KmppDmaBuf buf, osal_dev *dev, rk_u32 offset, rk_u32 size)
{
    KmppDmaBufImpl *impl = (KmppDmaBufImpl *)buf;
    rk_u32 _offset, _size;

    if (!impl) {
        kmpp_loge_f("invalid NULL buf\n");
        return rk_nok;
    }

    _offset = KMPP_ALIGN_DOWN(offset, CACHE_LINE_SIZE);
    _size = KMPP_ALIGN(size + offset - _offset, CACHE_LINE_SIZE);

    dma_buf_end_cpu_access_partial(impl->dma_buf, DMA_TO_DEVICE, _offset, _size);

    return 0;
}

EXPORT_SYMBOL(kmpp_dmaheap_get);
EXPORT_SYMBOL(kmpp_dmaheap_get_by_name);
EXPORT_SYMBOL(kmpp_dmaheap_put);

EXPORT_SYMBOL(kmpp_dmabuf_alloc);
EXPORT_SYMBOL(kmpp_dmabuf_free);
EXPORT_SYMBOL(kmpp_dmabuf_import_fd);
EXPORT_SYMBOL(kmpp_dmabuf_import_ctx);

EXPORT_SYMBOL(kmpp_dmabuf_get_kptr);
EXPORT_SYMBOL(kmpp_dmabuf_get_uptr);
EXPORT_SYMBOL(kmpp_dmabuf_get_size);
EXPORT_SYMBOL(kmpp_dmabuf_get_flag);
EXPORT_SYMBOL(kmpp_dmabuf_get_iova);
EXPORT_SYMBOL(kmpp_dmabuf_put_iova);
EXPORT_SYMBOL(kmpp_dmabuf_put_iova_by_device);
EXPORT_SYMBOL(kmpp_dmabuf_get_iova_by_device);
EXPORT_SYMBOL(kmpp_invalid_iova);

EXPORT_SYMBOL(kmpp_dmabuf_set_priv);
EXPORT_SYMBOL(kmpp_dmabuf_get_priv);

EXPORT_SYMBOL(kmpp_dmabuf_flush_for_cpu);
EXPORT_SYMBOL(kmpp_dmabuf_flush_for_dev);
EXPORT_SYMBOL(kmpp_dmabuf_flush_for_cpu_partial);
EXPORT_SYMBOL(kmpp_dmabuf_flush_for_dev_partial);
