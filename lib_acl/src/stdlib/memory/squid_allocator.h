#ifndef __SQUID_ALLOCATOR_INCLUDE_H__
#define __SQUID_ALLOCATOR_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_allocator.h"

#include "allocator.h"
#include <time.h> /* for time_t */

#if WITH_VALGRIND
#include <valgrind/memcheck.h>
#else
#define VALGRIND_MAKE_NOACCESS(a,b) (0)
#define VALGRIND_MAKE_WRITABLE(a,b) (0)
#define VALGRIND_MAKE_READABLE(a,b) (0)
#define VALGRIND_CHECK_WRITABLE(a,b) (0)
#define VALGRIND_CHECK_READABLE(a,b) (0)
#define VALGRIND_MALLOCLIKE_BLOCK(a,b,c,d)
#define VALGRIND_FREELIKE_BLOCK(a,b)
#define RUNNING_ON_VALGRIND 0
#endif /* WITH_VALGRIND */

typedef struct {
	unsigned int bytes;
	unsigned int kb;
} kb_t;

typedef struct {
	size_t count;
	size_t bytes;
	size_t gb;
} gb_t;


typedef struct MemMeter MemMeter;
typedef struct MemPoolMeter MemPoolMeter;
typedef struct MemPool MemPool;

/* object to track per-action memory usage (e.g. #idle objects) */
struct MemMeter {
	size_t level;			/* current level (count or volume) */
	size_t hwater_level;	/* high water mark */
	time_t hwater_stamp;	/* timestamp of last high water mark change */
};

/* object to track per-pool memory usage (alloc = inuse+idle) */
struct MemPoolMeter {
	MemMeter alloc;
	MemMeter inuse;
	MemMeter idle;
	gb_t saved;
	gb_t total;
};

/* a pool is a [growing] space for objects of the same size */
struct MemPool {
	ACL_MEM_POOL pool;
	MemPoolMeter meter;
#if DEBUG_MEMPOOL
	MemPoolMeter diff_meter;
#endif
};

typedef struct SQUID_MEM_ALLOCATOR {
	ACL_ALLOCATOR allocator;
	MemPoolMeter TheMeter;
	gb_t mem_traffic_volume;
} SQUID_MEM_ALLOCATOR;


/* in tools.h */

void kb_incr(kb_t * k, unsigned int v);
void gb_flush(gb_t * g);
double gb_to_double(const gb_t * g);
const char *gb_to_str(const gb_t * g);

/* in squid_allocator.c */
extern ACL_ALLOCATOR *squid_allocator_create(void);

#ifdef __cplusplus
}
#endif

#endif

