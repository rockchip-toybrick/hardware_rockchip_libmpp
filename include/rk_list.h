/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __RK_LIST_H__
#define __RK_LIST_H__

#include "rk_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef struct osal_list_head_t osal_list_head;

struct osal_list_head_t {
    osal_list_head *next, *prev;
};

#define OSAL_LIST_HEAD_INIT(name) { &(name), &(name) }

#define OSAL_LIST_HEAD(name) \
    osal_list_head name = OSAL_LIST_HEAD_INIT(name)

#define OSAL_INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
    } while (0)

#define osal_list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define osal_list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define osal_list_first_entry(ptr, type, member) \
        osal_list_entry((ptr)->next, type, member)

#define osal_list_last_entry(ptr, type, member) \
        osal_list_entry((ptr)->prev, type, member)

#define osal_list_first_entry_or_null(ptr, type, member) ({ \
        osal_list_head *head__ = (ptr); \
        osal_list_head *pos__ = head__->next; \
        pos__ != head__ ? osal_list_entry(pos__, type, member) : NULL; \
})

#define osal_list_next_entry(pos, type, member) \
        osal_list_entry((pos)->member.next, type, member)

#define osal_list_prev_entry(pos, type, member) \
        osal_list_entry((pos)->member.prev, type, member)

#define osal_list_for_each_entry(pos, head, type, member) \
    for (pos = osal_list_entry((head)->next, type, member); \
         &pos->member != (head); \
         pos = osal_list_next_entry(pos, type, member))

#define osal_list_for_each_entry_safe(pos, n, head, type, member) \
    for (pos = osal_list_first_entry(head, type, member),  \
         n = osal_list_next_entry(pos, type, member); \
         &pos->member != (head);                    \
         pos = n, n = osal_list_next_entry(n, type, member))

#define osal_list_for_each_entry_reverse(pos, head, type, member) \
    for (pos = osal_list_last_entry(head, type, member); \
         &pos->member != (head); \
         pos = osal_list_prev_entry(pos, type, member))

#define osal_list_for_each_entry_safe_reverse(pos, n, head, type, member) \
    for (pos = osal_list_last_entry(head, type, member),  \
         n = osal_list_prev_entry(pos, type, member); \
         &pos->member != (head);                    \
         pos = n, n = osal_list_prev_entry(n, type, member))

static __inline void __osal_list_add(osal_list_head * _new,
                                     osal_list_head * prev,
                                     osal_list_head * next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static __inline void osal_list_add(osal_list_head *_new, osal_list_head *head)
{
    __osal_list_add(_new, head, head->next);
}

static __inline void osal_list_add_tail(osal_list_head *_new, osal_list_head *head)
{
    __osal_list_add(_new, head->prev, head);
}

static __inline void __osal_list_del(osal_list_head * prev, osal_list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void osal_list_replace(osal_list_head *old, osal_list_head *_new)
{
    _new->next = old->next;
    _new->next->prev = _new;
    _new->prev = old->prev;
    _new->prev->next = _new;
}

static inline void osal_list_replace_init(osal_list_head *old, osal_list_head *_new)
{
    osal_list_replace(old, _new);
    OSAL_INIT_LIST_HEAD(old);
}

static inline void osal_list_swap(osal_list_head *entry1, osal_list_head *entry2)
{
    osal_list_head *pos = entry2->prev;

    __osal_list_del(entry2->prev, entry2->next);
    osal_list_replace(entry1, entry2);
    if (pos == entry1)
        pos = entry2;
    osal_list_add(entry1, pos);
}

static __inline void osal_list_del_init(osal_list_head *entry)
{
    __osal_list_del(entry->prev, entry->next);

    OSAL_INIT_LIST_HEAD(entry);
}

static __inline void osal_list_move(osal_list_head *list, osal_list_head *head)
{
    __osal_list_del(list->prev, list->next);
    osal_list_add(list, head);
}

static __inline void osal_list_move_tail(osal_list_head *list, osal_list_head *head)
{
    __osal_list_del(list->prev, list->next);
    osal_list_add_tail(list, head);
}

static inline int osal_list_is_first(const osal_list_head *list, const osal_list_head *head)
{
    return list->prev == head;
}

static inline int osal_list_is_last(const osal_list_head *list, const osal_list_head *head)
{
    return list->next == head;
}

static inline int osal_list_empty(const osal_list_head *head)
{
    return head->next == head;
}

static inline void __osal_list_splice(const osal_list_head *list,
                                 osal_list_head *prev,
                                 osal_list_head *next)
{
        osal_list_head *first = list->next;
        osal_list_head *last = list->prev;

        first->prev = prev;
        prev->next = first;

        last->next = next;
        next->prev = last;
}

static inline void osal_list_splice(const osal_list_head *list, osal_list_head *head)
{
    if (!osal_list_empty(list))
        __osal_list_splice(list, head, head->next);
}

static inline void osal_list_splice_tail(osal_list_head *list, osal_list_head *head)
{
    if (!osal_list_empty(list))
        __osal_list_splice(list, head->prev, head);
}

static inline void osal_list_splice_init(osal_list_head *list, osal_list_head *head)
{
    if (!osal_list_empty(list)) {
        __osal_list_splice(list, head, head->next);
        OSAL_INIT_LIST_HEAD(list);
    }
}

static inline void osal_list_splice_tail_init(osal_list_head *list,
                                         osal_list_head *head)
{
    if (!osal_list_empty(list)) {
        __osal_list_splice(list, head->prev, head);
        OSAL_INIT_LIST_HEAD(list);
    }
}

typedef RK_S32 (*osal_list_cmp_func_t)(void *, const osal_list_head *, const osal_list_head *);

void osal_list_sort(void *priv, osal_list_head *head, osal_list_cmp_func_t cmp);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __RK_LIST_H__ */
