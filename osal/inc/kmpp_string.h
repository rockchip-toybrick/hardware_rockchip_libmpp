/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_STRING_H__
#define __KMPP_STRING_H__

#include "rk_type.h"

rk_u8 *osal_strcpy(rk_u8 *s1, const rk_u8 *s2);
rk_u8 *osal_strncpy(rk_u8 *s1, const rk_u8 *s2, rk_s32 size);
rk_s32 osal_strlcpy(rk_u8 *s1, const rk_u8 *s2, rk_s32 size);
rk_u8 *osal_strcat(rk_u8 *s1, const rk_u8 *s2);
rk_u8 *osal_strncat(rk_u8 *s1, const rk_u8 *s2, rk_s32 size);
rk_s32 osal_strlcat(rk_u8 *s1, const rk_u8 *s2, rk_s32 size);
rk_s32 osal_strcmp(const rk_u8 *s1, const rk_u8 *s2);
rk_s32 osal_strncmp(const rk_u8 *s1, const rk_u8 *s2, rk_s32 size);
rk_s32 osal_strnicmp(const rk_u8 *s1, const rk_u8 *s2, rk_s32 size);
rk_s32 osal_strcasecmp(const rk_u8 *s1, const rk_u8 *s2);
rk_s32 osal_strncasecmp(const rk_u8 *s1, const rk_u8 *s2, rk_s32 n);
rk_u8 *osal_strchr(const rk_u8 *s, rk_s32 n);
rk_u8 *osal_strnchr(const rk_u8 *s, rk_s32 count, rk_s32 c);
rk_u8 *osal_strrchr(const rk_u8 *s, rk_s32 c);
rk_u8 *osal_strstr(const rk_u8 *s1, const rk_u8 *s2);
rk_u8 *osal_strnstr(const rk_u8 *s1, const rk_u8 *s2, rk_s32 n);
rk_s32 osal_strlen(const rk_u8 *s);
rk_s32 osal_strnlen(const rk_u8 *s, rk_s32 size);
rk_u8 *osal_strpbrk(const rk_u8 *s1, const rk_u8 *s2);
rk_u8 *osal_strsep(rk_u8 **s, const rk_u8 *ct);
rk_s32 osal_strspn(const rk_u8 *s, const rk_u8 *accept);
rk_s32 osal_strcspn(const rk_u8 *s, const rk_u8 *reject);
rk_void *osal_memset(rk_void *str, rk_s32 c, rk_s32 count);
rk_void *osal_memcpy(rk_void *s1, const rk_void *s2, rk_s32 count);
rk_void *osal_memmove(rk_void *s1, const rk_void *s2, rk_s32 count);
rk_void *osal_memscan(rk_void *addr, rk_s32 c, rk_s32 size);
rk_s32 osal_memcmp(const rk_void *cs, const rk_void *ct, rk_s32 count);
rk_void *osal_memchr(const rk_void *s, rk_s32 c, rk_s32 n);
rk_void *osal_memchr_inv(const rk_void *s, rk_s32 c, rk_s32 n);

rk_u64 osal_strtoull(const rk_u8 *cp, rk_u8 **endp, rk_u32 base);
rk_ulong osal_strtoul(const rk_u8 *cp, rk_u8 **endp, rk_u32 base);
rk_long osal_strtol(const rk_u8 *cp, rk_u8 **endp, rk_u32 base);
rk_s64 osal_strtoll(const rk_u8 *cp, rk_u8 **endp, rk_u32 base);
rk_s32 osal_snprintf(rk_u8 *buf, rk_s32 size, const rk_u8 *fmt, ...) __attribute__((format(printf, 3, 4)));
rk_s32 osal_scnprintf(rk_u8 *buf, rk_s32 size, const rk_u8 *fmt, ...) __attribute__((format(printf, 3, 4)));
rk_s32 osal_sprintf(rk_u8 *buf, const rk_u8 *fmt, ...) __attribute__((format(printf, 2, 3)));
rk_s32 osal_sscanf(const rk_u8 *buf, const rk_u8 *fmt, ...);

#endif /* __KMPP_STRING_H__ */