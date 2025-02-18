/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#include "kmpp_log.h"
#include "kmpp_string.h"

#include "kmpp_vsp_impl.h"
#include "kmpp_vsp_objs.h"

#include "vepu500_pp.h"

static KmppVspApi *vsp_apis[] = {
#ifdef RKVEPU500_PP_ENABLE
    &vepu500_pp_api,
#endif
    NULL,
};

rk_s32 kmpp_vsp_drv_register_all(void)
{
    kmpp_logi_f("\n");

    kmpp_vsp_init_cfg_init();
    kmpp_vsp_pp_rt_cfg_init();
    kmpp_vsp_pp_rt_out_init();

    return rk_ok;
}

void kmpp_vsp_drv_unregister_all(void)
{
    kmpp_vsp_init_cfg_deinit();
    kmpp_vsp_pp_rt_cfg_deinit();
    kmpp_vsp_pp_rt_out_deinit();

    kmpp_logi_f("\n");
}

KmppVspApi *kmpp_vsp_get_api(const rk_u8 *name)
{
    rk_s32 i;

    for (i = 0; vsp_apis[i]; i++) {
        KmppVspApi *api = vsp_apis[i];

        if (!api)
            break;

        kmpp_logd_f("compare api vs name: %s - %s\n", api->name, name);

        if (!osal_strcmp(api->name, name))
            return api;
    }

    kmpp_loge_f("vsp api %s not found\n", name);

    return NULL;
}
