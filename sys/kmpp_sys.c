/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_sys"

#include <linux/module.h>
#include <linux/kernel.h>

#include "version.h"

#include "kmpp_osal.h"

#include "kmpp_sym.h"
#include "kmpp_shm.h"
#include "kmpp_frame.h"
#include "kmpp_venc_objs.h"

KmppEnvGrp kmpp_env_sys;
EXPORT_SYMBOL(kmpp_env_sys);

static KmppSym sys_sym = NULL;

static void sys_version_show(KmppEnvNode node, void *data)
{
    kmpp_env_log(node, "kmpp sys version: %s\n", KMPP_VERSION);
}

static void sys_history_show(KmppEnvNode node, void *data)
{
    static const rk_u32 kmpp_history_cnt = KMPP_VER_HIST_CNT;
    static const char *kmpp_history[]  = {
        KMPP_VER_HIST_0,
        KMPP_VER_HIST_1,
        KMPP_VER_HIST_2,
        KMPP_VER_HIST_3,
        KMPP_VER_HIST_4,
        KMPP_VER_HIST_5,
        KMPP_VER_HIST_6,
        KMPP_VER_HIST_7,
        KMPP_VER_HIST_8,
        KMPP_VER_HIST_9,
    };
    rk_u32 i;

    kmpp_env_log(node, "kmpp sys version history %d:\n", kmpp_history_cnt);
    for (i = 0; i < kmpp_history_cnt; i++) {
        kmpp_env_log(node, "%s\n", kmpp_history[i]);
    }
}

static void sys_env_version_init(void)
{
    KmppEnvInfo info;

    info.name = "version";
    info.readonly = 1;
    info.type = KmppEnv_user;
    info.val = NULL;
    info.env_show = sys_version_show;

    kmpp_env_add(kmpp_env_sys, NULL, &info);

    info.name = "version_history";
    info.readonly = 1;
    info.type = KmppEnv_user;
    info.val = NULL;
    info.env_show = sys_history_show;

    kmpp_env_add(kmpp_env_sys, NULL, &info);
}

static void sys_env_init(void)
{
    kmpp_logi("sys env init\n");

    kmpp_env_get(&kmpp_env_sys, NULL, "kmpp_sys");
}

static void sys_env_deinit(void)
{
    kmpp_logi("sys env deinit\n");

    kmpp_env_put(kmpp_env_sys);
    kmpp_env_sys = NULL;
}

static rk_s32 sys_func_sample(void *arg, const rk_u8 *caller)
{
    kmpp_logi("sys func sample called arg %px at %s\n", arg, caller);
    return rk_ok;
}

int sys_init(void)
{
    kmpp_logi("sys init\n");

    sys_env_init();
    sys_env_version_init();

    kmpp_sym_init();
    kmpp_symdef_get(&sys_sym, "sys");
    /* Add sys export funciton here */
    if (sys_sym) {
        kmpp_symdef_add(sys_sym, "sample", sys_func_sample);
        kmpp_symdef_install(sys_sym);
    }

    kmpp_shm_init();
    kmpp_frame_init();
    kmpp_venc_init_cfg_init();

    return 0;
}

void sys_exit(void)
{
    kmpp_logi("sys exit\n");

    kmpp_venc_init_cfg_deinit();
    kmpp_frame_deinit();
    kmpp_shm_deinit();

    if (sys_sym) {
        kmpp_symdef_uninstall(sys_sym);
        kmpp_symdef_put(sys_sym);
        sys_sym = NULL;
    }
    kmpp_sym_deinit();

    sys_env_deinit();
}
#ifndef BUILD_ONE_KO
module_init(sys_init);
module_exit(sys_exit);

MODULE_AUTHOR("rockchip");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
#endif
