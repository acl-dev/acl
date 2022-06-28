#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_ring.h"
#include "event/acl_events.h"

#endif

#include "events.h"

struct EVENT_TIMERS {
	ACL_HTABLE *table;		/**< 哈希表用于按键值查询      */
	avl_tree_t  avl;		/**< 用于按时间排序的平衡二叉树 */
};

typedef struct TIMER_INFO TIMER_INFO;
typedef struct TIMER_NODE TIMER_NODE;

/* 每个元素的内部对象，所有元素连接在一起，同时表明该元素所属的树节点 */
struct TIMER_INFO {
	TIMER_NODE *node;
	TIMER_INFO *prev;
	TIMER_INFO *next;
	ACL_HTABLE_INFO *entry;

	ACL_EVENT_NOTIFY_TIME callback; /* callback function      */
	void *context;                  /* callback context       */
	int   event_type;		/* event type             */
	acl_int64 delay;
	int   keep;
	ACL_RING tmp;
};

/* 具有相同过期时间截的元素存放里该树节点上 */
struct TIMER_NODE {
	acl_int64   when;
	avl_node_t  node;
	TIMER_INFO *head;
	TIMER_INFO *tail;
};

#define BUILD_KEY(x1, x2) \
	char key[128];    \
	snprintf(key, sizeof(key), "%p.%p", x1, x2);

/**
 * AVL 用的比较回调函数
 */
static int avl_cmp_fn(const void *v1, const void *v2)
{
	const struct TIMER_NODE *n1 = (const struct TIMER_NODE*) v1;
	const struct TIMER_NODE *n2 = (const struct TIMER_NODE*) v2;
	acl_int64 ret = n1->when - n2->when;

	if (ret < 0) {
		return -1;
	} else if (ret > 0) {
		return 1;
	} else {
		return 0;
	}
}

void event_timer_create(ACL_EVENT *eventp)
{
	eventp->timers = (EVENT_TIMERS*) acl_mymalloc(sizeof(EVENT_TIMERS));
	eventp->timers->table = acl_htable_create(1024, 0);
	avl_create(&eventp->timers->avl, avl_cmp_fn, sizeof(TIMER_INFO),
		   offsetof(TIMER_NODE, node));
}

acl_int64 event_timer_when(ACL_EVENT *eventp)
{
	TIMER_NODE *node = avl_first(&eventp->timers->avl);
	return node ? node->when : -1;
}

void event_timer_free(ACL_EVENT *eventp)
{
	TIMER_NODE *node, *next;

	node = (TIMER_NODE*) avl_first(&eventp->timers->avl);
	while (node) {
		next = AVL_NEXT(&eventp->timers->avl, node);
		acl_myfree(node);
		node = next;
	}

	acl_htable_free(eventp->timers->table, acl_myfree_fn);
	acl_myfree(eventp->timers);
}

/* event_timer_request - (re)set timer */

static void node_link(TIMER_NODE *node, TIMER_INFO *info)
{
	if (node->tail == NULL) {
		info->prev = info->next = NULL;
		node->head = node->tail = info;
	} else {
		node->tail->next = info;
		info->prev = node->tail;
		info->next = NULL;
		node->tail = info;
	}
	info->node = node;
}

/* return 1 if the info's node has been freed*/
static int node_unlink(ACL_EVENT *eventp, TIMER_INFO *info)
{
	TIMER_NODE *node = info->node;

	acl_assert(node);
	if (info->prev) {
		info->prev->next = info->next;
	} else {
		node->head = info->next;
	}
	if (info->next) {
		info->next->prev = info->prev;
	} else {
		node->tail = info->prev;
	}
	info->node = NULL;

	if (node->head == NULL) {
		avl_remove(&eventp->timers->avl, node);
		acl_myfree(node);
		return 1;
	}
	return 0;
}

acl_int64 event_timer_request(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, acl_int64 delay, int keep)
{
	TIMER_INFO *info;
	TIMER_NODE *node, iter;
	BUILD_KEY(callback, context);

	/* Make sure we schedule this event at the right time. */
	SET_TIME(eventp->present);

	/**
	 * See if they are resetting an existing timer request. If so, take the
	 * request away from the timer queue so that it can be inserted at the
	 * right place.
	 */

	info = (TIMER_INFO*) acl_htable_find(eventp->timers->table, key);
	if (info == NULL) {
		/* If not found, schedule a new timer request. */
		info = (TIMER_INFO *) acl_mycalloc(1, sizeof(TIMER_INFO));
		acl_assert(info);
		info->delay      = delay;
		info->keep       = keep;
		info->callback   = callback;
		info->context    = context;
		info->event_type = ACL_EVENT_TIME;
		acl_ring_init(&info->tmp);
		info->entry = acl_htable_enter(eventp->timers->table, key, info);
	} else {
		info->delay = delay;
		info->keep  = keep;
		node_unlink(eventp, info);
	}

	iter.when = eventp->present + delay;
	node = (TIMER_NODE*) avl_find(&eventp->timers->avl, &iter, NULL);
	if (node == NULL) {
		node = (TIMER_NODE*) acl_mycalloc(1, sizeof(TIMER_NODE));
		node->when = iter.when;
		/**
		 * Insert the request at the right place. Timer requests are
		 * kept sorted to reduce lookup overhead in the event loop.
		 */
		avl_add(&eventp->timers->avl, node);
	}

	node_link(node, info);
	return node->when;
}

/* event_timer_cancel - cancel timer */

static acl_int64 timer_cancel(ACL_EVENT *eventp, TIMER_INFO *info)
{
	acl_int64   time_left = -1;
	TIMER_NODE *first, *node;

	/**
	 * See if they are canceling an existing timer request. Do not complain
	 * when the request is not found. It might have been canceled from some
	 * other thread.
	 */

	SET_TIME(eventp->present);

	acl_htable_delete_entry(eventp->timers->table, info->entry, NULL);
	acl_assert(info->node);

	node = info->node;
	first = avl_first(&eventp->timers->avl);
	if (first == node) {
		first = AVL_NEXT(&eventp->timers->avl, first);
		if (node_unlink(eventp, info) == 0) {
			first = node;
		}
	} else {
		node_unlink(eventp, info);
	}

	if (first) {
		time_left = first->when - eventp->present;
		if (time_left < 0) {
			time_left = 0;
		}
	}

	acl_ring_detach(&info->tmp);
	acl_myfree(info);

	return time_left;
}

acl_int64 event_timer_cancel(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	TIMER_INFO *info;
	BUILD_KEY(callback, context);

	info = (TIMER_INFO*) acl_htable_find(eventp->timers->table, key);
	if (info == NULL) {
		return -1;
	}

	return timer_cancel(eventp, info);
}

void event_timer_keep(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int keep)
{
	TIMER_INFO *info;
	BUILD_KEY(callback, context);

	info = (TIMER_INFO*) acl_htable_find(eventp->timers->table, key);
	if (info) {
		info->keep = keep;
	}
}

int  event_timer_ifkeep(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context)
{
	TIMER_INFO *info;
	BUILD_KEY(callback, context);

	info = (TIMER_INFO*) acl_htable_find(eventp->timers->table, key);
	return info ? info->keep : 0;
}

void event_timer_trigger(ACL_EVENT *eventp)
{
	ACL_RING *ring;
	ACL_EVENT_NOTIFY_TIME timer_fn;
	void *timer_arg;
	TIMER_NODE *iter;
	TIMER_INFO *info;
	int n = 0;

	SET_TIME(eventp->present);

	/* collect all the timers that should be triggered */
	iter = avl_first(&eventp->timers->avl);
	while (iter) {
		if (iter->when > eventp->present) {
			break;
		}
		info = iter->head;
		while (info) {
			acl_ring_prepend(&eventp->timers_ready, &info->tmp);
			info = info->next;
			n++;
		}
		iter = AVL_NEXT(&eventp->timers->avl, iter);
	}

#define TMP_TO_INFO(r) \
	((TIMER_INFO *) ((char *) (r) - offsetof(TIMER_INFO, tmp)))

	while ((ring = acl_ring_pop_head(&eventp->timers_ready)) != NULL) {
		info      = TMP_TO_INFO(ring);
		timer_fn  = info->callback;
		timer_arg = info->context;

		if (info->delay > 0 && info->keep) {
			acl_ring_detach(&info->tmp);
			event_timer_request(eventp, timer_fn,
				timer_arg, info->delay, info->keep);
		} else {
			timer_cancel(eventp, info);
		}

		timer_fn(ACL_EVENT_TIME, eventp, timer_arg);
	}
}
