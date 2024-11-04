/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_clk"

#include <linux/clk.h>
#include <linux/of_clk.h>
#include <linux/of_platform.h>

#include "kmpp_log.h"
#include "kmpp_mem.h"
#include "kmpp_dev_impl.h"
#include "kmpp_string.h"

rk_s32 osal_clk_probe(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    struct device_node *np = dev_of_node(impl->device);
    const char *name = dev_name(impl->device);
    kmpp_clk *clk;
    rk_s32 clk_cnt;
    rk_s32 i;

    clk_cnt = of_clk_get_parent_count(np);
    if (clk_cnt <= 0) {
        kmpp_loge("%s has no clk\n", name);
        impl->clk = NULL;
        return rk_nok;
    }

    clk = osal_kcalloc(sizeof(*clk) + sizeof(clk->clks[0]) * clk_cnt, osal_gfp_normal);
    if (!clk) {
        kmpp_loge("%s alloc clk failed\n", name);
        return rk_nok;
    }

    impl->clk = clk;
    clk->clk_cnt = clk_cnt;

    for (i = 0; i < clk_cnt; i++) {
        kmpp_clk_info *info = &clk->clks[i];

        of_property_read_string_index(np, "clock-names", i, &info->name);
        info->clk = of_clk_get(np, i);
        if (IS_ERR_OR_NULL(info->clk)) {
            clk->clk_cnt = i;
            info->clk = NULL;
            kmpp_loge("%s get clk %d failed\n", name, i);
            return rk_nok;
        }

        kmpp_logi("%s clk %d - %-6s : 0x%px\n", name, i, info->name, info->clk);
    }

    return rk_ok;
}

rk_s32 osal_clk_release(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    const char *name = dev_name(impl->device);
    kmpp_clk *clk = impl->clk;

    if (clk) {
        rk_s32 i;

        for (i = 0; i < clk->clk_cnt; i++) {
            kmpp_clk_info *info = &clk->clks[i];

            if (info->clk) {
                clk_put(info->clk);
                info->clk = NULL;
            }
        }

        osal_kfree(clk);
        impl->clk = NULL;
    }

    kmpp_logi("%s clk release\n", name);
    return rk_ok;
}

rk_s32 osal_clk_get_index(osal_dev *dev, const char *name)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;
    rk_s32 i;

    if (clk) {
        for (i = 0; i < clk->clk_cnt; i++) {
            if (!osal_strcmp(clk->clks[i].name, name))
                return i;
        }
    }

    return -1;
}
EXPORT_SYMBOL(osal_clk_get_index);

rk_s32 osal_clk_set_rate_tbl(osal_dev *dev, rk_s32 index, osal_clk_rate_tbl *tbl)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;

    if (clk && index >= 0 && index < clk->clk_cnt) {
        kmpp_clk_info *info = &clk->clks[index];

        osal_memcpy(info->rate_tbl, tbl, sizeof(info->rate_tbl));
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_clk_set_rate_tbl);

rk_s32 osal_clk_update_rate_mode(osal_dev *dev, rk_s32 index, osal_clk_mode mode, rk_u32 rate)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;

    if (clk && index >= 0 && index < clk->clk_cnt &&
        mode >= CLK_MODE_BASE && mode < CLK_MODE_BUTT) {
        kmpp_clk_info *info = &clk->clks[index];

        info->rate_tbl[mode] = rate;
        return rk_ok;
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_clk_update_rate_mode);

rk_s32 osal_clk_get_rate(osal_dev *dev, rk_s32 index, osal_clk_mode mode)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;

    if (clk && index >= 0 && index < clk->clk_cnt &&
        mode >= CLK_MODE_BASE && mode < CLK_MODE_BUTT) {
        kmpp_clk_info *info = &clk->clks[index];

        return info->rate_tbl[mode];
    }

    return 0;
}
EXPORT_SYMBOL(osal_clk_get_rate);

rk_s32 osal_clk_set_rate(osal_dev *dev, rk_s32 index, osal_clk_mode mode)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;

    if (clk && index >= 0 && index < clk->clk_cnt &&
        mode >= CLK_MODE_BASE && mode < CLK_MODE_BUTT) {
        kmpp_clk_info *info = &clk->clks[index];
        rk_u32 clk_rate_hz = info->rate_tbl[mode];

        if (info->clk && clk_rate_hz) {
            clk_set_rate(info->clk, clk_rate_hz);
            info->rate_real[mode] = clk_get_rate(info->clk);
            return rk_ok;
        }
    }

    return rk_nok;
}

rk_s32 osal_clk_on(osal_dev *dev, rk_s32 index)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;

    if (clk && index >= 0 && index < clk->clk_cnt) {
        kmpp_clk_info *info = &clk->clks[index];

        if (info->clk)
            return clk_prepare_enable(info->clk);
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_clk_on);

rk_s32 osal_clk_off(osal_dev *dev, rk_s32 index)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;

    if (clk && index >= 0 && index < clk->clk_cnt) {
        kmpp_clk_info *info = &clk->clks[index];

        if (info->clk) {
            clk_disable_unprepare(info->clk);
            return rk_ok;
        }
    }

    return rk_nok;
}
EXPORT_SYMBOL(osal_clk_off);

rk_s32 osal_clk_on_all(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;
    rk_s32 i;

    if (!clk)
        return rk_nok;

    for (i = 0; i < clk->clk_cnt; i++) {
        kmpp_clk_info *info = &clk->clks[i];

        if (info->clk)
            clk_prepare_enable(info->clk);
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_clk_on_all);

rk_s32 osal_clk_off_all(osal_dev *dev)
{
    osal_dev_impl *impl = dev_to_impl(dev);
    kmpp_clk *clk = impl->clk;
    rk_s32 i;

    if (!clk)
        return rk_nok;

    for (i = 0; i < clk->clk_cnt; i++) {
        kmpp_clk_info *info = &clk->clks[i];

        if (info->clk)
            clk_disable_unprepare(info->clk);
    }

    return rk_ok;
}
EXPORT_SYMBOL(osal_clk_off_all);