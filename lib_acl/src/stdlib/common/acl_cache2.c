#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <time.h>
#include "stdlib/acl_htable.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/avl.h"
#include "stdlib/acl_cache2.h"

#endif

typedef struct {
	ACL_CACHE2  cache;		/**< 封装了 ACL_CACHE2 */
	ACL_HTABLE *table;      	/**< 哈希表 */
	avl_tree_t  avl;		/**< 用于按时间排序的平稳二叉树 */
	acl_pthread_mutex_t lock;       /**< 缓存池锁 */
} CACHE;

typedef struct CACHE_INFO CACHE_INFO;

typedef struct TREE_NODE {
	CACHE_INFO *head;
	CACHE_INFO *tail;
	avl_node_t node;
	time_t when_timeout;
} TREE_NODE;

struct CACHE_INFO {
	ACL_CACHE2_INFO info;
	TREE_NODE *tree_node;
	CACHE_INFO *prev;
	CACHE_INFO *next;
};

/**
 * AVL 用的比较回调函数
 */
static int cmp_fn(const void *v1, const void *v2)
{
	const TREE_NODE *n1 = (const TREE_NODE*) v1;
	const TREE_NODE *n2 = (const TREE_NODE*) v2;
	time_t ret = n1->when_timeout - n2->when_timeout;

	if (ret < 0)
		return (-1);
	else if (ret > 0)
		return (1);
	else
		return (0);
}

static void *cache_iter_head(ACL_ITER *iter, struct ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	CACHE_INFO *info;
	TREE_NODE *pnode;

	iter->dlen = -1;
	iter->i = 0;
	iter->size = cache2->size;

	pnode = avl_first(&cache->avl);
	if (pnode == NULL) {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
		return (iter->ptr);
	}

	iter->ptr = info = pnode->head;
	acl_assert(info);
	iter->data = ((ACL_CACHE2_INFO *) info)->value;
	iter->key = ((ACL_CACHE2_INFO *) info)->key;
	return (iter->ptr);
}

static void *cache_iter_next(ACL_ITER *iter, struct ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	CACHE_INFO *info = (CACHE_INFO*) iter->ptr;
	TREE_NODE *pnode = info->tree_node;

	info = info->next;
	if (info) {
		iter->ptr = info;
		iter->data = ((ACL_CACHE2_INFO *) info)->value;
		iter->key = ((ACL_CACHE2_INFO *) info)->key;
		iter->i++;
		return (iter->ptr);
	}

	pnode = AVL_NEXT(&cache->avl, pnode);
	if (pnode == NULL) {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
		return (iter->ptr);
	}

	iter->ptr = info = pnode->head;
	acl_assert(info);
	iter->data = ((ACL_CACHE2_INFO *) info)->value;
	iter->key = ((ACL_CACHE2_INFO *) info)->key;
	iter->i++;
	return (iter->ptr);
}

static void *cache_iter_tail(ACL_ITER *iter, struct ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	CACHE_INFO *info;
	TREE_NODE *pnode;

	iter->dlen = -1;
	iter->i = cache2->size - 1;
	iter->size = cache2->size;

	pnode = avl_last(&cache->avl);
	if (pnode == NULL) {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
		return (iter->ptr);
	}

	iter->ptr = info = pnode->tail;
	acl_assert(info);
	iter->data = ((ACL_CACHE2_INFO *) info)->value;
	iter->key = ((ACL_CACHE2_INFO *) info)->key;
	return (iter->ptr);
}

static void *cache_iter_prev(ACL_ITER *iter, struct ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	CACHE_INFO *info = (CACHE_INFO*) iter->ptr;
	TREE_NODE *pnode = info->tree_node;

	info = info->prev;
	if (info) {
		iter->ptr = info;
		iter->data = ((ACL_CACHE2_INFO *) info)->value;
		iter->key = ((ACL_CACHE2_INFO *) info)->key;
		iter->i++;
		return (iter->ptr);
	}

	pnode = AVL_PREV(&cache->avl, pnode);
	if (pnode == NULL) {
		iter->ptr = NULL;
		iter->data = NULL;
		iter->key = NULL;
		return (iter->ptr);
	}

	iter->ptr = info = pnode->tail;
	acl_assert(info);
	iter->data = ((ACL_CACHE2_INFO *) info)->value;
	iter->key = ((ACL_CACHE2_INFO *) info)->key;
	iter->i++;
	return (iter->ptr);
}

static ACL_CACHE2_INFO *cache_iter_info(ACL_ITER *iter, struct ACL_CACHE2 *cache2 acl_unused)
{
	if (iter->ptr == NULL)
		return (NULL);
	return ((ACL_CACHE2_INFO*) iter->ptr);
}

ACL_CACHE2 *acl_cache2_create(int max_size,
	void (*free_fn)(const ACL_CACHE2_INFO*, void*))
{       
	const char *myname = "acl_cache2_create";
	ACL_CACHE2 *cache2;
	CACHE *cache;

	if (max_size <= 0) {
		acl_msg_info("%s(%d): max_size(%d), no need cache",
			myname, __LINE__, max_size);
		return (NULL);
	}

	if (free_fn == NULL)
		acl_msg_info("%s(%d), %s: free_fn null",
			__FILE__, __LINE__, myname);

	cache = (CACHE *) acl_mycalloc(1, sizeof(CACHE));
	cache->table = acl_htable_create(100, 0);
	avl_create(&cache->avl, cmp_fn, sizeof(TREE_NODE), offsetof(TREE_NODE, node));
	acl_pthread_mutex_init(&cache->lock, NULL);

	cache2 = (ACL_CACHE2*) cache;
	cache2->max_size = max_size < 1 ? 1 : max_size;
	cache2->free_fn = free_fn;
	cache2->iter_head = cache_iter_head;
	cache2->iter_next = cache_iter_next;
	cache2->iter_tail = cache_iter_tail;
	cache2->iter_prev = cache_iter_prev;
	cache2->iter_info = cache_iter_info;
	return (cache2);
}

void acl_cache2_free(ACL_CACHE2 *cache2)
{
	const char *myname = "acl_cache2_free";
	CACHE *cache = (CACHE*) cache2;
	TREE_NODE *pnode;
	ACL_CACHE2_INFO *info2;
	CACHE_INFO *info;

	if (cache == NULL)
		return;

	pnode = (TREE_NODE*) avl_first(&cache->avl);
	while (pnode) {
		info = pnode->head;
		pnode = AVL_NEXT(&cache->avl, pnode);
		while (info) {
			info2 = (ACL_CACHE2_INFO*) info;
			info = info->next;
			if (info2->nrefer > 0) {
				acl_msg_warn("%s(%d): key(%s)'s nrefer(%d) > 0",
					myname, __LINE__, info2->key, info2->nrefer);
				info2->nrefer = 0;  /* force to set 0 */
			}
			(void) acl_cache2_delete(cache2, info2);
		}
	}       

	avl_destroy(&cache->avl);
	acl_htable_free(cache->table, NULL);
	acl_pthread_mutex_destroy(&cache->lock);
	acl_myfree(cache);
}

ACL_CACHE2_INFO *acl_cache2_enter(ACL_CACHE2 *cache2,
	const char *key, void *value, int timeout)
{
	const char *myname = "acl_cache2_enter";
	CACHE *cache = (CACHE *) cache2;
	ACL_CACHE2_INFO *info2;
	CACHE_INFO *info;
	TREE_NODE *pnode, node;
	time_t when_timeout = time(NULL) + timeout;

	if (cache == NULL)
		return (NULL);

	info2 = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info2 != NULL) {
		if (info2->nrefer > 0) {
			acl_msg_warn("%s(%d): key(%s)'s old's"
				" value's refer(%d) > 0",
				myname, __LINE__, key, info2->nrefer);
			return (NULL);
		}
		if (cache2->free_fn)
			cache2->free_fn(info2, info2->value);
		info2->value = value;
		return (info2);
	}

	/* 如果发现缓存池溢出，则优先采用过期策略 */
	if (cache2->size >= cache2->max_size) {
		(void) acl_cache2_timeout(cache2);
	}

	/* 如果依然发现缓存池溢出，则采用删除最旧的数据策略 */
	if (cache2->size >= cache2->max_size) {
		pnode = (TREE_NODE*) avl_first(&cache->avl);
		while (pnode) {
			ACL_CACHE2_INFO *tmp = NULL;
			if (pnode->when_timeout == 0) {
				pnode = AVL_NEXT(&cache->avl, pnode);
				continue;
			}
			info = pnode->head;
			pnode = AVL_NEXT(&cache->avl, pnode);
			while (info) {
				info2 = (ACL_CACHE2_INFO*) info;
				info = info->next;
				if (info2->nrefer == 0) {
					tmp = info2;
					break;
				}
			}
			if (tmp != NULL) {
				(void) acl_cache2_delete(cache2, tmp);
				break;
			}
		}
	}

	/* 如果缓存池还是处于溢出状态，则直接返回不进行任务添加 */
	if (cache2->size >= cache2->max_size) {
		acl_msg_error("%s(%d): cache->size(%d) >= cache->max_size(%d)"
			", add key(%s) error", myname, __LINE__,
			cache2->size, cache2->max_size, key);
		return (NULL);
	}

	node.when_timeout = timeout > 0 ? when_timeout : 0;
	pnode = (TREE_NODE*) avl_find(&cache->avl, &node, NULL);
	if (pnode == NULL) {
		pnode = (TREE_NODE*) acl_mycalloc(1, sizeof(TREE_NODE));
		pnode->when_timeout = node.when_timeout;
		avl_add(&cache->avl, pnode);
	}

	info = (CACHE_INFO*) acl_mycalloc(1, sizeof(CACHE_INFO));
	info2 = (ACL_CACHE2_INFO*) info;

	info2->value = value;
	info2->key = acl_mystrdup(key);
	if (acl_htable_enter(cache->table, key, (char*) info2) == NULL) {
		acl_msg_fatal("%s(%d): add key(%s) to htable error(%s)",
			myname, __LINE__, key, acl_last_serror());
	}

	if (pnode->tail == NULL) {
		info->prev = info->next = NULL;
		pnode->head = pnode->tail = info;
	} else {
		pnode->tail->next = info;
		info->prev = pnode->tail;
		info->next = NULL;
		pnode->tail = info;
	}
	info->tree_node = pnode;
	info2->when_timeout = pnode->when_timeout;

	cache2->size++;
	return (info2);
}

void *acl_cache2_find(ACL_CACHE2 *cache2, const char *key)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info;

	if (cache2 == NULL || cache2->max_size <= 0)
		return (NULL);

	info = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info != NULL)
		return (info->value);
	else
		return (NULL);
}

ACL_CACHE2_INFO *acl_cache2_locate(ACL_CACHE2 *cache2, const char *key)
{
	ACL_CACHE2_INFO *info;
	CACHE *cache = (CACHE*) cache2;

	if (cache2 == NULL || cache2->max_size <= 0)
		return (NULL);

	info = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info != NULL)
		return (info);
	else
		return (NULL);
}

int acl_cache2_delete(ACL_CACHE2 *cache2, ACL_CACHE2_INFO *info2)
{
	CACHE_INFO *info = (CACHE_INFO*) info2;
	TREE_NODE *pnode = info->tree_node;
	CACHE *cache = (CACHE*) cache2;

	if (cache2 == NULL || cache2->max_size <= 0)
		return (0);

	if (info2->nrefer > 0)
		return (-1);
	if (acl_htable_delete(cache->table, info2->key, NULL) < 0)
		return (-1);

	if (info->prev)
		info->prev->next = info->next;
	else    
		pnode->head = info->next;
	if (info->next)
		info->next->prev = info->prev;
	else    
		pnode->tail = info->prev;

	if (cache2->free_fn)
		cache2->free_fn(info2, info2->value);
	acl_myfree(info2->key);
	acl_myfree(info2);
	cache2->size--;

	if (pnode->head == NULL) {
		avl_remove(&cache->avl, pnode);
		acl_myfree(pnode);
	}
	return (0);
}

int acl_cache2_delete2(ACL_CACHE2 *cache2, const char *key)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;

	if (cache2 == NULL || cache2->max_size <= 0)
		return (0);

	info2 = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info2 == NULL)
		return (-1);
	if (info2->nrefer > 0)
		return (-1);
	return (acl_cache2_delete(cache2, info2));
}

int acl_cache2_timeout(ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;
	CACHE_INFO *info;
	TREE_NODE *pnode, *pnode_next;
	time_t now = time(NULL);
	int   n = 0;

	if (cache2 == NULL || cache2->max_size <= 0)
		return (n);

	pnode = (TREE_NODE*) avl_first(&cache->avl);
	while (1) {
		if (pnode == NULL || pnode->when_timeout > now)
			break;
		if (pnode->when_timeout == 0) {
			pnode = AVL_NEXT(&cache->avl, pnode);
			continue;
		}
		pnode_next = AVL_NEXT(&cache->avl, pnode);
		info = pnode->head;
		while (info) {
			info2 = (ACL_CACHE2_INFO*) info;
			info = info->next;
			if (info2->nrefer > 0)
				continue;
			if (acl_cache2_delete(cache2, info2) == 0)
				n++;
		}
		pnode = pnode_next;
	}
	return (n);
}

void acl_cache2_update2(ACL_CACHE2 *cache2, ACL_CACHE2_INFO *info2, int timeout)
{
	CACHE *cache = (CACHE*) cache2;
	CACHE_INFO *info = (CACHE_INFO*) info2;
	TREE_NODE *pnode = info->tree_node, node;

	if (cache2 == NULL || cache2->max_size <= 0)
		return;

	if (info->prev) 
		info->prev->next = info->next;
	else    
		pnode->head = info->next;
	if (info->next)
		info->next->prev = info->prev;
	else
		pnode->tail = info->prev;

	if (pnode->head == NULL) {
		avl_remove(&cache->avl, pnode);
		acl_myfree(pnode);
	}

	node.when_timeout = timeout > 0 ? timeout : 0;
	pnode = (TREE_NODE*) avl_find(&cache->avl, &node, NULL);
	if (pnode == NULL) {
		pnode = (TREE_NODE*) acl_mycalloc(1, sizeof(TREE_NODE));
		pnode->when_timeout = node.when_timeout;
		avl_add(&cache->avl, pnode);
	}

	if (pnode->tail == NULL) {
		info->prev = info->next = NULL;
		pnode->head = pnode->tail = info;
	} else {
		pnode->tail->next = info;
		info->prev = pnode->tail;
		info->next = NULL;
		pnode->tail = info;
	}
	info->tree_node = pnode;
	info2->when_timeout = pnode->when_timeout;
}

void acl_cache2_update(ACL_CACHE2 *cache2, const char *key, int timeout)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;

	if (cache2 == NULL || cache2->max_size <= 0)
		return;

	info2 = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info2)
		acl_cache2_update2(cache2, info2, timeout);
}

void acl_cache2_refer(ACL_CACHE2_INFO *info2)
{
	info2->nrefer++;
}

void acl_cache2_refer2(ACL_CACHE2 *cache2, const char *key)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;

	if (cache2 == NULL || cache2->max_size <= 0)
		return;

	info2 = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info2 == NULL)
		return;
	info2->nrefer++;
}

void acl_cache2_unrefer2(ACL_CACHE2 *cache2, const char *key)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;

	if (cache2 == NULL || cache2->max_size <= 0)
		return;

	info2 = (ACL_CACHE2_INFO*) acl_htable_find(cache->table, key);
	if (info2 == NULL)
		return;
	info2->nrefer--;
}

void acl_cache2_unrefer(ACL_CACHE2_INFO *info2)
{
	const char *myname = "acl_cache2_unrefer";
	
	info2->nrefer--;
	if (info2->nrefer < 0)
		acl_msg_warn("%s(%d): key(%s)'s nrefer(%d) invalid",
			myname, __LINE__, info2->key, info2->nrefer);
}

void acl_cache2_lock(ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	if (cache2 == NULL || cache2->max_size <= 0)
		return;
	acl_pthread_mutex_lock(&cache->lock);
}

void acl_cache2_unlock(ACL_CACHE2 *cache2)
{
	CACHE *cache = (CACHE*) cache2;
	
	if (cache2 == NULL || cache2->max_size <= 0)
		return;
	acl_pthread_mutex_unlock(&cache->lock);
}

void acl_cache2_walk(ACL_CACHE2 *cache2,
	void (*walk_fn)(ACL_CACHE2_INFO*, void*), void *arg)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;
	CACHE_INFO *info;
	TREE_NODE *pnode;

	if (cache2 == NULL || cache2->max_size <= 0)
		return;

	pnode = (TREE_NODE*) avl_first(&cache->avl);
	while (1) {
		if (pnode == NULL)
			break;
		info = pnode->head;
		while (info) {
			info2 = (ACL_CACHE2_INFO*) info;
			walk_fn(info2, arg);
			info = info->next;
		}
		pnode = AVL_NEXT(&cache->avl, pnode);
	}
}

int acl_cache2_clean(ACL_CACHE2 *cache2, int force)
{
	CACHE *cache = (CACHE*) cache2;
	ACL_CACHE2_INFO *info2;
	CACHE_INFO *info;
	TREE_NODE *pnode;
	int   n = 0;

	if (cache2 == NULL || cache2->max_size <= 0)
		return (0);

	pnode = (TREE_NODE*) avl_first(&cache->avl);
	while (pnode) {
		info = pnode->head;
		pnode = AVL_NEXT(&cache->avl, pnode);
		while (info) {
			info2 = (ACL_CACHE2_INFO*) info;
			info = info->next;
			if (info2->nrefer > 0 && force == 0)
				continue;
			if (acl_cache2_delete(cache2, info2) == 0)
				n++;
		}
	}
	return (n);
}

int acl_cache2_size(ACL_CACHE2 *cache2)
{
	if (cache2 == NULL || cache2->max_size <= 0)
		return (0);
	return (cache2->size);
}
