#include "stdafx.h"
#include "msg.h"
#include "memory.h"
#include "cache.h"

static int avl_cmp_fn(const void *v1, const void *v2)
{
	const CACHE_NODE *n1 = (const CACHE_NODE*) v1;
	const CACHE_NODE *n2 = (const CACHE_NODE*) v2;
	long long ret = n1->expire - n2->expire;

	if (ret < 0) {
		return -1;
	} else if (ret > 0) {
		return 1;
	} else {
		return 0;
	}
}

CACHE *cache_create(void)
{
	CACHE *cache = mem_malloc(sizeof(CACHE));

	avl_create(&cache->tree, avl_cmp_fn, sizeof(CACHE_NODE),
		offsetof(CACHE_NODE, node));
	return cache;
}

unsigned cache_size(CACHE *cache)
{
	return avl_numnodes(&cache->tree);
}

void cache_free(CACHE *cache)
{
	mem_free(cache);
}

void cache_add(CACHE *cache, long long expire, RING *entry)
{
	CACHE_NODE n, *node;

	n.expire = expire;
	node = avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		node = mem_malloc(sizeof(CACHE_NODE));
		node->expire = expire;
		ring_init(&node->ring);
		avl_add(&cache->tree, node);
	}

	ring_append(&node->ring, entry);
}

void cache_remove(CACHE *cache, long long expire, RING *entry)
{
	CACHE_NODE n, *node;
	n.expire = expire;
	node = avl_find(&cache->tree, &n, NULL);
	if (node == NULL) {
		msg_error("not found expire=%lld", expire);
		return;
	}
	ring_detach(entry);
	if (ring_size(&node->ring) == 0) {
		avl_remove(&cache->tree, node);
		mem_free(node);
	}
}

void cache_free_node(CACHE *cache, CACHE_NODE *node)
{
	avl_remove(&cache->tree, node);
	mem_free(node);
}
