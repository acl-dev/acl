#ifndef CACHE_INCLUDE_H
#define CACHE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ring.h"
#include "avl.h"

typedef struct CACHE_NODE CACHE_NODE;
typedef struct CACHE CACHE;

struct CACHE_NODE {
	RING ring;
	avl_node_t node;
	long long expire;
};

struct CACHE {
	avl_tree_t tree;
};

CACHE *cache_create(void);
unsigned cache_size(CACHE *cache);
void cache_free(CACHE *cache);
void cache_add(CACHE *cache, long long expire, RING *entry);
void cache_remove(CACHE *cache, long long expire, RING *entry);
void cache_free_node(CACHE *cache, CACHE_NODE *node);

#ifdef __cplusplus
}
#endif

#endif
