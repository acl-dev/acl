#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <string.h>
#include <stdlib.h>
#ifdef	ACL_WINDOWS
#include <search.h>   /* just for qsort */
#endif
#include <stdio.h>
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_meter_time.h"

#endif

#include "ring.h"

typedef struct MBUF_SLOTS {
	void **slots;		/* in which free slice can use */
	int   islots;		/* current position of free slots slice */
	int   nslots;		/* total slice count free in slots */
} MBUF_SLOTS;

struct ACL_SLICE {
	char  name[64];		/* the app's name */
	int   page_nslots;	/* count slice of each page */
	int   page_size;	/* length of MBUF's buf */
	int   gap_size;		/* gap_size = page_size - page_nslots * slice_size */
	int   slice_length;	/* length of each slice from app */
	int   slice_size;	/* the real size of each slice */
	int   nbuf;		/* count of MBUF allocated */
	acl_uint64 length;	/* total size of all MBUF's buf */
	acl_uint64 used_length;	/* total size of used */
	int   nalloc;		/* statistics: number of calling malloc */
	int   nfree;		/* statistics: number of calling free */
	unsigned int flag;	/* as: ACL_SLICE_FLAG_XXX */

	ACL_SLICE *(*slice_create)(int page_size, int slice_length);
	void (*slice_destroy)(ACL_SLICE *slice);

	void *(*slice_alloc)(ACL_SLICE *slice);
	void  (*slice_free)(ACL_SLICE *slice, void *ptr);
	int   (*slice_gc)(ACL_SLICE *slice);  /* 返回 1 表示已经释放完毕 */
	int   (*slice_used)(ACL_SLICE *slice);
	void  (*slice_stat)(ACL_SLICE *slice, ACL_SLICE_STAT *sbuf);
};

/**
 * MBUF1: 比较节省内存模式，但该方式在垃圾自动回收时比较费时，
 * 比较适合于分配但不回收的情形
 */

typedef struct MBUF1 {
	RING  entry;		/* linked by SLICE1->mbuf_head */
	void *buf;		/* memory buf can be sliced */
} MBUF1;

typedef struct SLICE1 {
	ACL_SLICE slice;	/* the bases class */
	MBUF_SLOTS mslots;	/* slice slots holding all slice of the mbuf */
	RING  mbuf_head;	/* link all mbuf's entry */
} SLICE1;

/* MBUF2, MBUF3 的基础类型 */

#define SIGNATURE		0xdead
#define	SLICE_OFF_SIZE		4

typedef struct MBUF {
	ACL_SLICE *slice;
	int   signature;        /* set when block is active */
	int   nused;		/* number of using memory slice in payload */
} MBUF;

/**
 * MBUF2: 内存稍微大点，但实时垃圾的自动回收性能比较好, 但如果某
 * 个 MBUF2 只要有一个结点被引用, 则该内存便不能被释放给系统
 */

#define	SLICE2_HEAD_SIZE	8

typedef struct MBUF2 {
	MBUF  mbuf;
	RING  entry;		/* be linked by SLICE2->mbuf_head */

	/* in x64 CPU, sizeof before is 32 bytes */

	char  payload[1];	/* format: {off}{pos}{data}{off}{pos}{data}... */
} MBUF2;

typedef struct SLICE2 {
	ACL_SLICE slice;	/* the base class */
	MBUF_SLOTS mslots;	/* slice slots holding all slice of the mbuf */
	RING  mbuf_head;	/* link all entry in MBUF2 */
} SLICE2;

/**
 * MBUF3: 内存更大，当顺序分配同时顺序释放时回收内存效果更好,
 * 但如果是随机释放则效率下降非常厉害，尤其当结点比较多时
 */

#define	SLICE3_HEAD_SIZE	4

typedef struct MBUF3 {
	MBUF  mbuf;
	MBUF_SLOTS mslots;	/* slice slots holding all slice of the mbuf */
	int   ibuf;		/* position in SLICE3->mbufs array */

	/* in x64 CPU, sizeof before is 36 bytes */

	char  payload[1];	/* format: {off}{data}{off}{data}... */
} MBUF3;

typedef struct SLICE3 {
	ACL_SLICE slice;	/* the base class */
	MBUF3 **mbufs;		/* all MBUF3's array */
	int   imbuf_avail;	/* current available pos of MBUF3 in mbufs */
	int   capacity;		/* the mbufs array's size */
} SLICE3;

#define	MBUF_SLOTS_SPACE(slice, mslots_in, incr, incr_real) do  \
{  \
	MBUF_SLOTS *mslots = mslots_in;  \
	if (mslots->slots == NULL) {  \
		incr_real = incr < slice->page_nslots ? slice->page_nslots : incr;  \
		mslots->slots = (void **) acl_default_malloc(__FILE__, __LINE__,  \
						sizeof(void*) * incr_real);  \
		mslots->nslots = incr_real;  \
		mslots->islots = 0;  \
	} else if (mslots->islots + incr >= mslots->nslots) {  \
		incr_real = incr < slice->page_nslots ? slice->page_nslots : incr;  \
		mslots->nslots += incr_real;  \
		mslots->slots = (void **) acl_default_realloc(__FILE__, __LINE__,  \
				mslots->slots, sizeof(void*) * mslots->nslots);  \
	}  \
} while (0)

#define	SLICE_MBUF_SPACE(slice_in, incr, incr_real, mbuf_type) do  \
{  \
	if (slice_in->mbufs == NULL) {  \
		incr_real = incr > 16 ? incr : 16;  \
		slice_in->mbufs = (mbuf_type**) acl_default_malloc(__FILE__,  \
					__LINE__, sizeof(mbuf_type*) * incr_real);  \
		slice_in->capacity = incr_real;  \
		slice_in->imbuf_avail = 0; \
	} else if (slice_in->slice.nbuf + incr >= slice_in->capacity) {  \
		incr_real = incr > 16 ? incr : 16;  \
		slice_in->capacity += incr_real;  \
		slice_in->mbufs = (mbuf_type**)  \
			acl_default_realloc(__FILE__, __LINE__, slice_in->mbufs,  \
				sizeof(mbuf_type*) * slice_in->capacity);  \
	}  \
} while (0)

/* forward declare */

#ifdef	_LP64
#include <stdint.h>	/* just for uintptr_t */
#endif

static void slice_init(ACL_SLICE *slice, unsigned int flag);

/*------------------------- just for time min3 ------------------------------*/

static void slice3_mbuf_alloc(ACL_SLICE *slice)
{
#ifdef	_LP64
	const char *myname = "slice3_mbuf_alloc";
#endif
	SLICE3 *slice3 = (SLICE3*) slice;
	MBUF3 *mbuf;
	int   i, incr_real = 0;
	char *ptr;

	mbuf = (MBUF3*) acl_default_malloc(__FILE__, __LINE__,
			slice->page_size);
	mbuf->mbuf.slice = slice;
	mbuf->mbuf.nused = 0;
	mbuf->mbuf.signature = SIGNATURE;
	ptr = mbuf->payload;

	slice->nalloc++;
	mbuf->mslots.slots = NULL;

	MBUF_SLOTS_SPACE(slice, &mbuf->mslots, slice->page_nslots, incr_real);
	acl_assert(mbuf->mslots.islots == 0);

	for (i = 0; i < slice->page_nslots; i++) {
		ptr += SLICE3_HEAD_SIZE;
#ifdef	_LP64
		if ((slice->flag & ACL_SLICE_FLAG_LP64_ALIGN)
			&& ((uintptr_t)ptr & 0x7) != 0)  /* just for AVL */
		{
			acl_msg_fatal("%s(%d): %s, ptr(%lx) invalid",
				myname, __LINE__, slice->name, (long) ptr);
		}
#endif
		*((int*) (ptr - SLICE_OFF_SIZE)) = (int) (ptr - (char*) mbuf);
		mbuf->mslots.slots[mbuf->mslots.islots++] = ptr;
		ptr += slice->slice_length;
	}

	for (i = slice->page_nslots; i < incr_real; i++)
		mbuf->mslots.slots[i] = NULL;

	SLICE_MBUF_SPACE(slice3, 1, incr_real, MBUF3);
	for (i = slice->nbuf; i < slice3->capacity; i++)
		slice3->mbufs[i] = NULL;
	slice3->mbufs[slice->nbuf] = mbuf;
	mbuf->ibuf = slice->nbuf;
	slice->nbuf++;
	slice->length += slice->page_size + sizeof(void*) * slice->page_nslots;
}

static void *slice3_alloc(ACL_SLICE *slice)
{
	SLICE3 *slice3 = (SLICE3*) slice;
	MBUF3 *mbuf = NULL;
	char *ptr;
	int   i;

	if (slice->nbuf == 0 || slice3->mbufs[slice->nbuf - 1]->mslots.islots == 0)
		slice3_mbuf_alloc(slice);

	acl_assert(slice->nbuf > 0 &&
		slice3->mbufs[slice->nbuf - 1]->mslots.islots > 0);
	acl_assert(slice3->imbuf_avail >= 0 && slice3->imbuf_avail < slice->nbuf);

	if (slice3->mbufs[slice3->imbuf_avail]->mslots.islots == 0) {
		for (i = slice3->imbuf_avail + 1; i < slice->nbuf; i++) {
			if (slice3->mbufs[i]->mslots.islots > 0) {
				mbuf = slice3->mbufs[i];
				slice3->imbuf_avail = i;
				break;
			}
		}
		acl_assert(mbuf && mbuf->mslots.islots > 0);
	} else {
		mbuf = slice3->mbufs[slice3->imbuf_avail];
		acl_assert(mbuf->mslots.islots > 0);
		for (i = slice3->imbuf_avail - 1; i >= 0; i--) {
			if (slice3->mbufs[i]->mslots.islots == 0)
				break;
			mbuf = slice3->mbufs[i];
			acl_assert(mbuf->mslots.islots > 0);
			slice3->imbuf_avail = i;
		}
		acl_assert(mbuf->mslots.islots > 0);
	}

	ptr = (char*) mbuf->mslots.slots[--mbuf->mslots.islots];
	slice->used_length += slice->slice_size;
	mbuf->mbuf.nused++;
	return ptr;
}

static void slice3_mbuf_free(ACL_SLICE *slice, MBUF3 *mbuf)
{
	SLICE3 *slice3 = (SLICE3*) slice;

	acl_assert(mbuf->ibuf + 1 == slice->nbuf);

	if (slice3->imbuf_avail == mbuf->ibuf)
		slice3->imbuf_avail--;
	if (slice3->imbuf_avail == -1)
		slice3->imbuf_avail = 0;

	acl_default_free(__FILE__, __LINE__, mbuf->mslots.slots);
	acl_default_free(__FILE__, __LINE__, mbuf);
	slice->nbuf--;
	slice->nfree++;
	slice->length -= slice->page_size + sizeof(void*) * slice->page_nslots;
}

static void slice3_free(ACL_SLICE *slice_dummy acl_unused, void *buf)
{
	const char *myname = "slice3_free";
	ACL_SLICE *slice;
	SLICE3 *slice3;
	char *ptr = (char*) buf;
	int   off;
	MBUF3 *mbuf;
	int   i;

	off = *((int*)(ptr - SLICE_OFF_SIZE));
	mbuf = (MBUF3*) ((char*) buf - off);
	if (mbuf->mbuf.signature != SIGNATURE)
		acl_msg_fatal("%s(%d): off (%u), corrupt or unallocated "
			"memory block(0x%x, 0x%x)", myname, __LINE__, off,
			mbuf->mbuf.signature, SIGNATURE);

	slice = mbuf->mbuf.slice;
	slice3 = (SLICE3*) slice;

	acl_assert(mbuf->ibuf < (int) slice->nbuf);
	acl_assert(mbuf->mslots.islots < mbuf->mslots.nslots);

	mbuf->mslots.slots[mbuf->mslots.islots++] = ptr;
	mbuf->mbuf.nused--;

	for (i = mbuf->ibuf + 1; i < slice->nbuf; i++) {
		if (slice3->mbufs[i]->mslots.islots >= mbuf->mslots.islots)
			break;
		slice3->mbufs[mbuf->ibuf] = slice3->mbufs[i];
		slice3->mbufs[i] = mbuf;
		slice3->mbufs[mbuf->ibuf]->ibuf = mbuf->ibuf;
		mbuf->ibuf = i;
	}

	if (mbuf->mbuf.nused == 0 && !(slice->flag & ACL_SLICE_FLAG_RTGC_OFF))
		slice3_mbuf_free(slice, mbuf);

	slice->used_length -= slice->slice_size;
}

static int slice3_gc(ACL_SLICE *slice)
{
	SLICE3 *slice3 = (SLICE3*) slice;
	MBUF3 *mbuf;
	int   i;

	for (i = slice->nbuf - 1; i >= 0; i--) {
		mbuf = slice3->mbufs[i];
		if (mbuf->mbuf.nused == 0)
			slice3_mbuf_free(slice, mbuf);
		else
			return 0;
	}

	return 1;
}

static int slice3_used(ACL_SLICE *slice)
{
	SLICE3 *slice3 = (SLICE3*) slice;
	MBUF3 *mbuf;
	int   i, n = 0;

	for (i = slice->nbuf - 1; i >= 0; i--) {
		mbuf = slice3->mbufs[i];
		n += mbuf->mbuf.nused;
	}
	return n;
}

static void slice3_destroy(ACL_SLICE *slice)
{
	SLICE3 *slice3 = (SLICE3*) slice;
	MBUF3 *mbuf;
	int   i;

	for (i = slice->nbuf - 1; i >= 0; i--) {
		mbuf = slice3->mbufs[i];
		slice3_mbuf_free(slice, mbuf);
	}
	acl_default_free(__FILE__, __LINE__, slice3->mbufs);
	acl_default_free(__FILE__, __LINE__, slice3);
}

static void slice3_stat(ACL_SLICE *slice,  ACL_SLICE_STAT *sbuf)
{
	SLICE3 *slice3 = (SLICE3*) slice;
	MBUF3 *mbuf;
	int   i;

	sbuf->nslots = 0;
	sbuf->islots = 0;

	for (i = 0; i < slice->nbuf; i++) {
		mbuf = slice3->mbufs[i];
		sbuf->nslots += mbuf->mslots.nslots;
		sbuf->islots += mbuf->mslots.islots;
	}

	sbuf->page_nslots = slice->page_nslots;
	sbuf->page_size = slice->page_size;
	sbuf->slice_length = slice->slice_length;
	sbuf->slice_size = slice->slice_size;
	sbuf->nbuf = slice->nbuf;
	sbuf->length = slice->length;
	sbuf->used_length = slice->used_length;
	sbuf->flag = slice->flag;
}

static ACL_SLICE *slice3_create(int page_size,
	int slice_length, unsigned int flag)
{
	SLICE3 *slice;
	int   incr_real;

	slice = (SLICE3 *) acl_default_calloc(__FILE__, __LINE__,
			1, sizeof(SLICE3));

	/* call the base ACL_SLICE's init function */

	slice_init((ACL_SLICE*) slice, flag);

	/* init the SLICE3's params */

	SLICE_MBUF_SPACE(slice, 3202, incr_real, MBUF3);

#ifdef	_LP64
	if ((flag & ACL_SLICE_FLAG_LP64_ALIGN) != 0) {
		if ((slice_length + SLICE3_HEAD_SIZE)
			% sizeof(uintptr_t) != 0)
		{
			slice_length = ((slice_length + SLICE3_HEAD_SIZE)
				/ sizeof(uintptr_t) + 1) * sizeof(uintptr_t)
				- SLICE3_HEAD_SIZE;
		}
	}
#endif

	/* reset the base ACL_SLICE's params */

	slice->slice.slice_length = slice_length;
	slice->slice.slice_size = slice_length + SLICE3_HEAD_SIZE;
	slice->slice.page_size = page_size;
	slice->slice.page_nslots = (page_size - sizeof(MBUF3))
		/slice->slice.slice_size;
	slice->slice.gap_size = page_size - slice->slice.page_nslots
		* slice->slice.slice_size;

	/* reset the base ACL_SLICE's callback */
	
	slice->slice.slice_destroy = slice3_destroy;
	slice->slice.slice_alloc = slice3_alloc;
	slice->slice.slice_free = slice3_free;
	slice->slice.slice_gc = slice3_gc;
	slice->slice.slice_used = slice3_used;
	slice->slice.slice_stat = slice3_stat;

	return (ACL_SLICE*) slice;
}

/*---------------------------- just for slice2 -------------------------------*/

static void slice2_mbuf_alloc(ACL_SLICE *slice)
{
#ifdef	_LP64
	const char *myname = "slice2_mbuf_alloc";
#endif
	SLICE2 *slice2 = (SLICE2*) slice;
	MBUF2 *mbuf;
	int   i, incr_real = 0;
	char *ptr;

	mbuf = (MBUF2*) acl_default_malloc(__FILE__, __LINE__, slice->page_size);
	mbuf->mbuf.slice = slice;
	mbuf->mbuf.nused = 0;
	mbuf->mbuf.signature = SIGNATURE;
	ring_append(&slice2->mbuf_head, &mbuf->entry);
	ptr = mbuf->payload;

	slice->nalloc++;
	MBUF_SLOTS_SPACE(slice, &slice2->mslots, slice->page_nslots, incr_real);

	for (i = 0; i < slice->page_nslots; i++) {
		ptr += SLICE2_HEAD_SIZE;
#ifdef	_LP64
		if ((slice->flag & ACL_SLICE_FLAG_LP64_ALIGN)
			&& ((uintptr_t)ptr & 0x7) != 0)  /* just for AVL */
		{
			acl_msg_fatal("%s(%d): %s, ptr(%lx) invalid, "
				"slice_length: %d", myname, __LINE__,
				slice->name, (long) ptr, slice->slice_length);
		}
#endif
		*((int*) (ptr - SLICE2_HEAD_SIZE)) = (int) slice2->mslots.islots;
		*((int*) (ptr - SLICE_OFF_SIZE)) = (int) (ptr - (char*) mbuf);
		slice2->mslots.slots[slice2->mslots.islots++] = ptr;
		ptr += slice->slice_length;
	}

	for (i = slice->page_nslots; i < incr_real; i++)
		slice2->mslots.slots[i] = NULL;

	slice->nbuf++;
	slice->length += slice->page_size;
}

static void *slice2_alloc(ACL_SLICE *slice)
{
	const char *myname = "slice2_alloc";
	SLICE2 *slice2 = (SLICE2*) slice;
	char *ptr;
	MBUF2 *mbuf;
	int  off, pos;

	if (slice2->mslots.islots == 0) {
		slice2_mbuf_alloc(slice);
	}

	ptr = (char*) slice2->mslots.slots[--slice2->mslots.islots];
	slice->used_length += slice->slice_size;

	off = *((int*) (ptr - SLICE_OFF_SIZE));
	if (off < 0)
		acl_msg_fatal("%s(%d): off(%d) invalid", myname, __LINE__, off);
	pos = *((int*) (ptr - SLICE2_HEAD_SIZE));
	if (pos < 0)
		acl_msg_fatal("%s(%d): pos(%d) invalid", myname, __LINE__, pos);

	mbuf = (MBUF2 *) (ptr - off);

	if (mbuf->mbuf.signature != SIGNATURE) {
		acl_msg_info("%s(%d): %s, off(%d), nused(%d), islots(%d)"
			", used_length(%d), slice_size(%d), slice_length(%d)"
			", page_nslots(%d), page_size(%d)", myname, __LINE__,
			slice->name, off, mbuf->mbuf.nused,
			slice2->mslots.islots, (int) slice->used_length,
			(int) slice->slice_size, (int) slice->slice_length,
			slice->page_nslots, slice->page_size);

		acl_msg_fatal("%s(%d): %s, corrupt or unallocated "
			"memory block(0x%x, 0x%x)", myname, __LINE__,
			slice->name, mbuf->mbuf.signature, SIGNATURE);
	}

	/* reset the slice chunk's pos to -1 */
	*((int*) (ptr - SLICE2_HEAD_SIZE)) = -1;

	mbuf->mbuf.nused++;
	return ptr;
}

static void slice2_mbuf_free(SLICE2 *slice2, MBUF2 *mbuf)
{
	const char *myname = "slice2_mbuf_free";
	char *ptr;
	int   i;
	int   pos;

#if 0
	ptr = mbuf->payload;
	for (i = 0; i < slice2->slice.page_nslots; i++) {
		ptr += SLICE2_HEAD_SIZE;
		pos = *((int*) (ptr - SLICE2_HEAD_SIZE));
		if (pos < 0 || pos >= slice2->mslots.islots)
			acl_msg_fatal("%s(%d): %s, pos(%d) invalid, islots(%d), page_nslots(%d)",
				myname, __LINE__, ((ACL_SLICE*) slice2)->name,
				pos, slice2->mslots.islots, slice2->slice.page_nslots);

		if (slice2->mslots.slots[pos] != ptr)
			acl_msg_fatal("%s(%d): pos(%d)'s(%lld, %lld) invalid",
				myname, __LINE__, pos, slice2->mslots.slots[pos], ptr);

		if (pos + 1 < slice2->mslots.islots) {
			slice2->mslots.slots[pos] = slice2->mslots.slots[slice2->mslots.islots -i - 1];

			/* reset the slice chunk's pos */
			*((int*) ((char*) slice2->mslots.slots[pos] - SLICE2_HEAD_SIZE)) = pos;
		}
		ptr += slice2->slice.slice_length;
	}

	slice2->mslots.islots -= slice2->slice.page_nslots;
#elif 0
	ptr = mbuf->payload;
	for (i = 0; i < slice2->slice.page_nslots; i++) {
		ptr += SLICE2_HEAD_SIZE;
		pos = *((int*) (ptr - SLICE2_HEAD_SIZE));
		if (pos < 0) {
			acl_msg_fatal("%s(%d): %s, pos(%d) invalid, "
				"islots(%d), page_nslots(%d)", myname, __LINE__,
				((ACL_SLICE*) slice2)->name, pos,
				slice2->mslots.islots, slice2->slice.page_nslots);
		} else if (pos + 1 > slice2->mslots.islots) {
			ptr += slice2->slice.slice_length;
			continue;
		} else if (pos + 1 == slice2->mslots.islots) {
			ptr += slice2->slice.slice_length;
			slice2->mslots.islots--;
			continue;
		}

		if (slice2->mslots.slots[pos] != ptr)
			acl_msg_fatal("%s(%d): pos(%d)'s(%lld, %lld) invalid",
				myname, __LINE__, pos,
				slice2->mslots.slots[pos], ptr);

		slice2->mslots.slots[pos] =
			slice2->mslots.slots[--slice2->mslots.islots];
		/* reset the slice chunk's pos */
		*((int*) ((char*) slice2->mslots.slots[pos]
			- SLICE2_HEAD_SIZE)) = pos;
		ptr += slice2->slice.slice_length;
	}
#else
	ptr = mbuf->payload + slice2->slice.slice_size
		* (slice2->slice.page_nslots - 1);
	for (i = slice2->slice.page_nslots - 1; i >= 0; i--) {
		pos = *((int*) (ptr));
		if (pos < 0) {
			acl_msg_fatal("%s(%d): %s, pos(%d) invalid, islots(%d),"
				" page_nslots(%d)", myname, __LINE__,
				((ACL_SLICE*) slice2)->name, pos,
				slice2->mslots.islots, slice2->slice.page_nslots);
		} else if (pos + 1 > slice2->mslots.islots) {
			ptr -= slice2->slice.slice_size;
			continue;
		} else if (pos + 1 == slice2->mslots.islots) {
			ptr -= slice2->slice.slice_size;
			slice2->mslots.islots--;
			continue;
		}

		if ((char*) slice2->mslots.slots[pos] != ptr + SLICE2_HEAD_SIZE)
			acl_msg_fatal("%s(%d): pos(%d)'s(%p, %p) invalid",
				myname, __LINE__, pos,
				slice2->mslots.slots[pos],
				(ptr + SLICE2_HEAD_SIZE));

		slice2->mslots.slots[pos] =
			slice2->mslots.slots[--slice2->mslots.islots];
		/* reset the slice chunk's pos */
		*((int*) ((char*) slice2->mslots.slots[pos]
			- SLICE2_HEAD_SIZE)) = pos;
		ptr -= slice2->slice.slice_size;
	}
#endif
	ring_detach(&mbuf->entry);
	acl_default_free(__FILE__, __LINE__, mbuf);
	slice2->slice.nbuf--;
	slice2->slice.nfree++;
}

static void slice2_free(ACL_SLICE *slice_dummy acl_unused, void *buf)
{
	const char *myname = "slice2_free";
	ACL_SLICE *slice;
	SLICE2 *slice2;
	char *ptr = (char*) buf;
	int   incr_real;
	int   off;
	MBUF2 *mbuf;

	off = *((int*) (ptr - SLICE_OFF_SIZE));
	if (off < 0)
		acl_msg_fatal("%s(%d): off(%d) invalid", myname, __LINE__, off);

	mbuf = (MBUF2 *) (ptr - off);
	if (mbuf->mbuf.signature != SIGNATURE)
		acl_msg_fatal("%s(%d), (off %d): corrupt or unallocated "
			"memory block(0x%x, 0x%x)", myname, __LINE__, off,
			mbuf->mbuf.signature, SIGNATURE);

	slice = mbuf->mbuf.slice;
	slice2 = (SLICE2*) slice;

	if (slice != slice_dummy)
		acl_msg_fatal("%s(%d): %s invalid",
			myname, __LINE__, slice->name);

	if (mbuf->mbuf.nused <= 0)
		acl_msg_fatal("%s(%d): %s, nused(%d) <= 0",
			myname, __LINE__, slice->name, mbuf->mbuf.nused);

	MBUF_SLOTS_SPACE(slice, &slice2->mslots, 1, incr_real);
	slice2->mslots.slots[slice2->mslots.islots] = ptr;

	/* reset the slice chunk's pos */
	*((int*) (ptr - SLICE2_HEAD_SIZE)) = (int) slice2->mslots.islots++;

	mbuf->mbuf.nused--;
	if (mbuf->mbuf.nused == 0 && !(slice->flag & ACL_SLICE_FLAG_RTGC_OFF)) {
		if (mbuf->payload > ptr - SLICE2_HEAD_SIZE)
			acl_msg_fatal("%s(%d): %s, ptr overflow",
				myname, __LINE__, slice->name);
		slice2_mbuf_free(slice2, mbuf);
	}

	slice->used_length -= slice->slice_size;
}

static int slice2_gc(ACL_SLICE *slice)
{
	SLICE2 *slice2 = (SLICE2*) slice;
	MBUF2 *mbuf;
	RING *iter, *tmp;

	for (iter = ring_succ(&slice2->mbuf_head); iter != &slice2->mbuf_head;) {
		tmp = ring_succ(iter);
		mbuf = RING_TO_APPL(iter, MBUF2, entry);
		if (mbuf->mbuf.nused == 0) {
			slice2_mbuf_free(slice2, mbuf);
		} else
			return 0;
		iter = tmp;
	}

	return 1;
}

static int slice2_used(ACL_SLICE *slice)
{
	SLICE2 *slice2 = (SLICE2*) slice;
	MBUF2 *mbuf;
	RING *iter;
	int   n = 0;

	for (iter = ring_succ(&slice2->mbuf_head); iter != &slice2->mbuf_head;) {
		mbuf = RING_TO_APPL(iter, MBUF2, entry);
		n += mbuf->mbuf.nused;
		iter = ring_succ(iter);
	}
	return n;
}

static void slice2_destroy(ACL_SLICE *slice)
{
	SLICE2 *slice2 = (SLICE2*) slice;
	RING *iter, *tmp;
	MBUF2 *mbuf;

	for (iter = ring_succ(&slice2->mbuf_head); iter != &slice2->mbuf_head;) {
		tmp = ring_succ(iter);
		mbuf = RING_TO_APPL(iter, MBUF2, entry);
		acl_default_free(__FILE__, __LINE__, mbuf);
		iter = tmp;
	}

	if (slice2->mslots.slots)
		acl_default_free(__FILE__, __LINE__, slice2->mslots.slots);
	acl_default_free(__FILE__, __LINE__, slice2);
}

static void slice2_stat(ACL_SLICE *slice,  ACL_SLICE_STAT *sbuf)
{
	SLICE2 *slice2 = (SLICE2*) slice;

	sbuf->nslots = slice2->mslots.nslots;
	sbuf->islots = slice2->mslots.islots;

	sbuf->page_nslots = slice->page_nslots;
	sbuf->page_size = slice->page_size;
	sbuf->slice_length = slice->slice_length;
	sbuf->slice_size = slice->slice_size;
	sbuf->nbuf = slice->nbuf;
	sbuf->length = slice->length;
	sbuf->used_length = slice->used_length;
	sbuf->flag = slice->flag;
}

static ACL_SLICE *slice2_create(int page_size,
	int slice_length, unsigned int flag)
{
	SLICE2 *slice;

	slice = (SLICE2 *) acl_default_calloc(__FILE__, __LINE__,
		1, sizeof(SLICE2));

	/* call the base ACL_SLICE's init function */
	
	slice_init((ACL_SLICE*) slice, flag);

	/* init the SLICE2's params */

	slice->mslots.slots = NULL;
	slice->mslots.nslots = 0;
	slice->mslots.islots = 0;

	ring_init(&slice->mbuf_head);

#ifdef	_LP64
	if ((flag & ACL_SLICE_FLAG_LP64_ALIGN) != 0) {
		if ((slice_length + SLICE2_HEAD_SIZE) % sizeof(uintptr_t) != 0) {
			slice_length = ((slice_length + SLICE2_HEAD_SIZE)
				/ sizeof(uintptr_t) + 1) * sizeof(uintptr_t)
				- SLICE2_HEAD_SIZE;
		}
	}
#endif

	/* reset the base ACL_SLICE's params */

	slice->slice.slice_length = slice_length;
	slice->slice.slice_size = slice_length + SLICE2_HEAD_SIZE;
	slice->slice.page_size = page_size;
	slice->slice.page_nslots = (page_size - sizeof(MBUF2))
		/slice->slice.slice_size;
	slice->slice.gap_size = page_size - slice->slice.page_nslots
		* slice->slice.slice_size;

	/* reset the base ACL_SLICE's callback */
	
	slice->slice.slice_destroy = slice2_destroy;
	slice->slice.slice_alloc = slice2_alloc;
	slice->slice.slice_free = slice2_free;
	slice->slice.slice_gc = slice2_gc;
	slice->slice.slice_used = slice2_used;
	slice->slice.slice_stat = slice2_stat;

	return (ACL_SLICE*) slice;
}

/*---------------------------- just for slice1 -------------------------------*/

static void slice1_mbuf_alloc(ACL_SLICE *slice)
{
	SLICE1 *slice1 = (SLICE1*) slice;
	MBUF1 *mbuf = (MBUF1*) acl_default_malloc(__FILE__, __LINE__,
			sizeof(MBUF1));
	int   i, incr_real = 0;
	char *ptr;

	mbuf->buf = (void*) acl_default_malloc(__FILE__, __LINE__,
			slice->page_size);
	ring_append(&slice1->mbuf_head, &mbuf->entry);
	ptr = (char*) mbuf->buf;

	slice->nalloc++;
	MBUF_SLOTS_SPACE(slice, &slice1->mslots, slice->page_nslots, incr_real);

	for (i = 0; i < slice->page_nslots; i++) {
		slice1->mslots.slots[slice1->mslots.islots++] = ptr;
		ptr += slice->slice_size;
	}
	for (i = slice->page_nslots; i < incr_real; i++)
		slice1->mslots.slots[i] = NULL;

	slice->nbuf++;
	slice->length += slice->page_size;
}

static void *slice1_alloc(ACL_SLICE *slice)
{
	SLICE1 *slice1 = (SLICE1*) slice;
	void *ptr;

	if (slice1->mslots.islots == 0)
		slice1_mbuf_alloc(slice);

	ptr = slice1->mslots.slots[slice1->mslots.islots - 1];
	slice1->mslots.islots--;
	slice->used_length += slice->slice_size;
	return ptr;
}

static void slice1_free(ACL_SLICE *slice, void *ptr)
{
	SLICE1 *slice1 = (SLICE1*) slice;
	int   incr_real;

	MBUF_SLOTS_SPACE(slice, &slice1->mslots, 1, incr_real);
	slice1->mslots.slots[slice1->mslots.islots++] = ptr;
	slice->used_length -= slice->slice_size;
}

static void slice1_mbuf_free(ACL_SLICE *slice, void *buf)
{
	const char *myname = "slice1_mbuf_free";
	SLICE1 *slice1 = (SLICE1*) slice;
	RING *iter;
	MBUF1 *mbuf;

#if 0
	FOREACH_RING_FORWARD(iter, &slice->mbuf_head) {
#else
	FOREACH_RING_BACKWARD(iter, &slice1->mbuf_head) {
#endif
		mbuf = RING_TO_APPL(iter, MBUF1, entry);
		if (buf == mbuf->buf) {
			ring_detach(&mbuf->entry);
			acl_default_free(__FILE__, __LINE__, mbuf->buf);
			acl_default_free(__FILE__, __LINE__, mbuf);
			slice->nbuf--;
			slice->nfree++;
			return;
		}
	}

	acl_msg_fatal("%s: unknown buf addr: 0x%p",
		myname, buf ? buf : 0);
}

static int cmp_fn(const void *p1, const void *p2)
{
	acl_assert(p1);
	acl_assert(p2);
	return (int) ((const char*) p1 - (const char*) p2);
}

static int slice1_gc(ACL_SLICE *slice)
{
	const char *myname = "slice1_gc";
	SLICE1 *slice1 = (SLICE1*) slice;
	char *ptr;
	int   i, length, j, n, k = -1;

	if (slice1->mslots.islots == 0)
		return 1;

	qsort(slice1->mslots.slots, slice1->mslots.islots,
		sizeof(void*), cmp_fn);

	j = 0;
	n = 0;
	length = 0;
	ptr = slice1->mslots.slots[j];
	for (i = 0; i < slice1->mslots.islots; i++) {
		if (ptr + slice->slice_size * n++ != slice1->mslots.slots[i]) {
			j = i;
			n = 0;
			ptr = slice1->mslots.slots[j];
			length = 0;
			sleep(1);
			continue;
		}

		length += slice->slice_size;
		if (length + slice->gap_size == slice->page_size) {
			slice1_mbuf_free(slice, ptr);
			slice1->mslots.islots -= n;
			k = n;
			break;
		}
	}

	if (slice1->mslots.islots > 0) {
		for (i = 0; i < k; i++) {
			slice1->mslots.slots[j + i] =
				slice1->mslots.slots[slice1->mslots.islots + i];
			if (slice1->mslots.slots[j + i] == NULL)
				acl_msg_fatal("%s: slots[%d] null",
					myname, j + i);
		}
	}

	return 1;
}

static void slice1_destroy(ACL_SLICE *slice)
{
	SLICE1 *slice1 = (SLICE1*) slice;
	RING *iter, *tmp;
	MBUF1 *mbuf;

	for (iter = ring_succ(&slice1->mbuf_head); iter != &slice1->mbuf_head;) {
		tmp = ring_succ(iter);
		mbuf = RING_TO_APPL(iter, MBUF1, entry);
		acl_default_free(__FILE__, __LINE__, mbuf->buf);
		acl_default_free(__FILE__, __LINE__, mbuf);
		iter = tmp;
	}
	if (slice1->mslots.slots)
		acl_default_free(__FILE__, __LINE__, slice1->mslots.slots);
	acl_default_free(__FILE__, __LINE__, slice1);
}

static int slice1_used(ACL_SLICE *slice acl_unused)
{
	const char *myname = "slice1_used";

	acl_msg_warn("%s(%d): not implement yet!", myname, __LINE__);
	return 0;
}

static void slice1_stat(ACL_SLICE *slice,  ACL_SLICE_STAT *sbuf)
{
	SLICE1 *slice1 = (SLICE1*) slice;

	sbuf->nslots = slice1->mslots.nslots;
	sbuf->islots = slice1->mslots.islots;

	sbuf->page_nslots = slice->page_nslots;
	sbuf->page_size = slice->page_size;
	sbuf->slice_length = slice->slice_length;
	sbuf->slice_size = slice->slice_size;
	sbuf->nbuf = slice->nbuf;
	sbuf->length = slice->length;
	sbuf->used_length = slice->used_length;
	sbuf->flag = slice->flag;
}

static ACL_SLICE *slice1_create(int page_size, int slice_length, unsigned int flag)
{
	SLICE1 *slice;

	slice = (SLICE1 *) acl_default_calloc(__FILE__, __LINE__,
			1, sizeof(SLICE1));

	/* call the base ACL_SLICE's init function */
	
	slice_init((ACL_SLICE*) slice, flag);

	/* init the SLICE1's params */

	slice->mslots.slots = NULL;
	slice->mslots.nslots = 0;
	slice->mslots.islots = 0;

	/* reset the base ACL_SLICE's params */
	slice->slice.slice_length = slice_length;
	slice->slice.slice_size = slice_length;
	slice->slice.page_size = page_size;
	slice->slice.page_nslots = page_size / slice->slice.slice_size;
	slice->slice.gap_size = page_size - slice->slice.page_nslots
		* slice->slice.slice_size;

	/* set the base ACL_SLICE's callback */

	slice->slice.slice_destroy = slice1_destroy;
	slice->slice.slice_alloc = slice1_alloc;
	slice->slice.slice_free = slice1_free;
	slice->slice.slice_gc = slice1_gc;
	slice->slice.slice_used = slice1_used;
	slice->slice.slice_stat = slice1_stat;

	/* init the SLICE1's params */
	
	ring_init(&slice->mbuf_head);

	return (ACL_SLICE*) slice;
}

/*------------------------   public functions   -----------------------------*/

void acl_slice_stat(ACL_SLICE *slice,  ACL_SLICE_STAT *sbuf)
{
	slice->slice_stat(slice, sbuf);
}

static void slice_init(ACL_SLICE *slice, unsigned int flag)
{
	slice->nbuf = 0;
	slice->length = 0;
	slice->used_length = 0;
	slice->nalloc = 0;
	slice->nfree = 0;
	slice->flag = flag;
}

ACL_SLICE *acl_slice_create(const char *name, int page_size,
	int slice_length, unsigned int flag)
{
	const char *myname = "acl_slice_create";
	ACL_SLICE *slice = NULL;
	int   size, sys_page_size;

#ifdef ACL_UNIX
	sys_page_size = getpagesize();
#elif defined(ACL_WINDOWS)
	SYSTEM_INFO info;

	memset(&info, 0, sizeof(SYSTEM_INFO));
	GetSystemInfo(&info);
	sys_page_size = info.dwPageSize;
	if (sys_page_size <= 0)
		sys_page_size = 4096;
#else
	sys_page_size = 4096;
#endif

	size = ((page_size - 1) / sys_page_size + 1) * sys_page_size;
	if (size <= 4096)
		size = sys_page_size;

	if (size / slice_length < 2) {
		acl_msg_warn("%s: slice_length(%d). page_size(%d)"
			" maybe too small, please increase it.",
			myname, slice_length, page_size);
		return NULL;
	}

	if ((flag & ACL_SLICE_FLAG_GC1))
		slice = slice1_create(size, slice_length, flag);
	else if ((flag & ACL_SLICE_FLAG_GC2))
		slice = slice2_create(size, slice_length, flag);
	else if ((flag & ACL_SLICE_FLAG_GC3))
		slice = slice3_create(size, slice_length, flag);
	else {
		acl_msg_error("%s: flag invalid", myname);
		return NULL;
	}

	snprintf(slice->name, sizeof(slice->name), "%s", name);
	return slice;
}

void acl_slice_destroy(ACL_SLICE *slice)
{
	slice->slice_destroy(slice);
}

int acl_slice_used(ACL_SLICE *slice)
{
	return slice->slice_used(slice);
}

void *acl_slice_alloc(ACL_SLICE *slice)
{
	return slice->slice_alloc(slice);
}

void *acl_slice_calloc(ACL_SLICE *slice)
{
	char *ptr = slice->slice_alloc(slice);

	if (ptr)
		memset(ptr, 0, slice->slice_length);
	return ptr;
}

void acl_slice_free2(ACL_SLICE *slice, void *ptr)
{
	slice->slice_free(slice, ptr);
}

void acl_slice_free(void *ptr)
{
	const char *myname = "acl_slice_free";
	int off;
	MBUF *mbuf;
	ACL_SLICE *slice;

	off = *((int*) ((char*) ptr - SLICE_OFF_SIZE));
	if (off < 0)
		acl_msg_fatal("%s(%d): off(%d) invalid", myname, __LINE__, off);
	mbuf = (MBUF *) ((char*) ptr - off);
	slice = mbuf->slice;
	slice->slice_free(slice, ptr);
}

int acl_slice_gc(ACL_SLICE *slice)
{
	return slice->slice_gc(slice);
}

/*----------------------------------------------------------------------------*/

void acl_slice_pool_init(ACL_SLICE_POOL *asp)
{
	int   i;

	for (i = 0; i < asp->nslice; i++) {
		char  name[256];
		int   elsize = asp->base * (i + 1), page_size, n;

		snprintf(name, sizeof(name), "(memory SIZE: %d)", elsize);
		if (elsize >= 102400)
			n = 10;
		else if (elsize >= 81920)
			n = 10;
		else if (elsize >= 40960)
			n = 10;
		else if (elsize >= 20480)
			n = 20;
		else if (elsize >= 10240)
			n = 20;
		else if (elsize >= 8192)
			n = 20;
		else if (elsize >= 4096)
			n = 30;
		else if (elsize >= 2048)
			n = 30;
		else if (elsize >= 1024)
			n = 30;
		else if (elsize >= 512)
			n = 40;
		else if (elsize >= 256)
			n = 40;
		else if (elsize >= 128)
			n = 40;
		else if (elsize >= 64)
			n = 50;
		else if (elsize >= 32)
			n = 50;
		else if (elsize >= 16)
			n = 50;
		else
			n = 50;
		page_size = elsize * n;
		asp->slices[i] = acl_slice_create(name, page_size,
					elsize, asp->slice_flag);
	}
}

ACL_SLICE_POOL *acl_slice_pool_create(int base, int nslice,
	unsigned int slice_flag)
{
	ACL_SLICE_POOL *asp = (ACL_SLICE_POOL*)
		acl_default_calloc(__FILE__, __LINE__, 1, sizeof(*asp));

	asp->base = base;
	asp->nslice = nslice;
	asp->slice_flag = slice_flag;
	asp->slices = (ACL_SLICE**) acl_default_calloc(__FILE__, __LINE__,
			nslice, sizeof(ACL_SLICE*));
	acl_slice_pool_init(asp);
	return asp;
}

void acl_slice_pool_destroy(ACL_SLICE_POOL *asp)
{
	int   i;

	for (i = 0; i < asp->nslice; i++) {
		acl_slice_destroy(asp->slices[i]);
	}

	acl_default_free(__FILE__, __LINE__, asp->slices);
	acl_default_free(__FILE__, __LINE__, asp);
}

int acl_slice_pool_used(ACL_SLICE_POOL *asp)
{
	int   i, n = 0;

	for (i = 0; i < asp->nslice; i++) {
		n += acl_slice_used(asp->slices[i]);
	}

	return n;
}

void acl_slice_pool_gc(ACL_SLICE_POOL *asp)
{
	int   i;

	for (i = 0; i < asp->nslice; i++) {
		acl_slice_gc(asp->slices[i]);
	}
}

void acl_slice_pool_clean(ACL_SLICE_POOL *asp)
{
	int   i;

	for (i = 0; i < asp->nslice; i++) {
		acl_slice_destroy(asp->slices[i]);
		asp->slices[i] = NULL;
	}
}

void acl_slice_pool_reset(ACL_SLICE_POOL *asp)
{
	acl_slice_pool_clean(asp);
	acl_slice_pool_init(asp);
}

void acl_slice_pool_free(const char *filename, int line, void *buf)
{
	char *ptr = (char*) buf;

	ptr -= sizeof(size_t);  /* 移至内存头的标志位 */
	if (*((size_t*) ptr) == 0)
		acl_default_free(filename, line, ptr);
	else {
		acl_slice_free(ptr);
	}
}

void *acl_slice_pool_alloc(const char *filename, int line,
	ACL_SLICE_POOL *asp, size_t size)
{
	char *ptr;
	int   n;

	size += sizeof(size_t);  /* 头部留出空间做为标志位 */
	if (asp == NULL || (int) size >= asp->base * asp->nslice) {
		ptr = (char*) acl_default_malloc(filename, line, size);
		if (ptr) {
			*((size_t*) ptr) = 0;
			ptr += sizeof(size_t);
		}
		return ptr;
	}
	n = (int) size / asp->base;
	if (size % asp->base != 0)
		n++;

	ptr = (char*) acl_slice_alloc(asp->slices[n - 1]);
	if (ptr) {
		*((size_t*) ptr) = 1;
		ptr += sizeof(size_t);
	}
	return ptr;
}

void *acl_slice_pool_calloc(const char *filename, int line,
	ACL_SLICE_POOL *asp, size_t nmemb, size_t size)
{
	void *ptr = acl_slice_pool_alloc(filename, line, asp, nmemb * size);

	if (ptr)
		memset(ptr, 0, nmemb * size);
	return ptr;
}

void *acl_slice_pool_realloc(const char *filename, int line,
	ACL_SLICE_POOL *asp, void *ptr, size_t size)
{
	void *buf;

	buf = acl_slice_pool_alloc(filename, line, asp, size);
	memcpy(buf, ptr, size);
	acl_slice_pool_free(filename, line, ptr);
	return buf;
}

void *acl_slice_pool_memdup(const char *filename, int line,
	ACL_SLICE_POOL *asp, const void *ptr, size_t len)
{
	void *buf = acl_slice_pool_alloc(filename, line, asp, len);

	memcpy(buf, ptr, len);
	return buf;
}

char *acl_slice_pool_strdup(const char *filename, int line,
	ACL_SLICE_POOL *asp, const char *str)
{
	size_t n = strlen(str) + 1;
	char *ptr = (char*) acl_slice_pool_alloc(filename, line, asp, n);

	memcpy(ptr, str, n);
	return ptr;
}

char *acl_slice_pool_strndup(const char *filename, int line,
	ACL_SLICE_POOL *asp, const char *str, size_t len)
{
	char *ptr = (char*) acl_slice_pool_alloc(filename, line, asp, len + 1);

	memcpy(ptr, str, len);
	ptr[len] = 0;
	return ptr;
}
