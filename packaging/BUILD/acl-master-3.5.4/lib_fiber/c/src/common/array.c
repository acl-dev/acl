#include "stdafx.h"
#include "memory.h"
#include "msg.h"
#include "array.h"

static void array_push_back(struct ARRAY *a, void *obj)
{
	array_append(a, obj);
}

static void array_push_front(struct ARRAY *a, void *obj)
{
	array_prepend(a, obj);
}

static void *array_pop_back(struct ARRAY *a)
{
	void *obj;
	if (a->count <= 0) {
		return NULL;
	}
	a->count--;
	obj = a->items[a->count];
	return obj;
}

static void *array_pop_front(struct ARRAY *a)
{
	void *obj;
	int   i;

	if (a->count <= 0) {
		return NULL;
	}
	obj = a->items[0];
	a->count--;
	for (i = 0; i < a->count; i++) {
		a->items[i] = a->items[i + 1];
	}

	return obj;
}

/* array_iter_head - get the head of the array */

static void *array_iter_head(ITER *iter, struct ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = 0;
	iter->size = a->count;
	if (a->items == NULL || a->count <= 0) {
		iter->ptr = iter->data = 0;
	} else {
		iter->ptr = iter->data = a->items[0];
	}

	return iter->ptr;
}

/* array_iter_next - get the next of the array */

static void *array_iter_next(ITER *iter, struct ARRAY *a)
{
	iter->i++;
	if (iter->i >= a->count) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = a->items[iter->i];
	}
	return iter->ptr;
}
 
/* array_iter_tail - get the tail of the array */

static void *array_iter_tail(ITER *iter, struct ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = a->count - 1;
	iter->size = a->count;
	if (a->items == NULL || iter->i < 0) {
		iter->ptr = iter->data = 0;
	} else {
		iter->data = iter->ptr = a->items[iter->i];
	}
	return iter->ptr;
}

/* array_iter_prev - get the prev of the array */

static void *array_iter_prev(ITER *iter, struct ARRAY *a)
{
	iter->i--;
	if (iter->i < 0) {
		iter->data = iter->ptr = 0;
	} else {
		iter->data = iter->ptr = a->items[iter->i];
	}
	return iter->ptr;
}

/* grows internal buffer to satisfy required minimal capacity */
static void array_grow(ARRAY *a, int min_capacity)
{
	int min_delta = 16;
	int delta;

	/* don't need to grow the capacity of the array */
	if (a->capacity >= min_capacity) {
		return;
	}

	delta = min_capacity;
	/* make delta a multiple of min_delta */
	delta += min_delta - 1;
	delta /= min_delta;
	delta *= min_delta;
	/* actual grow */
	if (delta <= 0) {
		return;
	}

	a->capacity += delta;

	if (a->items == NULL) {
		a->items = (void**) mem_malloc(a->capacity * sizeof(void*));
	} else {
		a->items = (void**) mem_realloc(a->items, a->capacity * sizeof(void*));
	}

	/* reset, just in case */
	memset(a->items + a->count, 0,
		(a->capacity - a->count) * sizeof(void *));
}

ARRAY *array_create(int init_size)
{
	ARRAY *a;

	a = (ARRAY *) mem_calloc(1, sizeof(ARRAY));

	a->push_back  = array_push_back;
	a->push_front = array_push_front;
	a->pop_back   = array_pop_back;
	a->pop_front  = array_pop_front;
	a->iter_head  = array_iter_head;
	a->iter_next  = array_iter_next;
	a->iter_tail  = array_iter_tail;
	a->iter_prev  = array_iter_prev;

	if(init_size <= 0) {
		init_size = 100;
	}

	array_pre_append(a, init_size);

	return a;
}

void array_clean(ARRAY *a, void (*free_fn)(void *))
{
	int	idx;

	for(idx = 0; idx < a->count; idx++) {
		if(free_fn != NULL && a->items[idx] != NULL) {
			free_fn(a->items[idx]);
		}
		a->items[idx] = NULL;	/* sanity set to be null */
	}
	a->count = 0;
}

void array_free(ARRAY *a, void (*free_fn)(void *))
{
	array_clean(a, free_fn);
	if (a->items) {
		mem_free(a->items);
	}
	mem_free(a);
}

int array_append(ARRAY *a, void *obj)
{
	if (a->count >= a->capacity) {
		array_grow(a, a->count + 16);
	}
	a->items[a->count++] = obj;
	return a->count - 1;
}

int array_pred_insert(ARRAY *a, int position, void *obj)
{
	int	idx;

	/*
	 * a->items[count - 1] should be the last valid item node
	 * position should: positioin >= 0 && position <= a->count - 1
	 */
	if(position < 0 || position >= a->count) {
		return -1;
	}

	if(a->count >= a->capacity) {
		array_grow(a, a->count + 1);
	}

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

int array_succ_insert(ARRAY *a, int position, void *obj)
{
	int	idx, position_succ;

	/*
	 * a->items[count - 1] should be the last valid item node
	 * position should: position >= 0 && position <= a->count - 1
	 */
	if (position < 0 || position >= a->count) {
		return -1;
	}

	if (a->count >= a->capacity) {
		array_grow(a, a->count + 1);
	}

	position_succ = position + 1;

	/*
	 * position_succ should:
	 * position_succ > 0 (position >= 0 and position_succ = position + 1)
	 * and position_succ <= a->count (when position == a->count - 1,
	 * position == a->count, and just append one new node after the
	 * last node)
	 * NOTICE: the C's index begin with 0
	 */
	for (idx = a->count; idx > position_succ; idx--) {
		a->items[idx] = a->items[idx - 1];
	}
	a->items[position_succ] = obj;
	a->count++;
	return position_succ;
}

int array_prepend(ARRAY *a, void *obj)
{
	return array_pred_insert(a, 0, obj);
}

int array_delete_idx(ARRAY *a, int position, void (*free_fn)(void *))
{
	int	idx;

	if (position < 0 || position >= a->count) {
		return -1;
	}
	if (free_fn != NULL && a->items[position] != NULL) {
		free_fn(a->items[position]);
	}
	a->items[position] = NULL;   /* sanity set to be null */

	for (idx = position; idx < a->count - 1; idx++) {
		a->items[idx] = a->items[idx + 1];
	}
	a->count--;
	return 0;
}

int array_delete(ARRAY *a, int idx, void (*free_fn)(void*))
{
	if (idx < 0 || idx >= a->count) {
		return  -1;
	}
	if (free_fn != NULL && a->items[idx] != NULL) {
		free_fn(a->items[idx]);
	}
	a->count--;
	if (a->count > 0) {
		a->items[idx] = a->items[a->count];
	}
	return 0;
}

int array_delete_obj(ARRAY *a, void *obj, void (*free_fn)(void *))
{
	int   idx, position, ret;

	position = -1;
	for (idx = 0; idx < a->count; idx++) {
		if (a->items[idx] == obj) {
			position = idx;
			break;
		}
	}

	if (free_fn != NULL && obj != NULL) {
		free_fn(obj);
	}
	if (position == -1) { /* not found */
		return -1;
	}

	/* don't need to free the obj in array_delete_idx */
	a->items[idx] = NULL;
	ret = array_delete_idx(a, position, NULL);
	if (ret < 0) {
		return -1;
	}
	return ret;
}

int array_delete_range(ARRAY *a, int ibegin, int iend,
	void (*free_fn)(void*))
{
	int   i, imax;

	if (ibegin < 0 || iend < 0 || a->count <= 0) {
		return -1;
	}
	if (ibegin > iend) {
		return -1;
	}

	imax = a->count - 1;
	if (iend > imax) {
		iend = imax;
	}

	if (free_fn != NULL) {
		for (i = ibegin; i <= iend; i++) {
			if (a->items[i] != NULL) {
				free_fn(a->items[i]);
			}
			a->items[i] = NULL;
		}
	}

	a->count -= iend - ibegin + 1;

	for (iend++; iend <= imax;) {
		a->items[ibegin++] = a->items[iend++];
	}

	return 0;
}

int array_mv_idx(ARRAY *a, int ito, int ifrom, void (*free_fn)(void *))
{
	int   i, i_obj, i_src, i_max;

	if (ito < 0 || ifrom < 0 || a->count < 0) {
		return -1;
	}

	if (a->count == 0 || ito >= ifrom || ifrom >= a->count) {
		return 0;
	}

	i_obj = ito;
	i_src = ifrom;
	i_max = a->count - 1;

	if (free_fn != NULL) {
		for (i = i_obj; i < i_src; i++) {
			if (a->items[i] != NULL) {
				free_fn(a->items[i]);
			}
			a->items[i] = NULL;
		}
	}
	for (; i_src <= i_max; i_src++) {
		a->items[i_obj] = a->items[i_src];
		i_obj++;
	}

	a->count -= ifrom - ito;
	if (a->count < 0) { /* imposible, sanity check */
		return -1;
	}
	return 0;
}

/* if you are going to append a known and large number of items,
 * call this first
 */
void array_pre_append(ARRAY *a, int app_count)
{
	const char *name = "array_pre_append";

	if (app_count <= 0) {
		msg_fatal("%s(%d)->%s: invalid input", __FILE__, __LINE__, name);
	}

	if (a->count + app_count > a->capacity) {
		array_grow(a, a->count + app_count);
	}
}

void *array_index(const ARRAY *a, int idx)
{
	if (idx < 0 || idx > a->count - 1) {
		return NULL;
	}

	return a->items[idx];
}

int array_size(const ARRAY *a)
{
	return a->count;
}
