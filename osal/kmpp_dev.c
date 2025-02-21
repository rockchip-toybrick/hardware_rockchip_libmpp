/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_dev"

#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/interrupt.h>
#include <linux/of_platform.h>
#include <soc/rockchip/rockchip_iommu.h>

#include "rk_list.h"
#include "kmpp_atomic.h"
#include "kmpp_mem.h"
#include "kmpp_log.h"
#include "kmpp_mutex.h"
#include "kmpp_dev_impl.h"
#include "kmpp_pm.h"
#include "kmpp_string.h"

typedef struct osal_dev_srv_t {
    osal_mutex *lock;

    /* list head for list_srv in osal_drv_impl */
    osal_list_head list_drv;
    /* list head for list_srv in osal_dev_impl */
    osal_list_head list_dev;

    rk_s32 dev_uid;
} osal_dev_srv;

static osal_dev_srv *srv = NULL;

#define devs_to_drv(x)  container_of(x, osal_drv_impl, devs)
#define dev_to_impl(x)  container_of(x, osal_dev_impl, dev)

int osal_dev_mmu_fault(struct iommu_domain *iommu, struct device *iommu_dev,
                       unsigned long iova, int status, void *arg)
{
    osal_dev_impl *impl = (osal_dev_impl *)arg;
    const char *name = impl->dev.name;
    rk_u32 *reg = impl->dev.res->base;
    rk_u32 irq_status = readl(reg + (0x28 / sizeof(rk_u32)));

    if (iova != -1)
        kmpp_loge("%s mmu fault iova %lx status 0x%x\n", name, iova, status);

    kmpp_logi("irq mmu status %x\n", irq_status);

    return 0;

    /* check irq status */

#if 0
    if (impl->irq) {
        kmpp_loge("%s mmu fault handler start\n", name);
        return impl->irq(&impl->dev, iova, status);
    }
#endif

    /* disable mmu irq first then do the process */
    kmpp_loge("%s disable irq %d\n", name, impl->dev.irq_mmu);
    //disable_irq(impl->dev.irq_mmu);

    kmpp_loge("%s reset device start\n", name);
    //reset_control_bulk_assert(impl->rst_cnt, impl->resets);

    //reset_control_bulk_deassert(impl->rst_cnt, impl->resets);
    kmpp_loge("%s reset device done\n", name);

    osal_dev_iommu_refresh(&impl->dev);

    return 0;
}

static rk_s32 osal_dev_iommu_probe(osal_dev_impl *dev)
{
    struct device *device = dev->device;
    struct device_node *np = dev->np;
    struct platform_device *pdev_mmu = NULL;
    struct iommu_group *group = NULL;
    struct iommu_domain *domain = NULL;
    const char *name = dev->dev.name;
    rk_s32 ret = rk_nok;

    np = of_parse_phandle(np, "iommus", 0);
    if (!np || !of_device_is_available(np)) {
            kmpp_loge("failed to get %s mmu node\n", name);
            goto mmu_probe_done;
    }

    pdev_mmu = of_find_device_by_node(np);
    of_node_put(np);

    if (!pdev_mmu) {
        kmpp_loge("failed to get %s mmu device\n", name);
            goto mmu_probe_done;
    }

    name = pdev_mmu->name;
    kmpp_logi("get iommu %s\n", name);

    group = iommu_group_get(device);
    if (!group) {
        kmpp_loge("failed to get %s mmu group\n", name);
        goto mmu_probe_done;
    }

    kmpp_logi("%s get iommu group %px\n", name, group);

    dev->group = group;

    domain = iommu_get_domain_for_dev(device);
    if (!domain) {
        kmpp_loge("failed to get %s mmu domain\n", name);
        goto mmu_probe_done;
    }

    kmpp_logi("%s get iommu domain %px\n", name, domain);

    dev->domain = domain;

    iommu_attach_group(domain, group);

    // TODO: one hardware mmu may different device fault handler to switch
    iommu_set_fault_handler(domain, osal_dev_mmu_fault, dev);

    dev->pdev_mmu = pdev_mmu;
    dev->dev.irq_mmu = platform_get_irq(pdev_mmu, 0);
    kmpp_logi("%s get iommu irq %d\n", name, dev->dev.irq_mmu);

    ret = rk_ok;

mmu_probe_done:
    kmpp_logi("%s mmu %s probe done\n", name, pdev_mmu ? pdev_mmu->name : "");

    return ret;
}

static rk_s32 osal_dev_reset_probe(osal_dev_impl *dev)
{
    struct device *device = dev->device;
    struct device_node *np = dev->np;
    //struct property *prop;
    //const char *str;
    rk_s32 ret = rk_nok;
    //rk_s32 i;

    dev->rst_cnt = reset_control_get_count(device);
    dev->rstcs = NULL;//devm_kzalloc(device, sizeof(*(dev->rstcs)) * dev->rst_cnt, GFP_KERNEL);
    if (!dev->rstcs) {
        kmpp_loge("failed to alloc reset_control_bulk_data\n");
        return ret;
    }

#if 0
    i = 0;
    of_property_for_each_string(np, "reset-names", prop, str) {
        kmpp_logi("reset names: %s\n", str);
        dev->rstcs[i].id = str;
        dev->rstcs[i].rstc = devm_reset_control_get(device, str);
        i++;
    }
#endif

    of_node_put(np);

    return ret;
#if 0

    kmpp_logi("total reset count %d\n", dev->rst_cnt);

    {
        rk_s32 normal_cnt = 0;
        rk_s32 ret;


        /* get shared reset */

        for (i = 0; i < dev->rst_cnt; i++) {
            struct reset_control_bulk_data *reset = &dev->resets[i];

            kmpp_logi("normal reset %d - %-6s : 0x%px\n", i, reset->id, reset->rstc);
        }
        reset_control_bulk_deassert(dev->rst_cnt, dev->resets);

        rk_s32 shared_cnt = 0;

        /* get normal reset */
        for (i = impl->rst_cnt - normal_cnt ; i > 0; i--) {
            if (devm_reset_control_bulk_get_shared(device, i, impl->resets + normal_cnt))
                continue;

            shared_cnt = i;
            break;
        }
        for (i = shared_cnt; i < normal_cnt + shared_cnt; i++) {
            struct reset_control_bulk_data *reset = &impl->resets[i];

            kmpp_logi("shared reset %d - %-6s : 0x%px\n", i, reset->id, reset->rstc);
        }
    }
#endif
}

static int osal_dev_probe(struct platform_device *pdev)
{
    osal_drv_impl *drv = NULL;
    osal_dev_impl *impl = NULL;
    osal_dev *dev = NULL;
    osal_dev_res *res = NULL;
    struct device *device = NULL;
    const char *name = NULL;
    rk_s32 res_cnt;
    rk_s32 i;

    {
        osal_drv_impl *pos, *n;

        osal_mutex_lock(srv->lock);
        osal_list_for_each_entry_safe(pos, n, &srv->list_drv, osal_drv_impl, list_srv) {
            if (osal_strstr(pdev->name, pos->name)) {
                kmpp_logi("%s found device %s\n", pos->name, pdev->name);
                drv = pos;
                break;
            }
        }
        osal_mutex_unlock(srv->lock);

        if (!drv) {
            kmpp_loge("not match drv %s\n", pdev->name);
            return rk_nok;
        }
    }

    res_cnt = pdev->num_resources;

    impl = osal_kcalloc(sizeof(osal_dev_impl) + sizeof(osal_dev_res) * res_cnt, osal_gfp_normal);
    dev = &impl->dev;
    res = (osal_dev_res *)(impl + 1);

    impl->device = &pdev->dev;
    impl->pdev = pdev;
    impl->drv = drv;
    impl->np = dev_of_node(&pdev->dev);

    device = impl->device;
    name = dev_name(device);
    platform_set_drvdata(pdev, impl);
    OSAL_INIT_LIST_HEAD(&impl->list_srv);
    OSAL_INIT_LIST_HEAD(&impl->list_drv);

    dev->name = pdev->name;
    dev->dev_uid = KMPP_FETCH_ADD(&srv->dev_uid, 1);
    dev->core_id = 0;
    dev->res_cnt = res_cnt;
    dev->res = res;
    dev->drv = &drv->drv;

    {
        const struct of_device_id *match;

        match = of_match_node(drv->ids, impl->np);
        dev->drv_data = (match) ? (void *)match->data : NULL;
    }

    for (i = 0; i < res_cnt; i++) {
        res[i].name = pdev->resource[i].name;
        res[i].base = devm_ioremap_resource(device, &pdev->resource[i]);
        res[i].end = res[i].base + ((pdev->resource[i].end + 1 - pdev->resource[i].start) / sizeof(rk_u32));

        kmpp_logi("res %-10s : 0x%px - 0x%px | %d\n", res[i].name,
                  res[i].base, res[i].end, res[i].end - res[i].base);
    }
    dev->irq = platform_get_irq(pdev, 0);
    kmpp_logi("irq %d\n", dev->irq);

    osal_device_init_wakeup(dev, true);
    osal_pm_rt_enable(dev);

    /* probe device mmu */
    osal_dev_iommu_probe(impl);

    /* get all clock for device */
    osal_clk_probe(dev);

    /* get all reset for device */
    osal_dev_reset_probe(impl);

    drv->dops.probe(&impl->dev);

    osal_mutex_lock(srv->lock);
    osal_list_add_tail(&impl->list_srv, &srv->list_dev);
    osal_list_add_tail(&impl->list_drv, &drv->list_dev);
    drv->devs.dev[drv->devs.count] = &impl->dev;
    drv->devs.count++;
    if (drv->devs.count >= drv->dev_cnt)
        kmpp_loge("driver %s device count %d overflow\n", drv->name, drv->dev_cnt);
    osal_mutex_unlock(srv->lock);

    return 0;
}

static int osal_dev_remove(struct platform_device *pdev)
{
    osal_dev_impl *dev = dev_get_drvdata(&pdev->dev);

    if (dev) {
        osal_drv_impl *drv = dev->drv;
        rk_s32 i;

        if (drv && drv->dops.remove)
            drv->dops.remove(&dev->dev);

        if (dev->group) {
            iommu_detach_group(dev->domain, dev->group);
            iommu_group_put(dev->group);
            dev->group = NULL;
        }

        dev->domain = NULL;

        osal_clk_release(&dev->dev);

        osal_device_init_wakeup(&dev->dev, false);
        osal_pm_rt_disable(&dev->dev);

        if (dev->pdev_mmu) {
            platform_device_put(dev->pdev_mmu);
            dev->pdev_mmu = NULL;
        }

        osal_mutex_lock(srv->lock);
        osal_list_del_init(&dev->list_srv);
        osal_list_del_init(&dev->list_drv);
        for (i = 0; i < drv->dev_cnt; i++) {
            if (drv->devs.dev[i] == &dev->dev) {
                drv->devs.dev[i] = NULL;
                drv->devs.count--;
                break;
            }
        }
        osal_mutex_unlock(srv->lock);

        osal_kfree(dev);
    }

    return 0;
}

static void osal_dev_shutdown(struct platform_device *pdev)
{
    osal_dev_impl *dev = dev_get_drvdata(&pdev->dev);

    if (dev && dev->drv && dev->drv->dops.shutdown)
        dev->drv->dops.shutdown(&dev->dev);
}

static int osal_dev_suspend(struct platform_device *pdev, pm_message_t state)
{
    osal_dev_impl *dev = dev_get_drvdata(&pdev->dev);

    if (dev && dev->drv && dev->drv->dops.suspend)
        dev->drv->dops.suspend(&dev->dev);

    return 0;
}

static int osal_dev_resume(struct platform_device *pdev)
{
    osal_dev_impl *dev = dev_get_drvdata(&pdev->dev);

    if (dev && dev->drv && dev->drv->dops.resume)
        dev->drv->dops.resume(&dev->dev);

    return 0;
}

static irqreturn_t osal_dev_irq(int irq, void *param)
{
    osal_dev_impl *dev = (osal_dev_impl *)param;
    rk_s32 ret;

    kmpp_logd("%s irq enter\n", dev->dev.name);

    if (!dev || !dev->irq)
        return IRQ_NONE;

    ret = dev->irq(&dev->dev);
    if (ret) {
        kmpp_loge("dev %s irq failed ret %d\n", dev->dev.name, ret);
        return IRQ_NONE;
    }

    return IRQ_WAKE_THREAD;
}

static irqreturn_t osal_dev_isr(int irq, void *param)
{
    osal_dev_impl *dev = (osal_dev_impl *)param;
    rk_s32 ret;

    if (!dev)
        return IRQ_NONE;

    if (!dev->isr)
        return IRQ_HANDLED;

    ret = dev->isr(&dev->dev);
    if (ret)
        kmpp_loge("dev %s isr failed ret %d\n", dev->dev.name, ret);

    return IRQ_HANDLED;
}

rk_s32 osal_drv_probe(osal_drv_prop *prop, osal_devs **devs)
{
    rk_s32 dev_cnt = 8;
    rk_s32 devs_size = dev_cnt * sizeof(osal_dev *);
    rk_s32 dev_id_cnt = prop->id_cnt + 1;
    rk_s32 dev_id_size = dev_id_cnt * sizeof(struct of_device_id);
    rk_s32 plat_dev_size = sizeof(struct platform_driver);
    rk_s32 total_size = sizeof(osal_drv_impl) + devs_size + dev_id_size + plat_dev_size;
    struct platform_driver *pdev = NULL;
    osal_drv_impl *drv = osal_kcalloc(total_size, osal_gfp_normal);
    rk_s32 i;

    if (!drv) {
        kmpp_loge("osal_dev_init: alloc driver size %d failed\n", total_size);
        *devs = NULL;
        return rk_nok;
    }

    drv->name = prop->name;
    drv->id_cnt = prop->id_cnt;
    drv->ids = (struct of_device_id *)((rk_u8 *)(drv + 1) + devs_size);
    pdev =  (struct platform_driver *)(drv->ids + dev_id_cnt);
    for (i = 0; i < drv->id_cnt; i++) {
        struct of_device_id *id = &drv->ids[i];

        osal_strncpy(id->compatible, prop->ids[i].compatible_name, sizeof(id->compatible));
        id->data = prop->ids[i].drv_data;
    }
    /* keep last id info empty */

    drv->dev_cnt = dev_cnt;
    drv->dops = prop->dops;
    drv->pdev = pdev;
    drv->drv.session_size = prop->session_size;
    drv->drv.task_size = prop->task_size;

    pdev->probe = osal_dev_probe;
    pdev->remove = osal_dev_remove;
    pdev->shutdown = osal_dev_shutdown;
    pdev->suspend = osal_dev_suspend;
    pdev->resume = osal_dev_resume;

    pdev->driver.name = prop->name;
    pdev->driver.of_match_table = of_match_ptr(drv->ids);
    pdev->driver.probe_type = PROBE_FORCE_SYNCHRONOUS;

    OSAL_INIT_LIST_HEAD(&drv->list_srv);
    OSAL_INIT_LIST_HEAD(&drv->list_dev);

    osal_mutex_lock(srv->lock);
    osal_list_add_tail(&drv->list_srv, &srv->list_drv);
    osal_mutex_unlock(srv->lock);

    platform_driver_register(pdev);

    /* all device probe finished check the device list */
    kmpp_logi("%s probe total %d device\n", prop->name, drv->devs.count);

    *devs = &drv->devs;

    return rk_ok;
}
EXPORT_SYMBOL(osal_drv_probe);

rk_s32 osal_drv_release(osal_devs *devs)
{
    if (devs) {
        osal_drv_impl *p = devs_to_drv(devs);

        platform_driver_unregister(p->pdev);

        osal_mutex_lock(srv->lock);
        osal_list_del_init(&p->list_srv);
        osal_mutex_unlock(srv->lock);

        osal_kfree(p);
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_drv_release);

rk_s32 osal_dev_request_irq(osal_dev *dev, osal_irq irq, osal_irq isr, rk_u32 flag)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    rk_s32 ret;

    impl->irq = irq;
    impl->isr = isr;

    ret = devm_request_threaded_irq(impl->device, dev->irq, osal_dev_irq, osal_dev_isr,
                                    IRQF_SHARED, dev_name(impl->device), dev);
    if (ret)
        kmpp_loge("%s request irq %d failed\n", dev_name(impl->device), dev->irq);

    return ret;
}
EXPORT_SYMBOL(osal_dev_request_irq);

rk_s32 osal_dev_request_irq_mmu(osal_dev *dev, osal_irq_mmu handler)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    impl->irq_mmu = handler;

    return rk_ok;
}
EXPORT_SYMBOL(osal_dev_request_irq_mmu);

rk_s32 osal_dev_prop_read_s32(osal_dev *dev, const char *key, rk_s32 *val)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return of_property_read_s32(impl->np, key, val);
}
EXPORT_SYMBOL(osal_dev_prop_read_s32);

rk_s32 osal_dev_prop_read_u32(osal_dev *dev, const char *key, rk_u32 *val)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return of_property_read_u32(impl->np, key, val);
}
EXPORT_SYMBOL(osal_dev_prop_read_u32);

rk_s32 osal_dev_prop_read_string(osal_dev *dev, const char *key, const char **val)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return of_property_read_string(impl->np, key, val);
}
EXPORT_SYMBOL(osal_dev_prop_read_string);

rk_s32 osal_dev_power_on(osal_dev *dev)
{
    return 0;
}
EXPORT_SYMBOL(osal_dev_power_on);

rk_s32 osal_dev_power_off(osal_dev *dev)
{
    return 0;
}
EXPORT_SYMBOL(osal_dev_power_off);

rk_s32 osal_dev_mmu_flush_tlb(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    if (!dev)
        return rk_nok;

    if (impl->domain && impl->domain->ops)
        iommu_flush_iotlb_all(impl->domain);

    return rk_ok;
}
EXPORT_SYMBOL(osal_dev_mmu_flush_tlb);

rk_s32 osal_dev_iommu_refresh(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    rk_s32 ret;

    if (!dev)
        return rk_nok;

    ret = rockchip_iommu_disable(impl->device);
    if (ret)
        return ret;

    return rockchip_iommu_enable(impl->device);
}
EXPORT_SYMBOL(osal_dev_iommu_refresh);

rk_s32 osal_dev_init(void)
{
    rk_s32 mutex_size = osal_mutex_size();

    srv = osal_kcalloc(sizeof(*srv) + mutex_size, osal_gfp_atomic);
    osal_mutex_assign(&srv->lock, srv + 1, mutex_size);
    OSAL_INIT_LIST_HEAD(&srv->list_drv);
    OSAL_INIT_LIST_HEAD(&srv->list_dev);
    srv->dev_uid = 0;

    return rk_ok;
}
EXPORT_SYMBOL(osal_dev_init);

rk_s32 osal_dev_deinit(void)
{
    osal_drv_impl *p, *n;

    osal_mutex_lock(srv->lock);

    osal_list_for_each_entry_safe(p, n, &srv->list_drv, osal_drv_impl, list_srv)
        osal_drv_release(&p->devs);

    osal_mutex_unlock(srv->lock);

    osal_mutex_deinit(&srv->lock);

    osal_kfree(srv);

    return rk_ok;
}
EXPORT_SYMBOL(osal_dev_deinit);

osal_dev *osal_dev_get(const char *name)
{
    osal_dev_impl *p, *n;
    osal_dev *ret = NULL;

    osal_mutex_lock(srv->lock);

    osal_list_for_each_entry_safe(p, n, &srv->list_dev, osal_dev_impl, list_srv) {
        if (osal_strstr(p->dev.name, name)) {
            ret = &p->dev;
            osal_mutex_unlock(srv->lock);
            break;
        }
    }

    osal_mutex_unlock(srv->lock);
    return ret;
}
EXPORT_SYMBOL(osal_dev_get);

void *osal_dev_get_device(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);

    return dev ? impl->device : NULL;
}
EXPORT_SYMBOL(osal_dev_get_device);
