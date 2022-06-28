#ifndef TIMER_CACHE_INCLUDE_H
#define TIMER_CACHE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ring.h"
#include "avl.h"

typedef struct TIMER_CACHE_NODE TIMER_CACHE_NODE;
typedef struct TIMER_CACHE TIMER_CACHE;

struct TIMER_CACHE_NODE {
	RING ring;
	avl_node_t node;
	long long expire;
};

struct TIMER_CACHE {
	avl_tree_t tree;
	RING caches;		// caching the TIMER_CACHE_NODE memory
	int cache_max;
};

TIMER_CACHE *timer_cache_create(void);
unsigned timer_cache_size(TIMER_CACHE *cache);
void timer_cache_free(TIMER_CACHE *cache);
void timer_cache_add(TIMER_CACHE *cache, long long expire, RING *entry);
void timer_cache_remove(TIMER_CACHE *cache, long long expire, RING *entry);
void timer_cache_free_node(TIMER_CACHE *cache, TIMER_CACHE_NODE *node);

#ifdef __cplusplus
}
#endif

#endif
