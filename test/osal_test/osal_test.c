/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "osal_test"

#include <linux/module.h>
#include <linux/kernel.h>

#include "kmpp_osal.h"

static osal_worker *test_worker = NULL;
static osal_work *test_work = NULL;

static osal_cls *test_cls = NULL;
static osal_fs_dev_mgr *test_fs_dev_mgr = NULL;

static rk_u32 test_env_u64 = 0;
static rk_u32 test_env_u32 = 0;
static char *test_env_str = "this is a test env string";

static KmppEnvNode env_node_u64 = NULL;
static KmppEnvNode env_node_u32 = NULL;
static KmppEnvNode env_node_str = NULL;
static KmppEnvNode env_node_usr = NULL;

rk_s32 node_thds_func(void *data)
{
    kmpp_thds *thd = (kmpp_thds *)data;

    kmpp_logi("run idx %d\n", thd->idx);
    return 0;
}

rk_s32 osal_test_open(osal_fs_dev *ctx)
{
    kmpp_logi_f("\n");
    return rk_ok;
}

rk_s32 osal_test_release(osal_fs_dev *ctx)
{
    kmpp_logi_f("\n");
    return rk_ok;
}

rk_s32 osal_test_ioctl(osal_fs_dev *ctx, rk_s32 cmd, void *arg)
{
    kmpp_logi_f("\n");
    return rk_ok;
}

osal_dev_fops test_fops = {
    .open       = osal_test_open,
    .release    = osal_test_release,
    .ioctl      = osal_test_ioctl,
};

#define DMABUF_COUNT 100
#define DMABUF_SIZE  (720*480*2)

void test_env_show(KmppEnvNode node, void *data)
{
    kmpp_env_log(node, "%s enter data %px\n", __FUNCTION__, data);
}

rk_s32 osal_test_func(void *param)
{
    osal_fs_dev_cfg cfg;
    KmppEnvInfo info;
    osal_file *tmp_file = NULL;
    kmpp_thds *thds = NULL;
    osal_dev *odev = NULL;
    void *ptr = NULL;
    void *ptr_shm = NULL;
    KmppDmaHeap heap = NULL;
    KmppDmaBuf buf = NULL;
    rk_u64 uptr;
    rk_s32 size;
    rk_s32 i;

    kmpp_logi("osal test func start\n");

    ptr = osal_kmalloc(4096, osal_gfp_normal);
    ptr_shm = osal_malloc_share(4096);

    kmpp_logi("malloc get %px shm %px\n", ptr, ptr_shm);

    kmpp_thds_init(&thds, "node_thds", 4);
    kmpp_thds_setup(thds, node_thds_func, ptr);

    for (i = 0; i < 20; i++)
        kmpp_thds_run_nl(thds);

    kmpp_logi("node init ptr %px\n", ptr);

    kmpp_thds_deinit(&thds);

    osal_kfree(ptr);
    osal_kfree_share(ptr_shm);

    tmp_file = osal_fopen("/sdcard/hello.txt", OSAL_O_CREAT | OSAL_O_RDWR, 0666);
    if (tmp_file) {
        char str[] = "hello world\n";
        char buf[32];

        osal_fwrite(tmp_file, str, osal_strlen(str));
        osal_fseek(tmp_file, 0, OSAL_SEEK_SET);
        osal_fread(tmp_file, buf, sizeof(buf));

        printk("read file string: %s", buf);
        osal_fclose(tmp_file);
    } else {
        printk("tmp_file is invalid\n");
    }

    kmpp_logi("osal class test start\n");

    osal_class_init(&test_cls, "osal_test");

    cfg.cls = test_cls;
    cfg.name = "osal_fs_dev";
    cfg.fops = &test_fops;
    cfg.drv_data = NULL;
    cfg.priv_size = 0;
    cfg.buf = NULL;
    cfg.size = 0;

    osal_fs_dev_mgr_init(&test_fs_dev_mgr, &cfg);

    kmpp_logi("fs dev %s registered manager %px\n", cfg.name, test_fs_dev_mgr);

    odev = osal_dev_get("vdpp");
    if (odev) {
        osal_dmabuf *buf[DMABUF_COUNT];

        kmpp_logi("osal dmabuf test start\n");

        for (i = 0; i < DMABUF_COUNT; i++)
            osal_dmabuf_alloc(&buf[i], odev, DMABUF_SIZE, 0);

        for (i = 0; i < DMABUF_COUNT; i++) {
            osal_memset(buf[i]->kaddr, 0xff, DMABUF_SIZE);

            osal_dmabuf_sync_for_dev(buf[i], OSAL_DMA_TO_DEVICE);

            osal_dmabuf_sync_for_cpu(buf[i], OSAL_DMA_FROM_DEVICE);
        }

        for (i = 0; i < DMABUF_COUNT; i++)
            osal_dmabuf_free(buf[i]);

        kmpp_logi("osal dmabuf test donet\n");
    }

    kmpp_logi("osal env test start\n");

    info.name = "osal_test_u32";
    info.readonly = 0;
    info.type = KmppEnv_u32;
    info.val = &test_env_u32;
    info.env_show = NULL;

    kmpp_env_add(kmpp_env_osal, &env_node_u32, &info);

    info.name = "osal_test_u64";
    info.readonly = 0;
    info.type = KmppEnv_u64;
    info.val = &test_env_u64;
    info.env_show = NULL;

    kmpp_env_add(kmpp_env_osal, &env_node_u64, &info);

    info.name = "osal_test_str";
    info.readonly = 0;
    info.type = KmppEnv_str;
    info.val = test_env_str;
    info.env_show = NULL;

    kmpp_env_add(kmpp_env_osal, &env_node_str, &info);

    info.name = "osal_test_user";
    info.readonly = 0;
    info.type = KmppEnv_user;
    info.val = NULL;
    info.env_show = test_env_show;

    kmpp_env_add(kmpp_env_osal, &env_node_usr, &info);

    kmpp_logi("osal env test done\n");

    kmpp_logi("kmpp_dmabuf test start\n");

    kmpp_dmaheap_open_f(&heap, KMPP_DMAHEAP_FLAGS_DMA32);

    kmpp_logi("kmpp_dmaheap_open ret %px\n", heap);

    size = 4096;

    kmpp_dmabuf_alloc_f(&buf, heap, size, 0);
    ptr = kmpp_dmabuf_get_kptr(buf);
    uptr = kmpp_dmabuf_get_uptr(buf);

    kmpp_logi("kmpp_dmabuf_alloc normal  %px [k:u] [%px:%llx]\n", buf, ptr, uptr);
    memset(ptr, 0xff, size);
    if (((rk_u8 *)ptr)[0] == 0xff && ((rk_u8 *)ptr)[size - 1] == 0xff)
        kmpp_logi("access ok\n");
    kmpp_dmabuf_free_f(buf);

    kmpp_dmabuf_alloc_f(&buf, heap, size, KMPP_DMABUF_FLAGS_DUP_MAP);
    ptr = kmpp_dmabuf_get_kptr(buf);
    uptr = kmpp_dmabuf_get_uptr(buf);
    kmpp_logi("kmpp_dmabuf_alloc dup-map %px [k:u] [%px:%llx]\n", buf, ptr, uptr);
    memset(ptr, 0xff, size * 2);
    if (((rk_u8 *)ptr)[0] == 0xff && ((rk_u8 *)ptr)[size * 2 - 1] == 0xff)
        kmpp_logi("access ok\n");
    kmpp_dmabuf_free_f(buf);

    kmpp_dmaheap_close_f(heap);

    kmpp_logi("kmpp_dmabuf test done\n");

    kmpp_logi("osal test func done\n");

    return rk_ok;
}

void osal_test_end(void)
{
    kmpp_env_del(kmpp_env_osal, env_node_u32);
    kmpp_env_del(kmpp_env_osal, env_node_u64);
    kmpp_env_del(kmpp_env_osal, env_node_str);
    kmpp_env_del(kmpp_env_osal, env_node_usr);

    osal_fs_dev_mgr_deinit(test_fs_dev_mgr);
    osal_class_deinit(test_cls);

    kmpp_logi("osal class test done\n");
}

int osal_test_init(void)
{
    kmpp_logi("osal test start\n");

    osal_worker_init(&test_worker, "osal test");
    osal_work_init(&test_work, osal_test_func, NULL);

    osal_work_queue(test_worker, test_work);

    return 0;
}

void osal_test_exit(void)
{
    osal_worker_deinit(&test_worker);
    osal_work_deinit(&test_work);

    osal_test_end();

    kmpp_logi("osal test end\n");
}

module_init(osal_test_init);
module_exit(osal_test_exit);

MODULE_AUTHOR("rockchip");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");