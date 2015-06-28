#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_allocator.h"
#include "stdlib/acl_stack.h"

#endif

#include "allocator.h"
#include "squid_allocator.h"

/* huge constant to set mem_idle_limit to "unlimited" */
#define MB ((size_t)1024*1024)
static const size_t mem_unlimited_size = 2 * 1024 * MB - 1;

/* gb_type operations */
#define gb_flush_limit (0x3FFFFFFF)
#define gb_inc(gb, delta) { \
	if ((gb)->bytes > gb_flush_limit || delta > gb_flush_limit) \
		gb_flush(gb); \
	(gb)->bytes += delta; \
	(gb)->count++; \
}

static size_t memPoolInUseSize(const ACL_MEM_POOL * pool);
static int memPoolInUseCount(const ACL_MEM_POOL * pool);

/* MemMeter */
static void memMeterSyncHWater(MemMeter * m);

#define memMeterCheckHWater(m) do { \
	if ((m).hwater_level < (m).level) \
	memMeterSyncHWater(&(m)); \
} while (0)
#define memMeterInc(m) { (m).level++; memMeterCheckHWater(m); }
#define memMeterDec(m) { (m).level--; }
#define memMeterAdd(m, sz) { (m).level += (sz); memMeterCheckHWater(m); }
#define memMeterDel(m, sz) { (m).level -= (sz); }

static double toMB(size_t size)
{
	return ((double) size) / MB;
}

static size_t toKB(size_t size)
{
	return (size + 1024 - 1) / 1024;
}

/* to-do: make debug level a parameter? */
static void memPoolDescribe(const ACL_MEM_POOL * pool)
{
	acl_assert(pool);
	acl_msg_info("%-20s: %6d x %4d bytes = %5ld KB",
		pool->label, memPoolInUseCount(pool),
		(int) pool->obj_size,
		(long int) toKB(memPoolInUseSize((const ACL_MEM_POOL *)pool)));
}

/* MemMeter */

static void memMeterSyncHWater(MemMeter * m)
{
	acl_assert(m);
	if (m->hwater_level < m->level) {
		m->hwater_level = m->level;
		m->hwater_stamp = time(NULL);
	}
}

static void memPoolShrink(SQUID_MEM_ALLOCATOR *allocator,
	MemPool *pool, size_t new_limit)
{
	char  *ptr;

	acl_assert(pool);
	while (pool->meter.idle.level > new_limit
		&& acl_stack_size(((ACL_MEM_POOL *) pool)->pstack) > 0) {
		memMeterDec(pool->meter.alloc);
		memMeterDec(pool->meter.idle);
		memMeterDel(allocator->TheMeter.idle,
			((ACL_MEM_POOL *)pool)->obj_size);
		memMeterDel(allocator->TheMeter.alloc,
			((ACL_MEM_POOL *) pool)->obj_size);
		ptr = acl_stack_pop(((ACL_MEM_POOL *) pool)->pstack);
		acl_default_free(__FILE__, __LINE__, ptr);
	}
	acl_assert(pool->meter.idle.level <= new_limit);	/* paranoid */
}

static void memShrink(SQUID_MEM_ALLOCATOR *allocator, size_t new_limit)
{
	size_t start_limit = allocator->TheMeter.idle.level;
	ACL_ITER iter;

	/* first phase: cut proportionally to the pool idle size */

	acl_foreach_reverse(iter, ((ACL_ALLOCATOR *) allocator)->pools) {
		MemPool *pool = (MemPool*) iter.data;
		const size_t target_pool_size = (size_t) ((double)
			pool->meter.idle.level * new_limit) / start_limit;
		memPoolShrink(allocator, pool, target_pool_size);
	}

	acl_msg_info("memShrink: 1st phase done with %ld KB left",
		(long int) toKB(allocator->TheMeter.idle.level));
	
	/* second phase: cut to 0 */

	acl_foreach_reverse(iter, ((ACL_ALLOCATOR *) allocator)->pools) {
		MemPool *pool = (MemPool*) iter.data;
		memPoolShrink(allocator, pool, 0);
	}

	acl_msg_info("memShrink: 2nd phase done with %ld KB left",
		(long int) toKB(allocator->TheMeter.idle.level));
	acl_assert(allocator->TheMeter.idle.level <= new_limit); /* paranoid */
}

static void memCleanModule(ACL_ALLOCATOR *allocator)
{
	int dirty_count = 0;
	ACL_ITER iter;

	acl_foreach_reverse(iter, allocator->pools) {
		ACL_MEM_POOL *pool = (ACL_MEM_POOL*) iter.data;
		if (!pool)
			continue;
		if (memPoolInUseCount(pool)) {
			memPoolDescribe(pool);
			dirty_count++;
		} else {
			allocator->pool_destroy_fn(pool);
		}
	}

	if (dirty_count)
		acl_msg_warn("memCleanModule: %d pools are left dirty",
			dirty_count);

	/* we clean the stack anyway */

	acl_stack_clean(allocator->pools, NULL);
}

/* Initialization */

static void memConfigure(ACL_ALLOCATOR *allocator, size_t mem_limit)
{
	size_t new_pool_limit = allocator->mem_idle_limit;
	/* set to configured value first */

	if (mem_limit > 0)
		new_pool_limit = mem_limit;
	else if (mem_limit == 0)
		new_pool_limit = 0;
	else
		new_pool_limit = mem_unlimited_size;
	/* shrink memory pools if needed */
	if (((SQUID_MEM_ALLOCATOR *) allocator)->TheMeter.idle.level
		> new_pool_limit)
	{
		acl_msg_info("Shrinking idle mem pools to %.2f MB",
			toMB(new_pool_limit));
		memShrink((SQUID_MEM_ALLOCATOR *) allocator, new_pool_limit);
	}
	acl_assert(((SQUID_MEM_ALLOCATOR *) allocator)->TheMeter.idle.level
		<= new_pool_limit);
	allocator->mem_idle_limit = new_pool_limit;
}

/* MemPool */

static ACL_MEM_POOL *memPoolCreate(void)
{
	ACL_MEM_POOL *pool = acl_default_calloc(__FILE__, __LINE__,
		1, sizeof(MemPool));
	MemPool *squid_pool = (MemPool *) pool;

	memset(&squid_pool->meter, 0, sizeof(squid_pool->meter));
	return pool;
}

static void memPoolDestroy(ACL_MEM_POOL * pool)
{
	void *obj;
	ACL_ITER iter;

	acl_foreach_reverse(iter, pool->pstack) {
		obj = iter.data;
		if (pool->before_free_fn)
			pool->before_free_fn(obj, pool->pool_ctx);
		acl_default_free(__FILE__, __LINE__, obj);
	}

	acl_default_free(__FILE__, __LINE__, pool);
}

#if DEBUG_MEMPOOL
#define MEMPOOL_COOKIE(p) ((void *)((unsigned long)(p) ^ 0xDEADBEEF))
struct mempool_cookie {
	MemPool *pool;
	void *cookie;
};
#endif

static void *memPoolAlloc(ACL_ALLOCATOR *allocator, ACL_MEM_POOL * pool)
{
	SQUID_MEM_ALLOCATOR *squid_allocator =
		(SQUID_MEM_ALLOCATOR *) allocator;
	MemPool *squid_pool = (MemPool *) pool;
	void *obj;

	memMeterInc(squid_pool->meter.inuse);
	gb_inc(&squid_pool->meter.total, 1);
	gb_inc(&squid_allocator->TheMeter.total, pool->obj_size);
	memMeterAdd(squid_allocator->TheMeter.inuse, pool->obj_size);
	gb_inc(&squid_allocator->mem_traffic_volume, pool->obj_size);
	allocator->mem_pool_alloc_calls++;

	if (acl_stack_size(pool->pstack)) {
		acl_assert(squid_pool->meter.idle.level);
		memMeterDec(squid_pool->meter.idle);
		memMeterDel(squid_allocator->TheMeter.idle, pool->obj_size);
		gb_inc(&squid_pool->meter.saved, 1);
		gb_inc(&squid_allocator->TheMeter.saved, pool->obj_size);
		obj = acl_stack_pop(pool->pstack);

#if DEBUG_MEMPOOL
		(void) VALGRIND_MAKE_READABLE(obj, pool->real_obj_size
			+ sizeof(struct mempool_cookie));
#else
		(void) VALGRIND_MAKE_READABLE(obj, pool->obj_size);
#endif
#if DEBUG_MEMPOOL
		{
			struct mempool_cookie *cookie = (void *)
				(((unsigned char *) obj) + pool->real_obj_size);
			acl_assert(cookie->cookie == MEMPOOL_COOKIE(obj));
			acl_assert(cookie->pool == pool);
			(void) VALGRIND_MAKE_NOACCESS(cookie, sizeof(cookie));
		}
#endif
	} else {
		acl_assert(!squid_pool->meter.idle.level);
		memMeterInc(squid_pool->meter.alloc);
		memMeterAdd(squid_allocator->TheMeter.alloc, pool->obj_size);
#if DEBUG_MEMPOOL
		{
			struct mempool_cookie *cookie;
			obj = acl_default_malloc(__FILE__, __LINE__,
				pool->real_obj_size
				+ sizeof(struct mempool_cookie));
			cookie = (struct mempool_cookie *)
				(((unsigned char *) obj) + pool->real_obj_size);
			cookie->cookie = MEMPOOL_COOKIE(obj);
			cookie->pool = pool;
			(void) VALGRIND_MAKE_NOACCESS(cookie, sizeof(cookie));
		}
#else
		obj = acl_default_malloc(__FILE__, __LINE__, pool->obj_size);
#endif
	}

	if (pool->after_alloc_fn)
		pool->after_alloc_fn(obj, pool->pool_ctx);

	return obj;
}

static void memPoolFree(ACL_ALLOCATOR *allocator,
	ACL_MEM_POOL * pool, void *obj)
{
	SQUID_MEM_ALLOCATOR *squid_allocator =
		(SQUID_MEM_ALLOCATOR *) allocator;
	MemPool *squid_pool = (MemPool *) pool;

	acl_assert(pool && obj);
	memMeterDec(squid_pool->meter.inuse);
	memMeterDel(squid_allocator->TheMeter.inuse,	pool->obj_size);
	allocator->mem_pool_free_calls++;
	(void) VALGRIND_CHECK_WRITABLE(obj, pool->obj_size);
#if DEBUG_MEMPOOL
	{
		struct mempool_cookie *cookie = (void *)
			(((unsigned char *) obj) + pool->real_obj_size);
		(void) VALGRIND_MAKE_READABLE(cookie, sizeof(cookie));
		acl_assert(cookie->cookie == MEMPOOL_COOKIE(obj));
		acl_assert(cookie->pool == pool);
	}
#endif
	if (squid_allocator->TheMeter.idle.level + pool->obj_size
		<= allocator->mem_idle_limit)
	{
		memMeterInc(squid_pool->meter.idle);
		memMeterAdd(squid_allocator->TheMeter.idle, pool->obj_size);
#if DEBUG_MEMPOOL
		(void) VALGRIND_MAKE_NOACCESS(obj, pool->real_obj_size
				+ sizeof(struct mempool_cookie));
#else
		(void) VALGRIND_MAKE_NOACCESS(obj, pool->obj_size);
#endif
		if (pool->before_free_fn)
			pool->before_free_fn(obj, pool->pool_ctx);

		/* xxx: I should add some here--zsx */
		/* memset(obj, 0, pool->obj_size); */
		acl_stack_append(pool->pstack, obj);
	} else {
		memMeterDec(squid_pool->meter.alloc);
		memMeterDel(squid_allocator->TheMeter.alloc, pool->obj_size);
		if (pool->before_free_fn)
			pool->before_free_fn(obj, pool->pool_ctx);
		acl_default_free(__FILE__, __LINE__, obj);
	}
	acl_assert(squid_pool->meter.idle.level
		<= squid_pool->meter.alloc.level);
}

static int memPoolWasUsed(const ACL_MEM_POOL * pool)
{
	acl_assert(pool);
	return ((const MemPool *) pool)->meter.alloc.hwater_level > 0;
}

static int memPoolInUseCount(const ACL_MEM_POOL * pool)
{
	acl_assert(pool);
	return (int) (((const MemPool *) pool)->meter.inuse.level);
}

static size_t memPoolInUseSize(const ACL_MEM_POOL * pool)
{
	acl_assert(pool);
	return (pool->obj_size * ((const MemPool *) pool)->meter.inuse.level);
}

static size_t memTotalAllocated(ACL_ALLOCATOR *allocator)
{
	return ((SQUID_MEM_ALLOCATOR *) allocator)->TheMeter.alloc.level;
}

ACL_ALLOCATOR *squid_allocator_create(void)
{
	ACL_ALLOCATOR *allocator;

	allocator = allocator_alloc(sizeof(SQUID_MEM_ALLOCATOR));

	allocator->pool_create_fn = memPoolCreate;
	allocator->pool_config_fn = memConfigure;
	allocator->pool_destroy_fn = memPoolDestroy;
	allocator->pool_clean_fn = memCleanModule;

	allocator->pool_if_used = memPoolWasUsed;
	allocator->pool_inuse_count = memPoolInUseCount;
	allocator->pool_inuse_size = memPoolInUseSize;
	allocator->pool_total_allocated = memTotalAllocated;

	allocator->mem_alloc_fn = memPoolAlloc;
	allocator->mem_free_fn = memPoolFree;

	memset(&((SQUID_MEM_ALLOCATOR *) allocator)->TheMeter,
		0, sizeof(MemPoolMeter));

	return (ACL_ALLOCATOR *) allocator;
}
