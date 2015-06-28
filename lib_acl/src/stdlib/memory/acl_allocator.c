#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_malloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_allocator.h"

#endif

#include "squid_allocator.h"
#include "allocator.h"

static size_t __min_gross_size = 8;		/* 8 byte */
static size_t __max_gross_size = 1048576;	/* 1MB */

#define CHECK_TYPE(_type) do { \
	if (_type >= ACL_MEM_TYPE_MAX) \
		acl_msg_fatal("%s: type(%d) > ACL_MEM_TYPE_MAX(%d)", \
			myname, _type, ACL_MEM_TYPE_MAX); \
} while (0)

static ACL_MEM_POOL *pool_create(void)
{
	const char *myname = "pool_create";

	acl_msg_fatal("%s: not supported!", myname);

	return NULL;
}

static void pool_destroy(ACL_MEM_POOL * pool acl_unused)
{
	const char *myname = "pool_destroy";

	acl_msg_fatal("%s: not supported!", myname);
}

static void pool_clean(ACL_ALLOCATOR *allocator acl_unused)
{
	const char *myname = "pool_clean";

	acl_msg_fatal("%s: not supported!", myname);
}

static void *mem_alloc(ACL_ALLOCATOR *allocator acl_unused,
	ACL_MEM_POOL * pool acl_unused)
{
	const char *myname = "mem_alloc";

	acl_msg_fatal("%s: not supported!", myname);
	return NULL;
}

static void mem_free(ACL_ALLOCATOR *allocator acl_unused,
	ACL_MEM_POOL *pool acl_unused, void *obj acl_unused)
{
	const char *myname = "mem_free";

	acl_msg_fatal("%s: not supported!", myname);
}

static int pool_ifused(const ACL_MEM_POOL *pool acl_unused)
{
	const char *myname = "pool_ifused";

	acl_msg_fatal("%s: not supported!", myname);
	return 0;
}

static int pool_inuse_count(const ACL_MEM_POOL *pool acl_unused)
{
	const char *myname = "pool_inuse_count";

	acl_msg_fatal("%s: not supported!", myname);
	return 0;
}

static size_t pool_inuse_size(const ACL_MEM_POOL *pool acl_unused)
{
	const char *myname = "pool_inuse_size";

	acl_msg_fatal("%s: not supported!", myname);
	return 0;
}

static size_t pool_total_allocated(ACL_ALLOCATOR *pool acl_unused)
{
	const char *myname = "pool_total_allocated";

	acl_msg_fatal("%s: not supported!", myname);
	return 0;
}

ACL_ALLOCATOR *allocator_alloc(size_t size)
{
	ACL_ALLOCATOR *allocator;

	allocator = (ACL_ALLOCATOR *) acl_default_calloc(__FILE__, __LINE__,
			1, size);
	allocator->pools = acl_stack_create(100);

	allocator->pool_create_fn = pool_create;
	allocator->pool_destroy_fn = pool_destroy;
	allocator->pool_clean_fn = pool_clean;

	allocator->pool_if_used = pool_ifused;
	allocator->pool_inuse_count = pool_inuse_count;
	allocator->pool_inuse_size = pool_inuse_size;
	allocator->pool_total_allocated = pool_total_allocated;

	allocator->mem_alloc_fn = mem_alloc;
	allocator->mem_free_fn = mem_free;

	return allocator;
}

ACL_ALLOCATOR *acl_allocator_create(size_t mem_limit)
{
	ACL_ALLOCATOR *allocator;

	allocator = squid_allocator_create();
	if (allocator->pool_config_fn)
		allocator->pool_config_fn(allocator, mem_limit);

	/* 创建两个默认的内存分配类型池 */
	mem_pool_create(allocator);
	vstring_pool_create(allocator);

	return allocator;
}

void acl_allocator_ctl(int name, ...)
{
	const char *myname = "acl_allocator_ctl";
	va_list ap;

	va_start(ap, name);
	for (; name != ACL_ALLOCATOR_CTL_END; name = va_arg(ap, int)) {
		switch(name) {
		case ACL_ALLOCATOR_CTL_MIN_SIZE:
			__min_gross_size = va_arg(ap, int);
			break;
		case ACL_ALLOCATOR_CTL_MAX_SIZE:
			__max_gross_size = va_arg(ap, int);
			break;
		default:
			acl_msg_panic("%s: bad name %d", myname, name);
		}
		
	}
	va_end(ap);
}

void acl_allocator_config(ACL_ALLOCATOR *allocator, size_t mem_limit)
{
	if (allocator->pool_config_fn)
		allocator->pool_config_fn(allocator, mem_limit);
}

void acl_allocator_free(ACL_ALLOCATOR *allocator)
{
	ACL_MEM_POOL *pool;

	while (1) {
		pool = acl_stack_pop(allocator->pools);
		if (pool == NULL)
			break;
		allocator->pool_destroy_fn(pool);
	}

	acl_stack_destroy(allocator->pools, NULL);
	acl_default_free(__FILE__, __LINE__, allocator);
}

ACL_MEM_POOL *acl_allocator_pool_add(ACL_ALLOCATOR *allocator,
	const char *label, size_t obj_size, acl_mem_type type,
	void (*after_alloc_fn)(void *obj, void *pool_ctx),
	void (*before_free_fn)(void *obj, void *pool_ctx), void *pool_ctx)
{
	const char *myname = "acl_allocator_pool_add";
	ACL_MEM_POOL *pool;

	CHECK_TYPE(type);

	pool = allocator->pool_create_fn();
	pool->label = label;
	pool->obj_size = obj_size;
#if DEBUG_MEMPOOL
	pool->real_obj_size = (obj_size & 7) ? (obj_size | 7) + 1 : obj_size;
#endif
	pool->type = type;
	pool->after_alloc_fn = after_alloc_fn;
	pool->before_free_fn = before_free_fn;
	pool->pool_ctx = pool_ctx;
	pool->pstack = acl_stack_create(1000);

	allocator->MemPools[type] = pool;
	acl_stack_append(allocator->pools, pool);

	return pool;
}

void acl_allocator_pool_remove(ACL_ALLOCATOR *allocator, ACL_MEM_POOL *pool)
{
	const char *myname = "acl_allocatorr_pool_remove";
	ACL_MEM_POOL *data;
	ACL_ITER iter;

	if (allocator == NULL || pool == NULL)
		acl_msg_fatal("%s: input invalid", myname);

	acl_foreach(iter, allocator->pools) {
		data = (ACL_MEM_POOL*) iter.data;
		if (data == pool) {
			allocator->MemPools[pool->type] = NULL;
			allocator->pool_destroy_fn(data);
			acl_stack_delete(allocator->pools, iter.i, NULL);
			break;
		}
	}
}

void *acl_allocator_mem_alloc(ACL_ALLOCATOR *allocator, acl_mem_type type)
{
	const char *myname = "acl_allocator_mem_alloc";
	ACL_MEM_POOL *pool;

	CHECK_TYPE(type);
	
	pool = allocator->MemPools[type];
	return allocator->mem_alloc_fn(allocator, pool);
}

void acl_allocator_mem_free(ACL_ALLOCATOR *allocator,
	acl_mem_type type, void *obj)
{
	const char *myname = "acl_allocator_mem_free";
	ACL_MEM_POOL *pool;

	CHECK_TYPE(type);

	pool = allocator->MemPools[type];
	allocator->mem_free_fn(allocator, pool, obj);
}

/* Find the best fit ACL_MEM_TYPE_X_BUF type */

static acl_mem_type memBufFindSizeType(size_t net_size, size_t *gross_size)
{
	acl_mem_type type;
	size_t size;

	if (net_size < __min_gross_size || net_size > __max_gross_size) {
		if (gross_size)
			*gross_size = net_size;
		return ACL_MEM_TYPE_NONE;
	}

	if (net_size <= 8) {
		type = ACL_MEM_TYPE_8_BUF;
		size = 8;
	} else if (net_size <= 16) {
		type = ACL_MEM_TYPE_16_BUF;
		size = 16;
	} else if (net_size <= 32) {
		type = ACL_MEM_TYPE_32_BUF;
		size = 32;
	} else if (net_size <= 64) {
		type = ACL_MEM_TYPE_64_BUF;
		size = 64;
	} else if (net_size <= 128) {
		type = ACL_MEM_TYPE_128_BUF;
		size = 128;
	} else if (net_size <= 256) {
		type = ACL_MEM_TYPE_256_BUF;
		size = 256;
	} else if (net_size <= 512) {
		type = ACL_MEM_TYPE_512_BUF;
		size = 512;
	} else if (net_size <= 1024) {
		type = ACL_MEM_TYPE_1K_BUF;
		size = 1024;
	} else  if (net_size <= 2048) {
		type = ACL_MEM_TYPE_2K_BUF;
		size = 2048;
	} else if (net_size <= 4096) {
		type = ACL_MEM_TYPE_4K_BUF;
		size = 4096;
	} else if (net_size <= 8192) {
		type = ACL_MEM_TYPE_8K_BUF;
		size = 8192;
	} else if (net_size <= 16384) {
		type = ACL_MEM_TYPE_16K_BUF;
		size = 16384;
	} else if (net_size <= 32768) {
		type = ACL_MEM_TYPE_32K_BUF;
		size = 32768;
	} else if (net_size <= 65536) {
		type = ACL_MEM_TYPE_64K_BUF;
		size = 65536;
	} else if (net_size <= 131072) {
		type = ACL_MEM_TYPE_128K_BUF;
		size = 131072;
	} else if (net_size <= 262144) {
		type = ACL_MEM_TYPE_256K_BUF;
		size = 262144;
	} else if (net_size <= 524288) {
		type = ACL_MEM_TYPE_512K_BUF;
		size = 524288;
	} else if (net_size <= 1048576) {
		type = ACL_MEM_TYPE_1M_BUF;
		size = 1048576;
	} else {
		type = ACL_MEM_TYPE_NONE;
		size = net_size;
	}
	if (gross_size)
		*gross_size = size;
	return type;
}

void *acl_allocator_membuf_alloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, size_t size)
{
	size_t gross_size;
	acl_mem_type type = memBufFindSizeType(size, &gross_size);

	if (type != ACL_MEM_TYPE_NONE)
		return acl_allocator_mem_alloc(allocator, type);
	else
		return acl_default_malloc(filename, line, size);
}

void *acl_allocator_membuf_realloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, void *oldbuf, size_t size)
{
	/* XXX This can be optimized on very large buffers to use realloc() */
	void *newbuf = acl_allocator_membuf_alloc(filename, line,
			allocator, size);

	if (oldbuf) {
		size_t data_size;

		acl_default_memstat(filename, line, oldbuf, &data_size, NULL);
		memcpy(newbuf, oldbuf, data_size > size ? size : data_size);
		acl_allocator_membuf_free(filename, line, allocator, oldbuf);
	}
	return newbuf;
}

void acl_allocator_membuf_free(const char *filename, int line,
	ACL_ALLOCATOR *allocator, void *buf)
{
	size_t gross_size;
	acl_mem_type type;

	acl_default_memstat(filename, line, buf, &gross_size, NULL);
	type = memBufFindSizeType(gross_size, NULL);

	if (type != ACL_MEM_TYPE_NONE)
		acl_allocator_mem_free(allocator, type, buf);
	else
		acl_default_free(filename, line, buf);
}

int acl_allocator_pool_ifused(ACL_ALLOCATOR *allocator, acl_mem_type type)
{
	const char *myname = "acl_allocator_pool_ifused";

	CHECK_TYPE(type);
	return allocator->pool_if_used(allocator->MemPools[type]);
}

int acl_allocator_pool_inuse_count(ACL_ALLOCATOR *allocator, acl_mem_type type)
{
	const char *myname = "acl_allocator_pool_inuse_count";

	CHECK_TYPE(type);
	return allocator->pool_inuse_count(allocator->MemPools[type]);
}

int acl_allocator_pool_inuse_size(ACL_ALLOCATOR *allocator, acl_mem_type type)
{
	const char *myname = "acl_allocator_pool_inuse_size";

	CHECK_TYPE(type);
	return (int) allocator->pool_inuse_size(allocator->MemPools[type]);
}

int acl_allocator_pool_total_allocated(ACL_ALLOCATOR *allocator)
{
	return (int) allocator->pool_total_allocated(allocator);
}
