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
#include "kmpp_frame.h"
#include "kmpp_meta.h"

static osal_worker *test_worker = NULL;
static osal_work *test_work = NULL;

typedef struct KmppCallback_t {
    void *arg0;
    rk_u32 arg1;
    rk_u64 arg2;
    rk_s32 (*func)(void *param);
} KmppCallback;

#define ENTRY_TABLE(ENTRY, prefix) \
    ENTRY(prefix, kptr, void *, arg0) \
    ENTRY(prefix, u32, rk_u32, arg1) \
    ENTRY(prefix, u64, rk_u64, arg2) \
    ENTRY(prefix, kfp, void *, func)

static rk_s32 kmpp_obj_test_impl_dump(void *param)
{
    KmppCallback *cb = (KmppCallback *)param;

    kmpp_log_f("arg0 %px\n", cb->arg0);
    kmpp_log_f("arg1 %d\n", cb->arg1);
    kmpp_log_f("arg2 %lld\n", cb->arg2);

    return 0;
}

 static rk_s32 kmpp_obj_test_impl_init(void *obj, osal_fs_dev *file, const rk_u8 *caller)
 {
    KmppCallback *cb = (KmppCallback *)obj;

    cb->arg0 = NULL;
    cb->arg1 = 1234;
    cb->arg2 = 5678;
    cb->func = kmpp_obj_test_impl_dump;

    return rk_ok;
 }

#define KMPP_OBJ_NAME           kmpp_obj_test
#define KMPP_OBJ_INTF_TYPE      MppFrame
#define KMPP_OBJ_IMPL_TYPE      KmppCallback
#define KMPP_OBJ_ENTRY_TABLE    ENTRY_TABLE
#define KMPP_OBJ_FUNC_INIT      kmpp_obj_test_impl_init
#define KMPP_OBJ_FUNC_DUMP      kmpp_obj_test_impl_dump
#include "kmpp_obj_helper.h"

static KmppObjDef test_def = NULL;

rk_s32 kmpp_test_func(void *param)
{
    KmppObj obj = NULL;
    rk_s32 ret = rk_nok;
    MppFrame frame;
    KmppTrie trie1;
    KmppTrie trie2;
    KmppMeta meta = NULL;
    KmppShmPtr sptr;
    void *root;

    kmpp_objdef_dump_all();

    ret = kmpp_frame_get(&frame);
    kmpp_logi_f("get frame %px ret %d\n", frame, ret);
    if (ret)
        goto done;

    kmpp_frame_get_meta(frame, &sptr);
    meta = sptr.kptr;

    kmpp_logi_f("get meta ret %px\n", meta);

    kmpp_meta_dump(meta);

    kmpp_frame_dump_f(frame);

    ret = kmpp_frame_put(frame);
    kmpp_logi_f("put frame %px ret %d\n", frame, ret);
    if (ret)
        goto done;

    kmpp_obj_test_init();

    kmpp_logi_f("obj init done %px\n", kmpp_obj_test_def);

    trie1 = kmpp_objdef_get_trie(kmpp_obj_test_def);
    if (!trie1) {
        kmpp_loge("get trie failed\n");
        goto done;
    }

    root = kmpp_trie_get_node_root(trie1);
    if (!root) {
        kmpp_loge("get trie root failed\n");
        goto done;
    }

    ret = kmpp_trie_init_by_root(&trie2, root);
    if (ret) {
        kmpp_loge("trie2 init by root failed\n");
        goto done;
    }

    /* release trie2 but not release root */
    kmpp_trie_deinit(trie2);

    kmpp_obj_get_f(&obj, kmpp_obj_test_def);

    kmpp_obj_set_kptr(obj, "arg0", NULL);
    kmpp_obj_set_s32(obj, "arg1", 1);
    kmpp_obj_set_u64(obj, "arg2", 2);
    kmpp_obj_set_kfp(obj, "func", kmpp_obj_test_impl_dump);

    kmpp_logi_f("kmpp_show %px\n", kmpp_obj_test_impl_dump);

    kmpp_obj_run(obj, "func");

    kmpp_obj_put_f(obj);

    kmpp_objdef_find(&test_def, "kmpp_call");

    kmpp_logi_f("lookup kmpp_call ret %px\n", test_def);

    if (test_def) {
        kmpp_logi_f("found kmpp_call\n");

        kmpp_obj_get_f(&obj, test_def);
        kmpp_logi_f("get kmpp_call ret %px\n", obj);
        kmpp_obj_run(obj, "func");
        kmpp_obj_put_f(obj);

        kmpp_objdef_put(test_def);

        ret = rk_ok;
    }

done:
    kmpp_logi("kmpp test func done %s\n", ret ? "failed" : "success");

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

    if (kmpp_obj_test_def) {
        kmpp_objdef_put(kmpp_obj_test_def);
        kmpp_obj_test_def = NULL;
    }

    kmpp_test_finish();
}

module_init(kmpp_test_init);
module_exit(kmpp_test_exit);

MODULE_AUTHOR("rockchip");
MODULE_LICENSE("proprietary");
MODULE_VERSION("1.0");