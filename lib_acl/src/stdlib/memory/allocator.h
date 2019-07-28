#ifndef __ALLOCATOR_INCLUDE_H__
#define __ALLOCATOR_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_stack.h"
#include "stdlib/acl_allocator.h"

struct ACL_MEM_POOL {
	ACL_STACK *pstack;	/* stack for free pointers */
	const char *label;
	size_t obj_size;
	acl_mem_type type;
#if DEBUG_MEMPOOL
	size_t real_obj_size;	/* with alignment */
#endif
	void (*after_alloc_fn)(void *obj, void *pool_ctx);
	void (*before_free_fn)(void *obj, void *pool_ctx);
	void *pool_ctx;
};

struct ACL_ALLOCATOR {
	ACL_STACK *pools;
	size_t mem_idle_limit;
	unsigned int mem_pool_alloc_calls;
	unsigned int mem_pool_free_calls;
	ACL_MEM_POOL *MemPools[ACL_MEM_TYPE_MAX];

	ACL_MEM_POOL *(*pool_create_fn)(void);
	void (*pool_config_fn)(ACL_ALLOCATOR *, size_t);
	void (*pool_destroy_fn)(ACL_MEM_POOL *);
	void (*pool_clean_fn)(ACL_ALLOCATOR *);

	int (*pool_if_used)(const ACL_MEM_POOL *);
	int (*pool_inuse_count)(const ACL_MEM_POOL *);
	size_t (*pool_inuse_size)(const ACL_MEM_POOL *);
	size_t (*pool_total_allocated)(ACL_ALLOCATOR *);

	void *(*mem_alloc_fn)(ACL_ALLOCATOR *, ACL_MEM_POOL *);
	void  (*mem_free_fn)(ACL_ALLOCATOR *, ACL_MEM_POOL *, void *);
};

/* in acl_allocator.c */
extern ACL_ALLOCATOR *allocator_alloc(size_t size);

/* in vstring_pool.c */
extern void vstring_pool_create(ACL_ALLOCATOR *allocator);

/* in membuf_pool.c */
extern void mem_pool_create(ACL_ALLOCATOR *allocator);

#ifdef __cplusplus
}
#endif

#endif

