/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_test"

#include <linux/module.h>
#include <linux/kernel.h>

#include "kmpp_osal.h"

#include "kmpp_obj.h"
#include "kmpp_shm.h"
#include "kmpp_buffer.h"
#include "kmpp_frame.h"
#include "kmpp_meta.h"

#define TEST_DISABLE        1
#define TEST_DETAIL         2
#define TEST_DUMP           4

#define is_flag_set(val, flag)   (val && (*val & flag))

#define test_detail(fmt, ...) \
    do { \
        if (is_flag_set(flag, TEST_DETAIL)) \
            kmpp_logi_f(fmt, ##__VA_ARGS__); \
    } while (0)

static osal_worker *test_worker = NULL;
static osal_work *test_work = NULL;

rk_u32 objdef_dump_flag;
module_param(objdef_dump_flag, uint, 0644);
MODULE_PARM_DESC(objdef_dump_flag, "objdef dump flag");

static rk_s32 kmpp_objdef_dump_all_test(const rk_u8 *name, rk_u32 *flag, void *ctx)
{
    if (is_flag_set(flag, TEST_DUMP))
        kmpp_objdef_dump_all();

    return 0;
}

rk_u32 kmpp_frame_flag;
module_param(kmpp_frame_flag, uint, 0644);
MODULE_PARM_DESC(kmpp_frame_flag, "kmpp frame test flag");

static rk_s32 kmpp_frame_test(const rk_u8 *name, rk_u32 *flag, void *ctx)
{
    KmppShmPtr sptr;
    KmppBuffer buffer = NULL;
    KmppFrame frame = NULL;
    KmppMeta meta = NULL;
    rk_s32 ret = rk_nok;

    ret = kmpp_frame_get(&frame);
    test_detail("get frame %px ret %d\n", frame, ret);
    if (ret)
        goto done;

    sptr.uaddr = 0;
    sptr.kaddr = 0;
    ret = kmpp_frame_get_meta(frame, &sptr);
    meta = sptr.kptr;

    test_detail("get meta %px ret %d\n", meta, ret);

    ret = kmpp_frame_get_buffer(frame, &buffer);

    test_detail("get buffer %px ret %d\n", buffer, ret);

    if (is_flag_set(flag, TEST_DUMP))
        kmpp_frame_dump_f(frame);

    if (is_flag_set(flag, TEST_DUMP))
        kmpp_meta_dump(meta);

    if (is_flag_set(flag, TEST_DUMP))
        kmpp_buffer_dump(buffer, __FUNCTION__);

done:
    if (frame) {
        ret = kmpp_frame_put(frame);
        test_detail("put frame %px ret %d\n", frame, ret);
    }
    return ret;
}


#define ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, kptr, void *, arg0) \
    ENTRY(prefix, u32, rk_u32, arg1) \
    ENTRY(prefix, u64, rk_u64, arg2) \
    ENTRY(prefix, kfp, void *, func)

typedef struct KmppObjMacroTestImpl_t {
    void *arg0;
    rk_u32 arg1;
    rk_u64 arg2;
    rk_s32 (*func)(void *ctx, void *arg);
} KmppObjMacroTestImpl;

static rk_s32 kmpp_obj_test_impl_show(void *ctx, void *arg)
{
    KmppObjMacroTestImpl *cb = (KmppObjMacroTestImpl *)ctx;

    kmpp_log_f("arg0 %px\n", cb->arg0);
    kmpp_log_f("arg1 %d\n", cb->arg1);
    kmpp_log_f("arg2 %lld\n", cb->arg2);

    return 0;
}

static rk_s32 kmpp_obj_test_impl_dump(void *ctx)
{
    return kmpp_obj_test_impl_show(ctx, NULL);
}

static rk_s32 kmpp_obj_test_impl_init(void *obj, osal_fs_dev *file, const rk_u8 *caller)
{
    KmppObjMacroTestImpl *cb = (KmppObjMacroTestImpl *)obj;

    cb->arg0 = NULL;
    cb->arg1 = 1234;
    cb->arg2 = 5678;
    cb->func = kmpp_obj_test_impl_show;

    return rk_ok;
}

#define KMPP_OBJ_NAME           kmpp_obj_test
#define KMPP_OBJ_INTF_TYPE      void *
#define KMPP_OBJ_IMPL_TYPE      KmppObjMacroTestImpl
#define KMPP_OBJ_ENTRY_TABLE    ENTRY_TABLE
#define KMPP_OBJ_FUNC_INIT      kmpp_obj_test_impl_init
#define KMPP_OBJ_FUNC_DUMP      kmpp_obj_test_impl_dump
#include "kmpp_obj_helper.h"

rk_u32 kmpp_obj_macro_flag;
module_param(kmpp_obj_macro_flag, uint, 0644);
MODULE_PARM_DESC(kmpp_obj_macro_flag, "kmpp obj macro test flag");

static rk_s32 kmpp_obj_macro_test(const rk_u8 *name, rk_u32 *flag, void *ctx)
{
    KmppObj obj = NULL;
    KmppTrie src;
    KmppTrie dst;
    void *root;
    rk_s32 ret = rk_nok;

    ret = kmpp_obj_test_init();
    if (ret) {
        kmpp_loge("%s objdef init failed\n", name);
        goto done;
    }

    test_detail("obj init get def %px\n", kmpp_obj_test_def);

    src = kmpp_objdef_get_trie(kmpp_obj_test_def);
    if (!src) {
        kmpp_loge("get original trie failed\n");
        ret = rk_nok;
        goto done;
    }

    root = kmpp_trie_get_node_root(src);
    if (!root) {
        kmpp_loge("get original trie root failed\n");
        ret = rk_nok;
        goto done;
    }

    ret = kmpp_trie_init_by_root(&dst, root);
    if (ret) {
        kmpp_loge("get new trie by original root failed\n");
        goto done;
    }

    test_detail("obj trie root test done\n");

    /* release dst but not release root */
    kmpp_trie_deinit(dst);

    ret = kmpp_obj_get_f(&obj, kmpp_obj_test_def);
    if (ret) {
        kmpp_loge("get obj failed\n");
        goto done;
    }

    test_detail("obj get done\n");

    ret |= kmpp_obj_set_kptr(obj, "arg0", NULL);
    ret |= kmpp_obj_set_s32(obj, "arg1", 1);
    ret |= kmpp_obj_set_u64(obj, "arg2", 2);
    ret |= kmpp_obj_set_kfp(obj, "func", kmpp_obj_test_impl_dump);

    if (ret) {
        kmpp_loge("set obj failed\n");
        goto done;
    }

    test_detail("obj set done\n");

    if (is_flag_set(flag, TEST_DUMP)) {
        ret = kmpp_obj_run(obj, "func", NULL);
        if (ret)
            kmpp_loge("run obj func failed\n");
    }

done:
    if (obj) {
        kmpp_obj_put_f(obj);
        obj = NULL;
    }

    kmpp_obj_test_deinit();

    return ret;
}

rk_u32 kmpp_buffer_flag;
module_param(kmpp_buffer_flag, uint, 0644);
MODULE_PARM_DESC(kmpp_buffer_flag, "kmpp buffer test flag");

static rk_s32 kmpp_buffer_test(const rk_u8 *name, rk_u32 *flag, void *ctx)
{
    KmppShmPtr sptr;
    KmppBufGrp grp = NULL;
    KmppBufGrpCfg grp_cfg = NULL;
    KmppBuffer buf = NULL;
    KmppBufCfg buf_cfg = NULL;
    void *kptr;
    rk_s32 ret = rk_nok;

    ret = kmpp_buf_grp_get(&grp);
    if (ret) {
        kmpp_loge("get buf grp failed\n");
        goto done;
    }

    test_detail("get buf grp - success\n");

    grp_cfg = kmpp_buf_grp_get_cfg_k(grp);
    if (!grp_cfg) {
        kmpp_loge("get buf grp cfg failed\n");
        ret = rk_nok;
        goto done;
    }

    test_detail("get buf grp cfg - success\n");

    sptr.kptr = "rk dma heap";
    sptr.uaddr = 0;
    ret |= kmpp_buf_grp_cfg_set_allocator(grp_cfg, &sptr);
    sptr.kptr = "test";
    sptr.uaddr = 0;
    ret |= kmpp_buf_grp_cfg_set_name(grp_cfg, &sptr);
    ret |= kmpp_buf_grp_cfg_set_count(grp_cfg, 16);
    ret |= kmpp_buf_grp_cfg_set_size(grp_cfg, 4096);
    if (ret) {
        kmpp_loge("set buf grp cfg failed\n");
        goto done;
    }

    test_detail("set buf grp cfg - success\n");

    ret = kmpp_buf_grp_setup_f(grp);
    if (ret) {
        kmpp_loge("setup buf grp failed\n");
        goto done;
    }

    test_detail("setup buf grp - success\n");

    if (is_flag_set(flag, TEST_DUMP))
        kmpp_buf_grp_dump(grp, __FUNCTION__);

    ret = kmpp_buffer_get(&buf);
    if (ret) {
        kmpp_loge("get buffer failed\n");
        goto done;
    }

    test_detail("get buffer - success\n");

    buf_cfg = kmpp_buffer_get_cfg_k(buf);
    if (!buf_cfg) {
        kmpp_loge("get buffer cfg failed\n");
        ret = rk_nok;
        goto done;
    }

    test_detail("get buffer cfg - success\n");

    sptr.kptr = grp;
    sptr.uaddr = 0;
    ret |= kmpp_buf_cfg_set_group(buf_cfg, &sptr);
    ret |= kmpp_buf_cfg_set_size(buf_cfg, 4096);
    ret |= kmpp_buf_cfg_set_offset(buf_cfg, 1024);
    ret |= kmpp_buf_cfg_set_flag(buf_cfg, 0);
    ret |= kmpp_buf_cfg_set_index(buf_cfg, 1);
    if (ret) {
        kmpp_loge("set buffer cfg failed\n");
        goto done;
    }

    test_detail("set buffer cfg - success\n");

    ret = kmpp_buffer_setup_f(buf, NULL);
    if (ret) {
        kmpp_loge("setup buffer failed\n");
        goto done;
    }

    test_detail("setup buffer - success\n");

    kptr = kmpp_buffer_get_kptr(buf);
    if (!kptr) {
        kmpp_loge("get buffer kptr failed\n");
        ret = rk_nok;
        goto done;
    }

    test_detail("get buffer kptr - success\n");

    osal_memset(kptr, 0xff, 128);

    test_detail("memset buffer - success\n");

    ret = kmpp_buffer_inc_ref_f(buf);
    if (ret) {
        kmpp_loge("inc buffer ref failed\n");
        goto done;
    }

    test_detail("inc buffer ref - success\n");

    ret = kmpp_buffer_put(buf);
    if (ret) {
        kmpp_loge("put buffer failed\n");
        goto done;
    }

    test_detail("put buffer - success\n");

done:
    if (buf) {
        kmpp_buffer_put(buf);
        buf = NULL;
    }

    if (grp) {
        kmpp_buf_grp_put(grp);
        grp = NULL;
    }

    return ret;
}

typedef struct KmppTest_t {
    const rk_u8     *name;
    void            *ctx;
    rk_u32          *flag;
    rk_s32          (*func)(const rk_u8 *name, rk_u32 *flag, void *ctx);
} KmppTest;

static KmppTest tests[] = {
    [0] = {
        .name = "dump all objdef",
        .ctx = NULL,
        .flag = &objdef_dump_flag,
        .func = kmpp_objdef_dump_all_test,
    },
    [1] = {
        .name = "KmppFrame",
        .ctx = NULL,
        .flag = &kmpp_frame_flag,
        .func = kmpp_frame_test,
    },
    [2] = {
        .name = "kmpp obj macro",
        .ctx = NULL,
        .flag = &kmpp_obj_macro_flag,
        .func = kmpp_obj_macro_test,
    },
    [3] = {
        .name = "kmpp buffer",
        .ctx = NULL,
        .flag = &kmpp_buffer_flag,
        .func = kmpp_buffer_test,
    },
};

rk_s32 kmpp_test_func(void *param)
{
    rk_s32 ret = rk_ok;
    rk_s32 i;

    for (i = 0; i < ARRAY_SIZE(tests); i++) {
        KmppTest *test = &tests[i];
        const rk_u8 *name = test->name;
        rk_u32 *flag = test->flag;
        void *ctx = test->ctx;

        if (is_flag_set(flag, TEST_DISABLE)) {
            kmpp_logi("test %-16s disabled\n", name);
            continue;
        }

        ret = test->func(name, flag, ctx);
        if (ret) {
            kmpp_logi("test %-16s failed ret %d\n", name, ret);
            goto done;
        }
        kmpp_logi("test %-16s success\n", name);
    }

done:
    kmpp_logi("done %s \n", ret ? "failed" : "success");
    return ret;
}

void kmpp_test_prepare(void)
{
    kmpp_logi("kmpp test start\n");
}

void kmpp_test_finish(void)
{
    kmpp_obj_test_deinit();
    kmpp_logi("kmpp test end\n");
}

int kmpp_test_init(void)
{
    kmpp_test_prepare();

    osal_worker_init(&test_worker, "kmpp test");
    osal_work_init(&test_work, kmpp_test_func, NULL);

    osal_work_queue(test_worker, test_work);

    return 0;
}

void kmpp_test_exit(void)
{
    osal_worker_deinit(&test_worker);
    osal_work_deinit(&test_work);

    kmpp_test_finish();
}

module_init(kmpp_test_init);
module_exit(kmpp_test_exit);

MODULE_AUTHOR("rockchip");
MODULE_LICENSE("proprietary");
MODULE_VERSION("1.0");