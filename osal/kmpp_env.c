/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_env"

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "rk_list.h"

#include "kmpp_macro.h"
#include "kmpp_string.h"
#include "kmpp_log.h"
#include "kmpp_env.h"
#include "kmpp_mem.h"
#include "kmpp_spinlock.h"

typedef struct KmppEnvGrpImpl_t {
    rk_u8 name[64];
    /* list to global list_env_grp */
    osal_list_head list_group;
    /* list to parent groups */
    osal_list_head list_parent;
    /* list for child groups */
    osal_list_head list_child;
    /* list for nodes in the group */
    osal_list_head list_nodes;

    rk_u32 is_root;
    struct proc_dir_entry *dir;
} KmppEnvGrpImpl;

typedef struct KmppEnvNodeImpl_t {
    osal_list_head list;
    KmppEnvInfo info;
    rk_s32 str_val_size;
    KmppEnvGrpImpl *grp;
    struct seq_file *seq;
} KmppEnvNodeImpl;

static osal_list_head list_env_grp;
static osal_list_head list_env_node;
static osal_spinlock *lock_env;

KmppEnvGrp kmpp_env_osal;
EXPORT_SYMBOL(kmpp_env_osal);

KmppEnvGrp kmpp_env_debug;
EXPORT_SYMBOL(kmpp_env_debug);

void kmpp_env_log(KmppEnvNode node, const rk_u8 *fmt, ...)
{
    KmppEnvNodeImpl *impl = (KmppEnvNodeImpl *)node;
    va_list args;

    if (!impl || !impl->seq)
        return;

    va_start(args, fmt);
    seq_vprintf((struct seq_file *)impl->seq, fmt, args);
    va_end(args);
}
EXPORT_SYMBOL(kmpp_env_log);

static int env_fops_show(struct seq_file *file, void *v)
{
    KmppEnvNodeImpl *node = (KmppEnvNodeImpl *)(file->private);
    KmppEnvInfo *info = &node->info;
    void *val = info->val;

    switch (info->type) {
    case KmppEnv_s32 : {
        seq_printf(file, "%u\n", ((rk_s32 *)val)[0]);
    } break;
    case KmppEnv_u32 : {
        seq_printf(file, "%u\n", ((rk_u32 *)val)[0]);
    } break;
    case KmppEnv_s64 : {
        seq_printf(file, "%lld\n", ((rk_s64 *)val)[0]);
    } break;
    case KmppEnv_u64 : {
        seq_printf(file, "0x%llx\n", ((rk_u64 *)val)[0]);
    } break;
    case KmppEnv_str : {
        seq_printf(file, "%s\n", (char *)val);
    } break;
    case KmppEnv_user : {
        if (info->env_show)
            info->env_show(node, info->val);
    } break;
    default : {
        seq_printf(file, "unknown type\n");
    } break;
    }

    return 0;
}

static int env_fops_open(struct inode *inode, struct file *file)
{
    KmppEnvNodeImpl *node = (KmppEnvNodeImpl *)(pde_data(inode));
    int ret;

    ret = single_open(file, env_fops_show, node);
    node->seq = file->private_data;
    return ret;
}

static ssize_t env_fops_write(struct file *file, const char __user *buf,
                              size_t count, loff_t *ppos)
{
    struct seq_file *priv = file->private_data;
    KmppEnvNodeImpl *node = (KmppEnvNodeImpl *)(priv->private);
    char *val = (char *)node->info.val;
    int ret;

    if (priv != node->seq) {
        kmpp_loge_f("seq file mismatch %px - %px\n", priv, node->seq);
        return -EINVAL;
    }

    if (node->info.readonly)
        return -EPERM;

    switch (node->info.type) {
    case KmppEnv_s32 : {
        ret = kstrtos32_from_user(buf, count, 0, (rk_s32 *)val);
    } break;
    case KmppEnv_u32 : {
        ret = kstrtou32_from_user(buf, count, 0, (rk_u32 *)val);
    } break;
    case KmppEnv_s64 : {
        ret = kstrtos64_from_user(buf, count, 0, (rk_s64 *)val);
    } break;
    case KmppEnv_u64 : {
        ret = kstrtou64_from_user(buf, count, 0, (rk_u64 *)val);
    } break;
    case KmppEnv_str : {
        if (count > node->str_val_size) {
            node->str_val_size = KMPP_ALIGN(count, sizeof(rk_u64));
            val = kmpp_calloc(node->str_val_size);
            if (!val) {
                kmpp_loge_f("kmpp_calloc %d failed\n", node->str_val_size);
                return -ENOMEM;
            }
            kmpp_free(node->info.val);
            node->info.val = val;
        }
        ret = copy_from_user(val, buf, count);
        if (val[count - 1] != '\0')
            val[count - 1] = '\0';
    } break;
    default : {
        ret = -EINVAL;
    } break;
    }

    if (ret)
            return ret;

    return count;
}

static const struct proc_ops kmpp_env_fops = {
        .proc_open = env_fops_open,
        .proc_read = seq_read,
        .proc_release = single_release,
        .proc_write = env_fops_write,
};

void osal_env_init(void)
{
    OSAL_INIT_LIST_HEAD(&list_env_grp);
    OSAL_INIT_LIST_HEAD(&list_env_node);
    osal_spinlock_init(&lock_env);

    kmpp_env_get(&kmpp_env_osal, NULL, "kmpp_osal");
    kmpp_env_get(&kmpp_env_debug, NULL, "kmpp_debug");
}

void osal_env_deinit(void)
{
    KmppEnvGrpImpl *grp;

    osal_spin_lock(lock_env);
    do {
        grp = osal_list_first_entry_or_null(&list_env_grp, KmppEnvGrpImpl, list_group);
        if (!grp)
            break;

        osal_spin_unlock(lock_env);

        kmpp_env_put(grp);

        osal_spin_lock(lock_env);
    } while (grp);
    osal_spin_unlock(lock_env);

    if (!osal_list_empty(&list_env_grp))
        kmpp_loge_f("list_env_grp is still not empty\n");

    if (!osal_list_empty(&list_env_node))
        kmpp_loge_f("list_env_node is still not empty\n");

    osal_spinlock_deinit(&lock_env);
}

rk_s32 kmpp_env_get(KmppEnvGrp *env, KmppEnvGrp parent, const rk_u8 *name)
{
    KmppEnvGrpImpl *grp;

    if (!env || !name) {
        kmpp_loge_f("invalid param env %px name %s\n", env, name);
        return rk_nok;
    }

    *env = NULL;
    grp = kmpp_calloc_atomic(sizeof(*grp));
    if (!grp) {
        kmpp_loge_f("failed to create KmppEnvGrp size %d\n", sizeof(*grp));
        return rk_nok;
    }

    OSAL_INIT_LIST_HEAD(&grp->list_group);
    OSAL_INIT_LIST_HEAD(&grp->list_child);
    OSAL_INIT_LIST_HEAD(&grp->list_parent);
    OSAL_INIT_LIST_HEAD(&grp->list_nodes);

    osal_strncpy(grp->name, name, sizeof(grp->name));

    if (parent) {
        KmppEnvGrpImpl *p = (KmppEnvGrpImpl *)parent;

        grp->dir = proc_mkdir(grp->name, p->dir);

        osal_spin_lock(lock_env);
        osal_list_add_tail(&grp->list_group, &list_env_grp);
        osal_list_add_tail(&grp->list_parent, &p->list_child);
        osal_spin_unlock(lock_env);
    } else {
        grp->dir = proc_mkdir(grp->name, NULL);
        grp->is_root = 1;

        osal_spin_lock(lock_env);
        osal_list_add_tail(&grp->list_group, &list_env_grp);
        osal_spin_unlock(lock_env);
    }

    *env = grp;

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_env_get);

rk_s32 kmpp_env_put(KmppEnvGrp env)
{
    KmppEnvGrpImpl *grp = (KmppEnvGrpImpl *)env;
    KmppEnvGrpImpl *n_grp;
    KmppEnvGrpImpl *grp_child;
    KmppEnvNodeImpl *node, *n_node;
    osal_list_head local_list_node;

    if (!env) {
        kmpp_loge_f("invalid param env %px\n", env);
        return rk_nok;
    }

    /* release child env group first */
    osal_spin_lock(lock_env);

    /* break link on list_group and list_parent */
    osal_list_del_init(&grp->list_group);
    osal_list_del_init(&grp->list_parent);

    osal_list_for_each_entry_safe(grp_child, n_grp, &grp->list_child, KmppEnvGrpImpl, list_parent) {
        osal_spin_unlock(lock_env);

        kmpp_env_put(grp_child);

        osal_spin_lock(lock_env);
    }
    osal_spin_unlock(lock_env);

    OSAL_INIT_LIST_HEAD(&local_list_node);

    osal_spin_lock(lock_env);
    if (!osal_list_empty(&grp->list_nodes))
        osal_list_splice_tail_init(&grp->list_nodes, &local_list_node);
    osal_spin_unlock(lock_env);

    osal_list_for_each_entry_safe(node, n_node, &local_list_node, KmppEnvNodeImpl, list) {
        kmpp_env_del(grp, node);
    }

    /* remove only root procfs directory */
    if (grp->dir && grp->is_root) {
        proc_remove(grp->dir);
        grp->dir = NULL;
    }

    /* NOTE: release original buffer */
    kmpp_free(env);

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_env_put);

rk_s32 kmpp_env_add(KmppEnvGrp env, KmppEnvNode *node, KmppEnvInfo *info)
{
    struct proc_dir_entry *pde = NULL;
    KmppEnvGrpImpl *grp = (KmppEnvGrpImpl *)env;
    KmppEnvNodeImpl *n = NULL;
    rk_s32 name_len;

    if (!env || !info) {
        kmpp_loge_f("invalid param env %px node %px info %px\n", env, node, info);
        return rk_nok;
    }

    if (node)
        *node = NULL;

    name_len = KMPP_ALIGN(osal_strlen(info->name) + 1, sizeof(rk_u64));

    n = kmpp_calloc_atomic(sizeof(*n) + name_len);
    if (!n) {
        kmpp_loge_f("failed to create KmppEnvNode size %d\n", sizeof(*n));
        goto failed;
    }

    if (info->type == KmppEnv_str) {
        rk_s32 val_size = KMPP_ALIGN(osal_strlen(info->val) + 1, sizeof(rk_u64));
        void *val_buf = kmpp_calloc_atomic(val_size);

        if (!val_buf) {
            kmpp_loge_f("failed to create val_buf size %d\n", val_size);
            goto failed;
        }

        osal_strncpy(val_buf, info->val, val_size);
        n->info.val = val_buf;
        n->str_val_size = val_size;
    } else {
        n->info.val = info->val;
        n->str_val_size = 0;
    }

    /* copy env name */
    osal_strncpy((rk_u8 *)(n + 1), info->name, name_len);

    n->grp = grp;
    n->info.type = info->type;
    n->info.readonly = info->readonly;
    n->info.name = (const rk_u8 *)(n + 1);
    n->info.env_show = info->env_show;

    pde = proc_create_data(n->info.name, info->readonly ? 0444 : 0644, grp->dir,
                           &kmpp_env_fops, n);
    if (!pde) {
        kmpp_loge_f("failed to create procfs entry %s\n", n->info.name);
        goto failed;
    }

    osal_spin_lock(lock_env);
    osal_list_add_tail(&n->list, &grp->list_nodes);
    osal_spin_unlock(lock_env);

    if (node)
        *node = n;

    return rk_ok;

failed:
    if (pde)
        proc_remove(pde);

    kmpp_free(n);
    return rk_nok;
}
EXPORT_SYMBOL(kmpp_env_add);

rk_s32 kmpp_env_del(KmppEnvGrp env, KmppEnvNode node)
{
    KmppEnvNodeImpl *p = (KmppEnvNodeImpl *)node;

    if (p) {
        if (p->grp != env) {
            kmpp_loge_f("invalid param env %px node %px node->grp\n",
                        env, p, p->grp);

            return rk_nok;
        }

        osal_spin_lock(lock_env);
        osal_list_del_init(&p->list);
        osal_spin_unlock(lock_env);

        if (p->info.type == KmppEnv_str) {
            kmpp_free(p->info.val);
        }

        kmpp_free(p);
    }

    return rk_ok;
}
EXPORT_SYMBOL(kmpp_env_del);

void *kmpp_env_to_ptr(KmppEnvNode node)
{
    if (node) {
        KmppEnvNodeImpl *impl = (KmppEnvNodeImpl *)node;

        return impl->info.val;
    }

    return NULL;
}
EXPORT_SYMBOL(kmpp_env_to_ptr);
