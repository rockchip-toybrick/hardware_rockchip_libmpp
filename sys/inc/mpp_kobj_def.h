/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __ROCKCHIP_MPP_KOBJ_DEF_H__
#define __ROCKCHIP_MPP_KOBJ_DEF_H__

#include "mpp_kobj.h"

#define _TO_STR(x)					#x
#define TO_STR(x)					_TO_STR(x)
#define TO_PTR(x)					((void *)(unsigned long)(x))
#define TO_TYPE(type, x)			((type)(unsigned long)(x))
#define CONCAT2(x, y)				x##y
#define CONCAT3(x, y, z)			x##y##z

#define KOBJ_ENUM(kobj, elem)		ENUM_##kobj##_##elem
#define IMPL_TYPE_SIZE				sizeof(IMPL_TYPE)
#define MPP_STR_KOBJ_TOTAL_SIZE		"kobj_total_size"
#define MPP_STR_KOBJ_INFO_COUNT		"kobj_info_count"
#define MPP_STR_KOBJ_ELEM_COUNT		"kobj_elem_count"
#define MPP_STR_KOBJ_BATCH_COUNT	"kobj_batch_count"

enum MPP_KOBJ_COM_TYPE {
	MPP_KOBJ_TOTAL_SIZE,
	MPP_KOBJ_INFO_COUNT,
	MPP_KOBJ_ELEM_COUNT,
	MPP_KOBJ_BATCH_COUNT,
	MPP_KOBJ_PROP_LAST,
	MPP_KOBJ_ELEM_BASE = MPP_KOBJ_PROP_LAST,
};

#define __POW2_FLOOR(x)	((x) & BIT(31) ? BIT(31) : \
						((x) & BIT(30) ? BIT(30) : \
						((x) & BIT(29) ? BIT(29) : \
						((x) & BIT(27) ? BIT(27) : \
						((x) & BIT(26) ? BIT(26) : \
						((x) & BIT(25) ? BIT(25) : \
						((x) & BIT(24) ? BIT(24) : \
						((x) & BIT(23) ? BIT(23) : \
						((x) & BIT(22) ? BIT(22) : \
						((x) & BIT(21) ? BIT(21) : \
						((x) & BIT(20) ? BIT(20) : \
						((x) & BIT(19) ? BIT(19) : \
						((x) & BIT(17) ? BIT(17) : \
						((x) & BIT(16) ? BIT(16) : \
						((x) & BIT(15) ? BIT(15) : \
						((x) & BIT(14) ? BIT(14) : \
						((x) & BIT(13) ? BIT(13) : \
						((x) & BIT(12) ? BIT(12) : \
						((x) & BIT(11) ? BIT(11) : \
						((x) & BIT(10) ? BIT(10) : \
						((x) & BIT(9) ? BIT(9) : \
						((x) & BIT(7) ? BIT(7) : \
						((x) & BIT(6) ? BIT(6) : \
						((x) & BIT(5) ? BIT(5) : \
						((x) & BIT(4) ? BIT(4) : \
						((x) & BIT(3) ? BIT(3) : \
						((x) & BIT(2) ? BIT(2) : \
						((x) & BIT(1) ? BIT(1) : \
						(1)))))))))))))))))))))))))))))

#define __COUNT_IN_PAGE(x)		(PAGE_SIZE/(x))
#define __KOBJ_SIZE_TO_BATCH(x)	(__POW2_FLOOR(__COUNT_IN_PAGE(x)))
#define __KOBJ_HEAD_SIZE		(sizeof(struct mpp_ioc_kobj_head))
#define KOBJ_SIZE_TO_BATCH(x)	(((x) == __KOBJ_HEAD_SIZE) ? 1 : (__KOBJ_SIZE_TO_BATCH(x)))

#define KOBJ_PROPS(kobj)		__##kobj##_props
#define KOBJ_ELEM_LAST(kobj)		ENUM_##kobj##_ELEM_LAST
#define KOBJ_ELEM_NUM(kobj)		(KOBJ_ELEM_LAST(kobj) - MPP_KOBJ_ELEM_BASE)
#define KOBJ_BATCH_NUM			(KOBJ_SIZE_TO_BATCH(sizeof(IMPL_TYPE)))
#define KOBJ_QUERY(kobj)		__##kobj##_query
#define KOBJ_REGISTER(kobj)		__##kobj##_register

#define EXPAND_AS_FUNC_DECLARE(obj, elem_name, elem_type, kobj_type) \
	elem_type CONCAT3(obj, _get_, elem_name)(const INTF_TYPE s); \
	void CONCAT3(obj, _set_, elem_name)(INTF_TYPE s, elem_type val);

#define EXPAND_AS_PROPS_VAL(kobj, elem_name, elem_type, kobj_type) \
	{ \
		.data_size		= sizeof(((IMPL_TYPE *)0)->elem_name), \
		.data_offset	= offsetof(IMPL_TYPE, elem_name), \
		.data_enum		= KOBJ_ENUM(kobj, elem_name), \
		.data_type		= kobj_type, \
		.reserve		= 0, \
	},

#define EXPAND_AS_ACCESSORS(kobj, elem_name, elem_type, kobj_type) \
	elem_type CONCAT3(kobj, _get_, elem_name)(INTF_TYPE s) \
	{ \
		switch (kobj_type) { \
		case MPP_KOBJ_TYPE_FIX: \
		case MPP_KOBJ_TYPE_VAR: { \
			return TO_TYPE(elem_type, ((IMPL_TYPE *)s)->elem_name); \
		} break; \
		case MPP_KOBJ_TYPE_STRUCT: \
		case MPP_KOBJ_TYPE_BUFFER: { \
			return TO_TYPE(elem_type, &(((IMPL_TYPE *)s)->elem_name)); \
		} break; \
		} \
		return (elem_type)0; \
	} \
	void CONCAT3(kobj, _set_, elem_name)(INTF_TYPE s, elem_type v) \
	{ \
		switch (kobj_type) { \
		case MPP_KOBJ_TYPE_FIX: \
		case MPP_KOBJ_TYPE_VAR: \
		case MPP_KOBJ_TYPE_STRUCT: \
		case MPP_KOBJ_TYPE_BUFFER: { \
			memcpy(TO_PTR(&(((IMPL_TYPE *)s)->elem_name)), \
			       TO_PTR(v), sizeof(((IMPL_TYPE *)s)->elem_name)); \
		} break; \
		} \
	}

#define EXPAND_AS_ENUM(kobj, elem_name, elem_type, kobj_type) \
	KOBJ_ENUM(kobj, elem_name),

#define EXPAND_AS_QUERY_INFO(kobj, elem_name, elem_type, kobj_type) \
	{ \
		.name = #elem_name, \
		.prop = &KOBJ_PROPS(kobj)[KOBJ_ENUM(kobj, elem_name)], \
	},

#define MPP_KOBJ_REGISTER(kobj, ENTRY_TABLE) \
	enum ENUM_##kobj##_ELEM_e { \
		ENUM_##kobj##_TOTAL_SIZE = MPP_KOBJ_TOTAL_SIZE, \
		ENUM_##kobj##_INFO_COUNT = MPP_KOBJ_INFO_COUNT, \
		ENUM_##kobj##_ELEM_COUNT = MPP_KOBJ_ELEM_COUNT, \
		ENUM_##kobj##_BATCH_COUNT = MPP_KOBJ_BATCH_COUNT, \
		ENTRY_TABLE(kobj, EXPAND_AS_ENUM) \
		KOBJ_ELEM_LAST(kobj) \
	}; \
	static union mpp_kobj_prop KOBJ_PROPS(kobj)[] = { \
		{	.val = sizeof(IMPL_TYPE), }, \
		{	.val = MPP_KOBJ_PROP_LAST, }, \
		{	.val = KOBJ_ELEM_NUM(kobj), }, \
		{	.val = KOBJ_BATCH_NUM, }, \
		ENTRY_TABLE(kobj, EXPAND_AS_PROPS_VAL) \
	}; \
	ENTRY_TABLE(obj, EXPAND_AS_ACCESSORS) \
	void kobj##_kobj_register(void) \
	{ \
		static struct mpp_kobj_elem_info KOBJ_REGISTER(kobj) = { \
			.kobj_name = TO_STR(kobj), \
			.total_size = sizeof(IMPL_TYPE), \
			.elem_count = KOBJ_ELEM_NUM(kobj), \
			.batch_count = KOBJ_ELEM_NUM(kobj), \
			.dump = kobj##_kobj_dump, \
			.elems = { \
				{ \
					.name = MPP_STR_KOBJ_TOTAL_SIZE, \
					.prop = &KOBJ_PROPS(kobj)[MPP_KOBJ_TOTAL_SIZE], \
				}, \
				{ \
					.name = MPP_STR_KOBJ_INFO_COUNT, \
					.prop = &KOBJ_PROPS(kobj)[MPP_KOBJ_INFO_COUNT], \
				}, \
				{ \
					.name = MPP_STR_KOBJ_ELEM_COUNT, \
					.prop = &KOBJ_PROPS(kobj)[MPP_KOBJ_ELEM_COUNT], \
				}, \
				{ \
					.name = MPP_STR_KOBJ_BATCH_COUNT, \
					.prop = &KOBJ_PROPS(kobj)[MPP_KOBJ_BATCH_COUNT], \
				}, \
				ENTRY_TABLE(kobj, EXPAND_AS_QUERY_INFO) \
			}, \
		}; \
		mpp_kobj_info_register(&KOBJ_REGISTER(kobj)); \
	}

#define TO_TOTAL_SIZE(p)	((u32)((p)->elems[MPP_KOBJ_TOTAL_SIZE].prop->val))
#define TO_ELEM_COUNT(p)	((u32)((p)->elems[MPP_KOBJ_ELEM_COUNT].prop->val))
#define TO_BATCH_COUNT(p)	((u32)((p)->elems[MPP_KOBJ_BATCH_COUNT].prop->val))

#define ELEM_PRINT(p, field, type)	pr_info("%-32s : " type "\n", TO_STR(field), ((p)->field))

#define ELEM_PRINT_S32(p, field)	ELEM_PRINT(p, field, "%d")
#define ELEM_PRINT_U32(p, field)	ELEM_PRINT(p, field, "%u")
#define ELEM_PRINT_S64(p, field)	ELEM_PRINT(p, field, "%lld")
#define ELEM_PRINT_U64(p, field)	ELEM_PRINT(p, field, "%llu")
#define ELEM_PRINT_PTR(p, field)	ELEM_PRINT(p, field, "%px")

#define ELEM_PRINT_ARRAY(p, field, idx, type) \
	pr_info(TO_STR(field) "[%d] : " type "\n", idx, ((p)->field[idx]))

#define ELEM_PRINT_S32_ARRAY(p, field)	\
	do { \
		u32 i; \
		for (i = 0; i < ARRAY_SIZE((p)->field); i++) \
			ELEM_PRINT_ARRAY(p, field, i, "%d"); \
	} while (0)
#define ELEM_PRINT_U32_ARRAY(p, field)	\
	do { \
		u32 i; \
		for (i = 0; i < ARRAY_SIZE((p)->field); i++) \
			ELEM_PRINT_ARRAY(p, field, i, "%u"); \
	} while (0)

#define ELEM_PRINT_ARRAY2(p, field, row, col, type) \
	pr_info(TO_STR(field) "[%d][%d] : " type "\n", row, col, ((p)->field[row][col]))

#define ELEM_PRINT_S32_ARRAY2(p, field)	\
	do { \
		u32 i, j; \
		for (i = 0; i < ARRAY_SIZE((p)->field); i++) \
			for (j = 0; j < ARRAY_SIZE((p)->field[0]); j++) \
				ELEM_PRINT_ARRAY2(p, field, i, j, "%d"); \
	} while (0)
#define ELEM_PRINT_U32_ARRAY2(p, field)	\
	do { \
		u32 i, j; \
		for (i = 0; i < ARRAY_SIZE((p)->field); i++) \
			for (j = 0; j < ARRAY_SIZE((p)->field[0]); j++) \
				ELEM_PRINT_ARRAY2(p, field, i, j, "%u"); \
	} while (0)

#define ELEM_PRINT_ARRAY3(p, field, index, row, col, type) \
	pr_info(TO_STR(field) "[%d][%d][%d] : " type "\n", index, row, col, \
		       ((p)->field[index][row][col]))

#define ELEM_PRINT_S32_ARRAY3(p, field)	\
	do { \
		u32 i, j, k; \
		for (i = 0; i < ARRAY_SIZE((p)->field); i++) \
			for (j = 0; j < ARRAY_SIZE((p)->field[0]); j++) \
				for (k = 0; k < ARRAY_SIZE((p)->field[0][0]); k++) \
					ELEM_PRINT_ARRAY3(p, field, i, j, k, "%d"); \
	} while (0)

#define ELEM_PRINT_U32_ARRAY3(p, field)	\
	do { \
		u32 i, j, k; \
		for (i = 0; i < ARRAY_SIZE((p)->field); i++) \
			for (j = 0; j < ARRAY_SIZE((p)->field[0]); j++) \
				for (k = 0; k < ARRAY_SIZE((p)->field[0][0]); k++) \
					ELEM_PRINT_ARRAY3(p, field, i, j, k, "%u"); \
	} while (0)

#endif /* __ROCKCHIP_MPP_KOBJ_DEF_H__ */

