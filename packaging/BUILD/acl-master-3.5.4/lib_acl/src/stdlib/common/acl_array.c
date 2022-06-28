#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_dbuf_pool.h"
#include "stdlib/acl_array.h"

static void array_push_back(struct ACL_ARRAY *a, void *obj)
{
	acl_array_append(a, obj);
}

static void array_push_front(struct ACL_ARRAY *a, void *obj)
{
	acl_array_prepend(a, obj);
}

static void *array_pop_back(struct ACL_ARRAY *a)
{
	void *obj;
	if (a->count <= 0)
		return NULL;
	a->count--;
	obj = a->items[a->count];
	return obj;
}

static void *array_pop_front(struct ACL_ARRAY *a)
{
	void *obj;
	int   i;

	if (a->count <= 0)
		return NULL;
	obj = a->items[0];
	a->count--;
	for (i = 0; i < a->count; i++)
		a->items[i] = a->items[i + 1];

	return obj;
}

/* array_iter_head - get the head of the array */

static void *array_iter_head(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = 0;
	iter->size = a->count;
	if (a->items == NULL || a->count <= 0)
		iter->ptr = iter->data = 0;
	else
		iter->ptr = iter->data = a->items[0];

	return iter->ptr;
}

/* array_iter_next - get the next of the array */

static void *array_iter_next(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->i++;
	if (iter->i >= a->count)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = a->items[iter->i];
	return iter->ptr;
}
 
/* array_iter_tail - get the tail of the array */

static void *array_iter_tail(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = a->count - 1;
	iter->size = a->count;
	if (a->items == NULL || iter->i < 0)
		iter->ptr = iter->data = 0;
	else
		iter->data = iter->ptr = a->items[iter->i];
	return iter->ptr;
}

/* array_iter_prev - get the prev of the array */

static void *array_iter_prev(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->i--;
	if (iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = a->items[iter->i];
	return iter->ptr;
}

/* grows internal buffer to satisfy required minimal capacity */
static void acl_array_grow(ACL_ARRAY *a, int min_capacity)
{
	int min_delta = 16;
	int delta;

	/* don't need to grow the capacity of the array */
	if(a->capacity >= min_capacity)
		return;

	delta = min_capacity;
	/* make delta a multiple of min_delta */
	delta += min_delta - 1;
	delta /= min_delta;
	delta *= min_delta;
	/* actual grow */
	if (delta <= 0)
		return;

	a->capacity += delta;

	if (a->items == NULL) {
		if (a->dbuf == NULL) {
			a->items = (void**)
				acl_mymalloc(a->capacity * sizeof(void*));
		} else {
			a->items = (void**) acl_dbuf_pool_alloc(a->dbuf,
					a->capacity * sizeof(void*));
		}
	} else if (a->dbuf == NULL) {
		a->items = (void**) acl_myrealloc(a->items,
				a->capacity * sizeof(void*));
	} else if (a->count > 0) {
		void **old_items = a->items;
		a->items = (void**) acl_dbuf_pool_calloc(a->dbuf,
				a->capacity * sizeof(void*));
		memcpy(a->items, old_items, a->count * sizeof(void*));
	} else {
		a->items = (void **) acl_dbuf_pool_calloc(a->dbuf,
				a->capacity * sizeof(void *));
	}

	/* reset, just in case */
	memset(a->items + a->count, 0,
		(a->capacity - a->count) * sizeof(void *));
}

ACL_ARRAY *acl_array_create(int init_size)
{
	return acl_array_dbuf_create(init_size, NULL);
}

ACL_ARRAY *acl_array_dbuf_create(int init_size, ACL_DBUF_POOL *dbuf)
{
	ACL_ARRAY *a;

	if (dbuf != NULL) {
		a = (ACL_ARRAY *) acl_dbuf_pool_calloc(dbuf,
				sizeof(ACL_ARRAY));
		a->dbuf = dbuf;
	} else {
		a = (ACL_ARRAY *) acl_mycalloc(1, sizeof(ACL_ARRAY));
		a->dbuf = NULL;
	}

	a->push_back = array_push_back;
	a->push_front = array_push_front;
	a->pop_back = array_pop_back;
	a->pop_front = array_pop_front;
	a->iter_head = array_iter_head;
	a->iter_next = array_iter_next;
	a->iter_tail = array_iter_tail;
	a->iter_prev = array_iter_prev;

	if(init_size <= 0)
		init_size = 100;

	acl_array_pre_append(a, init_size);

	return a;
}

void acl_array_clean(ACL_ARRAY *a, void (*free_fn)(void *))
{
	int	idx;

	for(idx = 0; idx < a->count; idx++) {
		if(free_fn != NULL && a->items[idx] != NULL)
			free_fn(a->items[idx]);
		a->items[idx] = NULL;	/* sanity set to be null */
	}
	a->count = 0;
}

void acl_array_free(ACL_ARRAY *a, void (*free_fn)(void *))
{
	acl_array_clean(a, free_fn);

	if (a->dbuf)
		return;

	if (a->items)
		acl_myfree(a->items);
	acl_myfree(a);
}

int acl_array_append(ACL_ARRAY *a, void *obj)
{
	if (a->count >= a->capacity)
		acl_array_grow(a, a->count + 16);
	a->items[a->count++] = obj;
	return a->count - 1;
}

int acl_array_pred_insert(ACL_ARRAY *a, int position, void *obj)
{
	int	idx;

	/*
	 * a->items[count - 1] should be the last valid item node
	 * position should: positioin >= 0 && position <= a->count - 1
	 */
	if(position < 0 || position >= a->count)
		return -1;

	if(a->count >= a->capacity)
		acl_array_grow(a, a->count + 1);

	/* NOTICE: the C's index begin with 0
	 * when position == 0, just prepend one new node before the first node
	 * of the array
	 */
	for(idx = a->count; idx > position && idx > 0; idx--) {
		/* if idx == 0 then we has arrived
		 * at the beginning of the array
		 */
		a->items[idx] = a->items[idx - 1];
	}
	a->items[position] = obj;
	a->count++;
	return position;
}

int acl_array_succ_insert(ACL_ARRAY *a, int position, void *obj)
{
	int	idx, position_succ;

	/*
	 * a->items[count - 1] should be the last valid item node
	 * position should: position >= 0 && position <= a->count - 1
	 */
	if (position < 0 || position >= a->count)
		return -1;

	if (a->count >= a->capacity)
		acl_array_grow(a, a->count + 1);

	position_succ = position + 1;

	/*
	 * position_succ should:
	 * position_succ > 0 (position >= 0 and position_succ = position + 1)
	 * and position_succ <= a->count (when position == a->count - 1,
	 * position == a->count, and just append one new node after the
	 * last node)
	 * NOTICE: the C's index begin with 0
	 */
	for (idx = a->count; idx > position_succ; idx--)
		a->items[idx] = a->items[idx - 1];
	a->items[position_succ] = obj;
	a->count++;
	return position_succ;
}

int acl_array_prepend(ACL_ARRAY *a, void *obj)
{
	return acl_array_pred_insert(a, 0, obj);
}

int acl_array_delete_idx(ACL_ARRAY *a, int position, void (*free_fn)(void *))
{
	int	idx;

	if (position < 0 || position >= a->count)
		return -1;
	if (free_fn != NULL && a->items[position] != NULL)
		free_fn(a->items[position]);
	a->items[position] = NULL;   /* sanity set to be null */

	for (idx = position; idx < a->count - 1; idx++)
		a->items[idx] = a->items[idx + 1];
	a->count--;
	return 0;
}

int acl_array_delete(ACL_ARRAY *a, int idx, void (*free_fn)(void*))
{
	if (idx < 0 || idx >= a->count)
		return  -1;
	if (free_fn != NULL && a->items[idx] != NULL)
		free_fn(a->items[idx]);
	a->count--;
	if (a->count > 0)
		a->items[idx] = a->items[a->count];
	return 0;
}

int acl_array_delete_obj(ACL_ARRAY *a, void *obj, void (*free_fn)(void *))
{
	int   idx, position, ret;

	position = -1;
	for (idx = 0; idx < a->count; idx++) {
		if (a->items[idx] == obj) {
			position = idx;
			break;
		}
	}

	if (free_fn != NULL && obj != NULL)
		free_fn(obj);
	if (position == -1) /* not found */
		return -1;

	/* don't need to free the obj in acl_array_delete_idx */
	a->items[idx] = NULL;
	ret = acl_array_delete_idx(a, position, NULL);
	if (ret < 0)
		return -1;
	return ret;
}

int acl_array_delete_range(ACL_ARRAY *a, int ibegin, int iend,
	void (*free_fn)(void*))
{
	int   i, imax;

	if (ibegin < 0 || iend < 0 || a->count <= 0)
		return -1;
	if (ibegin > iend)
		return -1;

	imax = a->count - 1;
	if (iend > imax)
		iend = imax;

	if (free_fn != NULL) {
		for (i = ibegin; i <= iend; i++) {
			if (a->items[i] != NULL)
				free_fn(a->items[i]);
			a->items[i] = NULL;
		}
	}

	a->count -= iend - ibegin + 1;

	for (iend++; iend <= imax;) {
		a->items[ibegin++] = a->items[iend++];
	}

	return 0;
}

int acl_array_mv_idx(ACL_ARRAY *a, int ito, int ifrom, void (*free_fn)(void *))
{
	int   i, i_obj, i_src, i_max;

	if (ito < 0 || ifrom < 0 || a->count < 0)
		return -1;

	if (a->count == 0 || ito >= ifrom || ifrom >= a->count)
		return 0;

	i_obj = ito;
	i_src = ifrom;
	i_max = a->count - 1;

	if (free_fn != NULL) {
		for (i = i_obj; i < i_src; i++) {
			if (a->items[i] != NULL)
				free_fn(a->items[i]);
			a->items[i] = NULL;
		}
	}
	for (; i_src <= i_max; i_src++) {
		a->items[i_obj] = a->items[i_src];
		i_obj++;
	}

	a->count -= ifrom - ito;
	if (a->count < 0)	/* imposible, sanity check */
		return -1;

	return 0;
}

/* if you are going to append a known and large number of items,
 * call this first
 */
void acl_array_pre_append(ACL_ARRAY *a, int app_count)
{
	const char *myname = "acl_array_pre_append";

	if (app_count <= 0)
		acl_msg_fatal("%s(%d)->%s: invalid input",
				__FILE__, __LINE__, myname);

	if (a->count + app_count > a->capacity)
		acl_array_grow(a, a->count + app_count);
}

void *acl_array_index(const ACL_ARRAY *a, int idx)
{
	if (idx < 0 || idx > a->count - 1)
		return NULL;

	return a->items[idx];
}

int acl_array_size(const ACL_ARRAY *a)
{
	return a->count;
}
