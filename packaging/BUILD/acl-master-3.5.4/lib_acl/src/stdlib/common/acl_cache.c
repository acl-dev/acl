#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include "stdlib/acl_htable.h"
#include "stdlib/acl_ring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "thread/acl_pthread.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_cache.h"

#endif

static void *cache_iter_head(ACL_ITER *iter, struct ACL_CACHE *cache)
{
	ACL_CACHE_INFO *ptr;
	ACL_RING *ring;

	iter->dlen = -1;
	iter->i = 0;
	iter->size = cache->size;
	iter->ptr = ring = acl_ring_succ(&cache->ring);

	if (ring != &cache->ring) {
		ptr = ACL_RING_TO_APPL(ring, ACL_CACHE_INFO, entry);
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

static void *cache_iter_next(ACL_ITER *iter, struct ACL_CACHE *cache)
{
	ACL_CACHE_INFO *ptr;
	ACL_RING *ring;

	ring = (ACL_RING*) iter->ptr;
	iter->ptr = ring = acl_ring_succ(ring);
	if (ring != &cache->ring) {
		ptr = ACL_RING_TO_APPL(ring, ACL_CACHE_INFO, entry);
		iter->data = ptr->value;
		iter->key = ptr->key;
		iter->i++;
	} else {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

static void *cache_iter_tail(ACL_ITER *iter, struct ACL_CACHE *cache)
{
	ACL_CACHE_INFO *ptr;
	ACL_RING *ring;

	iter->dlen = -1;
	iter->i = cache->size - 1;
	iter->size = cache->size;
	iter->ptr = ring = acl_ring_pred(&cache->ring);

	if (ring != &cache->ring) {
		ptr = ACL_RING_TO_APPL(ring, ACL_CACHE_INFO, entry);
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

static void *cache_iter_prev(ACL_ITER *iter, struct ACL_CACHE *cache)
{
	ACL_CACHE_INFO *ptr;
	ACL_RING *ring;

	ring = (ACL_RING*) iter->ptr;
	iter->ptr = ring = acl_ring_pred(ring);
	if (ring != &cache->ring) {
		ptr = ACL_RING_TO_APPL(ring, ACL_CACHE_INFO, entry);
		iter->data = ptr->value;
		iter->key = ptr->key;
		iter->i--;
	} else {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

static ACL_CACHE_INFO *cache_iter_info(ACL_ITER *iter, struct ACL_CACHE *cache)
{
	ACL_CACHE_INFO *ptr;

	if (iter->ptr == NULL || iter->ptr == &cache->ring)
		return (NULL);
	ptr = ACL_RING_TO_APPL((ACL_RING*) iter->ptr, ACL_CACHE_INFO, entry);
	return (ptr);
}

ACL_CACHE *acl_cache_create(int max_size, int timeout,
	void (*free_fn)(const ACL_CACHE_INFO*, void*))
{
	const char *myname = "acl_cache_create";
	ACL_CACHE *cache;

	if (max_size <= 0 || timeout <= 0) {
		acl_msg_info("%s(%d): max_size(%d), timeout(%d), no need cache",
			myname, __LINE__, max_size, timeout);
		return (NULL);
	}

	if (free_fn == NULL)
		acl_msg_info("%s(%d), %s: free_fn null",
			__FILE__, __LINE__, myname);

	cache = (ACL_CACHE *) acl_mycalloc(1, sizeof(ACL_CACHE));
	cache->max_size = max_size < 1 ? 1 : max_size;
	cache->timeout = timeout < 0 ? 0 : timeout;
	cache->free_fn = free_fn;
	cache->table = acl_htable_create(100, 0);
	cache->slice = acl_slice_create("acl_cache", 0,
		sizeof(ACL_CACHE_INFO), ACL_SLICE_FLAG_GC1);
	acl_ring_init(&cache->ring);
	acl_pthread_mutex_init(&cache->lock, NULL);

	cache->iter_head = cache_iter_head;
	cache->iter_next = cache_iter_next;
	cache->iter_tail = cache_iter_tail;
	cache->iter_prev = cache_iter_prev;
	cache->iter_info = cache_iter_info;

	return (cache);
}

void acl_cache_free(ACL_CACHE *cache)
{
	const char *myname = "acl_cache_free";
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return;

	while ((info = ACL_RING_FIRST_APPL(&cache->ring, ACL_CACHE_INFO, entry)) != 0) {
		if (info->nrefer > 0) {
			acl_msg_warn("%s(%d), %s: key(%s)'s nrefer(%d) > 0",
				__FILE__, __LINE__, myname, info->key, info->nrefer);
			info->nrefer = 0;  /* force to set 0 */
		}
		(void) acl_cache_delete(cache, info);
	}
	acl_htable_free(cache->table, NULL);
	acl_pthread_mutex_destroy(&cache->lock);
	acl_slice_destroy(cache->slice);
	acl_myfree(cache);
}

ACL_CACHE_INFO *acl_cache_enter(ACL_CACHE *cache, const char *key, void *value)
{
	const char *myname = "acl_cache_enter";
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return (NULL);

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info != NULL) {
		if (info->nrefer > 0) {
			acl_msg_warn("%s(%d), %s: key(%s)'s old's"
				" value's refer(%d) > 0",
				__FILE__, __LINE__, myname, key, info->nrefer);
			return (NULL);
		}
		if (cache->free_fn)
			cache->free_fn(info, info->value);
		info->value = value;
		return (info);
	}

	/* 如果发现缓存池溢出，则优先采用过期策略 */
	if (cache->size >= cache->max_size) {
		(void) acl_cache_timeout(cache);
	}

	/* 如果依然发现缓存池溢出，则采用删除最旧的数据策略 */
	if (cache->size >= cache->max_size) {
		ACL_RING_ITER iter;

		/* 尽量删除一个最老的对象 */
		acl_ring_foreach(iter, &cache->ring) {
			info = ACL_RING_TO_APPL(iter.ptr, ACL_CACHE_INFO, entry);
			if (info->nrefer > 0 || info->when_timeout == 0)
				continue;
			(void) acl_cache_delete(cache, info);
			break;
		}
	}

	/* 如果缓存池还是处于溢出状态，则直接返回不进行任务添加 */
	if (cache->size >= cache->max_size) {
		acl_msg_error("%s(%d), %s: cache->size(%d) >= cache->max_size(%d)"
			", add key(%s) error", __FILE__, __LINE__, myname,
			cache->size, cache->max_size, key);
		return (NULL);
	}

	info = (ACL_CACHE_INFO*) acl_slice_calloc(cache->slice);
	info->key = acl_mystrdup(key);
	if (acl_htable_enter(cache->table, key, (char*) info) == NULL) {
		acl_msg_error("%s(%d), %s: add key(%s) to htable error(%s)",
			__FILE__, __LINE__, myname, key, acl_last_serror());
		acl_slice_free2(cache->slice, info);
		return (NULL);
	}
	cache->size++;

	info->value = value;
	info->when_timeout = cache->timeout > 0 ? (time(NULL) + cache->timeout) : 0;
	/* 将最新的数据添加在过期数据链的尾部, 当由链头向链尾方向遍历时，数据总是
	 * 由最旧的数据向最新的数据开始，即：acl_ring_succ: 旧 --> 新
	 */
	acl_ring_prepend(&cache->ring, &info->entry);
	return (info);
}

void *acl_cache_find(ACL_CACHE *cache, const char *key)
{
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return (NULL);

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info != NULL)
		return (info->value);
	else
		return (NULL);
}

ACL_CACHE_INFO *acl_cache_locate(ACL_CACHE *cache, const char *key)
{
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return (NULL);

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info != NULL)
		return (info);
	else
		return (NULL);
}

int acl_cache_delete(ACL_CACHE *cache, ACL_CACHE_INFO *info)
{
	if (cache == NULL || cache->max_size <= 0)
		return (0);

	if (info->nrefer > 0)
		return (-1);
	if (acl_htable_delete(cache->table, info->key, NULL) < 0)
		return (-1);
	acl_ring_detach(&info->entry);
	if (cache->free_fn)
		cache->free_fn(info, info->value);
	acl_myfree(info->key);
	acl_slice_free2(cache->slice, info);
	cache->size--;
	return (0);
}

int acl_cache_delete2(ACL_CACHE *cache, const char *key)
{
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return (0);

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info == NULL)
		return (-1);
	if (info->nrefer > 0)
		return (-1);
	return (acl_cache_delete(cache, info));
}

int acl_cache_timeout(ACL_CACHE *cache)
{
	ACL_CACHE_INFO *info;
	time_t now = time(NULL);
	ACL_RING *iter, *iter_next;
	int   n = 0;

	if (cache == NULL || cache->max_size <= 0)
		return (n);

	/* 因为数据链的添加是尾部添加的，所以从首部至尾部数据依次由旧变新 */
	for (iter = acl_ring_succ(&cache->ring); iter != &cache->ring;) {
		info = ACL_RING_TO_APPL(iter, ACL_CACHE_INFO, entry);
		if (info->when_timeout > now)
			break;
		if (info->nrefer > 0) {
			iter = acl_ring_succ(iter);
			continue;
		}
		if (info->when_timeout == 0) {
			iter = acl_ring_succ(iter);
			continue;
		}
		iter_next = acl_ring_succ(iter);
		if (acl_cache_delete(cache, info) == 0)
			n++;
		iter = iter_next;
	}
	return (n);
}

void acl_cache_update2(ACL_CACHE *cache, ACL_CACHE_INFO *info, int timeout)
{
	if (cache == NULL || cache->max_size <= 0)
		return;

	acl_ring_detach(&info->entry);
	info->when_timeout = timeout > 0 ? (time(NULL) + timeout) : 0;
	/* 将最新的数据添加在过期数据链的尾部, 当由链头向链尾方向遍历时，数据总是
	 * 由最旧的数据向最新的数据开始，即：acl_ring_succ: 旧 --> 新
	 */
	acl_ring_prepend(&cache->ring, &info->entry);
}

void acl_cache_update(ACL_CACHE *cache, const char *key, int timeout)
{
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return;

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info)
		acl_cache_update2(cache, info, timeout);
}

void acl_cache_refer(ACL_CACHE_INFO *info)
{
	info->nrefer++;
}

void acl_cache_refer2(ACL_CACHE *cache, const char *key)
{
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return;

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info == NULL)
		return;
	info->nrefer++;
}

void acl_cache_unrefer2(ACL_CACHE *cache, const char *key)
{
	ACL_CACHE_INFO *info;

	if (cache == NULL || cache->max_size <= 0)
		return;

	info = (ACL_CACHE_INFO*) acl_htable_find(cache->table, key);
	if (info == NULL)
		return;
	info->nrefer--;
}

void acl_cache_unrefer(ACL_CACHE_INFO *info)
{
	const char *myname = "acl_cache_unrefer";
	
	info->nrefer--;
	if (info->nrefer < 0)
		acl_msg_warn("%s(%d), %s: key(%s)'s nrefer(%d) invalid",
			__FILE__, __LINE__, myname, info->key, info->nrefer);
}

void acl_cache_lock(ACL_CACHE *cache)
{
	if (cache == NULL || cache->max_size <= 0)
		return;
	acl_pthread_mutex_lock(&cache->lock);
}

void acl_cache_unlock(ACL_CACHE *cache)
{
	if (cache == NULL || cache->max_size <= 0)
		return;
	acl_pthread_mutex_unlock(&cache->lock);
}

void acl_cache_walk(ACL_CACHE *cache, void (*walk_fn)(ACL_CACHE_INFO*, void*), void *arg)
{
	ACL_CACHE_INFO *info;
	ACL_RING_ITER iter;

	if (cache == NULL || cache->max_size <= 0)
		return;
	acl_ring_foreach(iter, &cache->ring) {
		info = ACL_RING_TO_APPL(iter.ptr, ACL_CACHE_INFO, entry);
		walk_fn(info, arg);
	}
}

int acl_cache_clean(ACL_CACHE *cache, int force)
{
	const char *myname = "acl_cache_clean";
	ACL_CACHE_INFO *info;
	ACL_RING *iter, *iter_next;
	int   n = 0;

	if (cache == NULL || cache->max_size <= 0)
		return (0);

	for (iter = acl_ring_succ(&cache->ring); iter != &cache->ring;) {
		info = ACL_RING_TO_APPL(iter, ACL_CACHE_INFO, entry);
		if (info->nrefer > 0) {
			if (force == 0) {
				iter = acl_ring_succ(iter);
				continue;
			} else {
				acl_msg_warn("%s(%d), %s: key(%s)'s refer(%d) > 0",
					__FILE__, __LINE__, myname,
					info->key, info->nrefer);
				info->nrefer = 0;  /* force set 0 */
			}
		}
		iter_next = acl_ring_succ(iter);
		if (acl_cache_delete(cache, info) == 0)
			n++;
		iter = iter_next;
	}

	return (n);
}

int acl_cache_size(ACL_CACHE *cache)
{
	if (cache == NULL || cache->max_size <= 0)
		return (0);
	return (cache->size);
}
