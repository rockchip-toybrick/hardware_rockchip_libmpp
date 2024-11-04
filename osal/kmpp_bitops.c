/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <linux/bitmap.h>
#include <linux/kernel.h>

#include "kmpp_bitops.h"

void osal_set_bit(rk_s32 nr, rk_ul *addr)
{
	set_bit(nr, (unsigned long *)addr);
}
EXPORT_SYMBOL(osal_set_bit);

void osal_clear_bit(rk_s32 nr, rk_ul *addr)
{
	clear_bit(nr, (unsigned long *)addr);
}
EXPORT_SYMBOL(osal_clear_bit);

rk_s32 osal_test_bit(rk_s32 nr, rk_ul *addr)
{
	return test_bit(nr, (unsigned long *)addr);
}
EXPORT_SYMBOL(osal_test_bit);

rk_s32 osal_test_and_set_bit(rk_s32 nr, rk_ul *addr)
{
	return test_and_set_bit(nr, (unsigned long *)addr);
}
EXPORT_SYMBOL(osal_test_and_set_bit);

rk_s32 osal_test_and_clear_bit(rk_s32 nr, rk_ul *addr)
{
	return test_and_clear_bit(nr, (unsigned long *)addr);
}
EXPORT_SYMBOL(osal_test_and_clear_bit);

rk_s32 osal_test_and_change_bit(rk_s32 nr, rk_ul *addr)
{
	return test_and_change_bit(nr, (unsigned long *)addr);
}
EXPORT_SYMBOL(osal_test_and_change_bit);

rk_s32 osal_find_first_bit(rk_ul *addr, rk_s32 size)
{
	return find_first_bit((unsigned long *)addr, size);
}
EXPORT_SYMBOL(osal_find_first_bit);

rk_s32 osal_find_next_bit(rk_ul *addr, rk_s32 size, rk_s32 start)
{
	return find_next_bit((unsigned long *)addr, size, start);
}
EXPORT_SYMBOL(osal_find_next_bit);

rk_s32 osal_find_last_bit(rk_ul *addr, rk_s32 size)
{
	return find_last_bit((unsigned long *)addr, size);
}
EXPORT_SYMBOL(osal_find_last_bit);

rk_s32 osal_find_first_zero_bit(rk_ul *addr, rk_s32 size)
{
	return find_first_zero_bit((unsigned long *)addr, size);
}
EXPORT_SYMBOL(osal_find_first_zero_bit);

rk_s32 osal_find_next_zero_bit(rk_ul *addr, rk_s32 size, rk_s32 start)
{
	return find_next_zero_bit((unsigned long *)addr, size, start);
}
EXPORT_SYMBOL(osal_find_next_zero_bit);
