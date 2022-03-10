#include "stdafx.h"
#include "msg.h"
#include "memory.h"
#include "timer_cache.h"

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

TIMER_CACHE *timer_cache_create(void)
{
	TIMER_CACHE *cache = mem_malloc(sizeof(TIMER_CACHE));

	avl_create(&cache->tree, avl_cmp_fn, sizeof(TIMER_CACHE_NODE),
		offsetof(TIMER_CACHE_NODE, node));
	ring_init(&cache->caches);
	cache->cache_max = 1000;

	return cache;
}

unsigned timer_cache_size(TIMER_CACHE *cache)
{
	return avl_numnodes(&cache->tree);
}

void timer_cache_free(TIMER_CACHE *cache)
{
	TIMER_CACHE_NODE *node = avl_first(&cache->tree), *next;

	while (node != NULL) {
		next = AVL_NEXT(&cache->tree, node);
		mem_free(node);
		node = next;
	}

	while (1) {
		RING *r = ring_pop_head(&cache->caches);
		if (r == NULL) {
			break;
		}
		node = ring_to_appl(r, TIMER_CACHE_NODE, ring);
		mem_free(node);
	}

	mem_free(cache);
}

void timer_cache_add(TIMER_CACHE *cache, long long expire, RING *entry)
{
	TIMER_CACHE_NODE n, *node;

	n.expire = expire;
	node = avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		RING *ring = ring_pop_head(&cache->caches);
		if (ring != NULL) {
			node = ring_to_appl(ring, TIMER_CACHE_NODE, ring);
		} else {
			node = mem_malloc(sizeof(TIMER_CACHE_NODE));
		}
		node->expire = expire;
		ring_init(&node->ring);
		avl_add(&cache->tree, node);
	}

	ring_append(&node->ring, entry);
}

void timer_cache_remove(TIMER_CACHE *cache, long long expire, RING *entry)
{
	TIMER_CACHE_NODE n, *node;
	n.expire = expire;
	node = avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		msg_error("not found expire=%lld", expire);
		return;
	}

	ring_detach(entry);

	if (ring_size(&node->ring) == 0) {
		timer_cache_free_node(cache, node);
	}
}

void timer_cache_free_node(TIMER_CACHE *cache, TIMER_CACHE_NODE *node)
{
	// The node will be removed if it hasn't any entry.
	avl_remove(&cache->tree, node);

	// The node object can be cached for being reused in future.
	if (cache->cache_max > 0 && ring_size(&cache->caches) < cache->cache_max) {
		ring_append(&cache->caches, &node->ring);
	} else {
		mem_free(node);
	}
}
