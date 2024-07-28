#include "stdafx.h"
#include "msg.h"
#include "memory.h"
#include "timer_cache.h"

#if 0
static int avl_cmp_fn(const void *v1, const void *v2)
{
	const TIMER_CACHE_NODE *n1 = (const TIMER_CACHE_NODE*) v1;
	const TIMER_CACHE_NODE *n2 = (const TIMER_CACHE_NODE*) v2;
	long long ret = n1->expire - n2->expire;

	if (ret < 0) {
		return -1;
	} else if (ret > 0) {
		return 1;
	} else {
		return 0;
	}
}
#else
static int avl_cmp_fn(const TIMER_CACHE_NODE *n1, const TIMER_CACHE_NODE *n2)
{
	if (n1->expire < n2->expire) {
		return -1;
	} else if (n1->expire > n2->expire) {
		return 1;
	} else {
		return 0;
	}
}
#endif

TIMER_CACHE *timer_cache_create(void)
{
	TIMER_CACHE *cache = mem_malloc(sizeof(TIMER_CACHE));

	fiber_avl_create(&cache->tree,
		(int (*)(const void*, const void*)) avl_cmp_fn,
		sizeof(TIMER_CACHE_NODE), offsetof(TIMER_CACHE_NODE, node));
	ring_init(&cache->caches);
	cache->cache_max = 1000;
	cache->objs = array_create(100, ARRAY_F_UNORDER);
	cache->objs2 = array_create(100, ARRAY_F_UNORDER);

	return cache;
}

unsigned timer_cache_size(TIMER_CACHE *cache)
{
	return fiber_avl_numnodes(&cache->tree);
}

void timer_cache_free(TIMER_CACHE *cache)
{
	TIMER_CACHE_NODE *node;

	while ((node = fiber_avl_first(&cache->tree))) {
		fiber_avl_remove(&cache->tree, node);
		mem_free(node);
	}

	while (1) {
		RING *r = ring_pop_head(&cache->caches);
		if (r == NULL) {
			break;
		}
		node = ring_to_appl(r, TIMER_CACHE_NODE, ring);
		mem_free(node);
	}

	array_free(cache->objs, NULL);
	array_free(cache->objs2, NULL);
	mem_free(cache);
}

void timer_cache_add(TIMER_CACHE *cache, long long expire, RING *entry)
{
	TIMER_CACHE_NODE n, *node;

	n.expire = expire;
	node = fiber_avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		RING *ring = ring_pop_head(&cache->caches);
		if (ring != NULL) {
			node = ring_to_appl(ring, TIMER_CACHE_NODE, ring);
		} else {
			node = mem_malloc(sizeof(TIMER_CACHE_NODE));
		}
		node->expire = expire;
		ring_init(&node->ring);
		fiber_avl_add(&cache->tree, node);
	}

	ring_append(&node->ring, entry);
}

int timer_cache_remove(TIMER_CACHE *cache, long long expire, RING *entry)
{
	TIMER_CACHE_NODE n, *node;
	n.expire = expire;

	node = fiber_avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		return 0;
	}

	if (entry->parent != &node->ring) {
		// Maybe the fiber has been append to the other ring.
		if (ring_size(&node->ring) == 0) {
			timer_cache_free_node(cache, node);
		}
		return 0;
	}

	// Detach the fiber from the current timer node.
	ring_detach(entry);

	if (ring_size(&node->ring) == 0) {
		// If the timer node is empty, just free it now.
		timer_cache_free_node(cache, node);
	}
	return 1;
}

void timer_cache_free_node(TIMER_CACHE *cache, TIMER_CACHE_NODE *node)
{
	// The node will be removed if it hasn't any entry.
	fiber_avl_remove(&cache->tree, node);

	// The node object can be cached for being reused in future.
	if (cache->cache_max > 0 && ring_size(&cache->caches) < cache->cache_max) {
		ring_append(&cache->caches, &node->ring);
	} else {
		mem_free(node);
	}
}

int timer_cache_remove_exist(TIMER_CACHE *cache, long long expire, RING *entry)
{
	//RING_ITER iter;
	TIMER_CACHE_NODE n, *node;

	n.expire = expire;
	node = fiber_avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		return 0;
	}

#if 0
	ring_foreach(iter, &node->ring) {
		if (iter.ptr == entry) {
			ring_detach(entry);
			if (ring_size(&node->ring) == 0) {
				timer_cache_free_node(cache, node);
			}
			return 1;
		}
	}
#else
	if (entry->parent != &node->ring) {
		return 0;
	}

	ring_detach(entry);

	if (ring_size(&node->ring) == 0) {
		timer_cache_free_node(cache, node);
	}
#endif
	return 1;
}

int timer_cache_exist(TIMER_CACHE *cache, long long expire, RING *entry)
{
	TIMER_CACHE_NODE n, *node;
	RING_ITER iter;

	n.expire = expire;
	node = fiber_avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		return 0;
	}

	ring_foreach(iter, &node->ring) {
		if (iter.ptr == entry) {
			return 1;
		}
	}

	return 0;
}
