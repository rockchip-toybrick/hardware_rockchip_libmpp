/* SPDX-License-Identifier: ((GPL-2.0+ WITH Linux-syscall-note) OR MIT) */
/*
 * Rockchip mpp system driver ioctl object
 * Copyright (C) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef _UAPI_RK_MPP_SYS_H
#define _UAPI_RK_MPP_SYS_H

#include <linux/types.h>

#define PENDING_NODE_ID_MIN		0
#define PENDING_NODE_ID_MAX		(BITS_PER_LONG)
#define PGOFF_PENDING_NODE_BASE		(64ul)
#define PGOFF_PENDING_NODE_END		(PGOFF_PENDING_NODE_BASE + PENDING_NODE_ID_MAX)

#define PENDING_NODE_START		((PGOFF_PENDING_NODE_BASE) << PAGE_SHIFT)
#define TO_PENDING_NODE_IDX(x)		((PENDING_NODE_START) + ((x) << PAGE_SHIFT))
#define TO_VM_MMAP_ID(x)		((x - (PENDING_NODE_START)) >> PAGE_SHIFT)
#define PENDING_NODE_END		TO_PENDING_NODE_IDX(BITS_PER_LONG)

#define MPP_KOBJ_NAME_LEN		16
#define MPP_KOBJ_ELEM_NAME_LEN		32

enum mpp_kobj_type {
	MPP_KOBJ_TYPE_NONE,
	/* fix length:  u32/s32, u64/s64 */
	MPP_KOBJ_TYPE_FIX,
	/* variable length: pointer/long/size_t, store as u64 */
	MPP_KOBJ_TYPE_VAR,
	/* structure larger than 64bit */
	MPP_KOBJ_TYPE_STRUCT,
	/* string buffer address in the struct */
	MPP_KOBJ_TYPE_BUFFER,
	/* max value of 4 bit */
	MPP_KOBJ_TYPE_END		= 0xf,
};

/*
 * flag bit description for flag in struct mpp_ioc_kobj_head
 * bit 00~03: 0 - kernel object header pool buffer for all headers;
 *            1 - kernel object buffer;
 *            2 - kernel object read-only info buffer;
 *            3 - normal share buffer;
 *            4 - dma-buf buffer;
 * bit 04~07: dma buffer flag, valid on requesting dma-buf;
 * bit 04   : 0 - uncached dma-buf;
 *            1 - cacheable dma-buf;
 * bit 05   : 0 - dma-buf with sg-table;
 *            1 - contiguous physical memory;
 * bit 06   : 0 - dma-buf with 64bit physical address;
 *            1 - dma-buf with 32bit physical address;
 * bit 07   : 0 - normal dma-buf;
 *            1 - secure dma-buf which can not be accessed by cpu;
 *
 * typical work flow:
 * step 1: request kobject header pool buffer for one session.
 * step 2: request kobject read-only info buffer to setup kobject access
 *         function property.
 * step 3: release kobject read-only info buffer to save memory.
 * step 4: create kobject buffer, normal share memory and dma-buf.
 * step 5: release all buffers after work flow.
 * step 6: release kobject header pool buffer and end communication.
 */
#define MPP_KOBJ_FLAG_KOBJ_HEAD_POOL	(0)
#define MPP_KOBJ_FLAG_KOBJ_PROP		(1)
#define MPP_KOBJ_FLAG_KOBJ		(2)
#define MPP_KOBJ_FLAG_NORMAL		(3)
#define MPP_KOBJ_FLAG_DMABUF		(4)

#define MPP_KOBJ_FLAG_CACHABLE		BIT(4)
#define MPP_KOBJ_FLAG_CONTIG		BIT(5)
#define MPP_KOBJ_FLAG_DMA32		BIT(6)
#define MPP_KOBJ_FLAG_SECURE		BIT(7)

union mpp_kobj_prop {
	__u64				val;
	struct {
		__u32			data_size   : 16;
		__u32			data_offset : 16;
		__u32			data_enum   : 12;
		enum mpp_kobj_type	data_type   : 4;
		__u32			reserve     : 16;
	};
};

struct mpp_ioc_kobj_info {
	/* offset and length of name string behind elems */
	__u32				offset;
	__u32				length;
	/* kobject element property info */
	union mpp_kobj_prop		prop;
};

/*
 * Kernel object header structure
 * The kernel object header structure is used for serval stage:
 * 1. Userspace requests share buffer with kernel.
 *    When requesting normal share memory buffer userspace fills the kobj_name,
 *    flag and size. Then kernel fills the kobj_id, uaddr and kaddr.
 *    When requesting kobject with access function userspace fills the
 *    kobj_name first then get the access function info buffer (read-only).
 *    Then build the userspace access property and request the buffer entry for
 *    userspace and kernel communication.
 * 2. userspace requests readonly kobject access info property.
 * 3. kernel will record and management these header structure for check.
 * 4. when kernel return kobject to userspace this header will be wrote at the
 *    beginning.
 * 5. when userspace send kobject to kernel the kernel will check the buffer
 *    header with the info in registered records.
 */
struct mpp_ioc_kobj_head {
	/* identifier for the kernel object */
	char				kobj_name[MPP_KOBJ_NAME_LEN];
	/* MPP_KOBJ_FLAG definition by bit */
	__u32				flag;
	/* kobj total size with both head and body must be 32 byte aligned */
	__u32				size;
	/*
	 * kobject global index is composed of four parts:
	 * name_id  - kernel object name index in mpp_kobj_info_root pool.
	 * batch_lsh - kernel userspace kobject count shifter in a group set.
	 *            there is 1 << batch_lsh element in each group set.
	 * user_gid - userspace kobject manager group id for each batch kobj_get.
	 * kern_sid - kernel session id.
	 * kern_uid - kernel unify id, bit 8~15 for batch id 0~7 for entry id.
	 */
	union {
		__u64			kobj_id;
		struct {
			__u16		name_id		: 12;
			__u16		batch_lsh	: 4;
			__u16		user_uid;
			__u16		kern_sid;
			__u16		kern_uid;
		};
	};
	/* kobject userspace address for kobject body */
	__u64				uaddr;
	/* kobject kernel address for kobject header */
	__u64				kaddr;
	/* the rest body elements */
};

/* kernel object share info */
typedef struct kmpp_obj_sinfo_t {
	/* GET/PUT SHM info */
	__u32 				kobj_type_id	: 16;
	__u32  				shdr_size		: 8; 	/* share header size */
	__u32  				batch_count		: 8; 	/* share object batch count for each get / put */
	__u32  				kobj_size;
	/* trie info */
	__u64 				trie_root;				/* share object trie root */
} kmpp_obj_sinfo;

/* kernel object share header */
typedef struct kmpp_obj_shdr_t {
	__u64 				shdr_uaddr;				/* share header userspace address for check */
	__u64 				shdr_kaddr;				/* share header kernel address for check */
	__u64 				kobj_uaddr;
	__u64 				kobj_kaddr;
} kmpp_obj_shdr;

typedef struct kmpp_obj_ref_t {
	__u64 				kobj_uaddr;
	__u64 				kobj_kaddr;
	/* DO NOT access reserved data only used by kernel */
} kmpp_obj_ref;

struct mpp_ioc_kobj_query {
	char				kobj_name[MPP_KOBJ_NAME_LEN];
	__u32				prop_count;
	__u32				batch_count;
	__u32				reserve[2];
	struct mpp_ioc_kobj_info	elems[];
};

#endif /* _UAPI_RK_MPP_SYS_H */

