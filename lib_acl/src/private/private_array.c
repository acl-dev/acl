#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif

#include "private_array.h"

/* array_iter_head - get the head of the array */

static void *array_iter_head(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = 0;
	iter->size = a->count;
	if (a->items == NULL)
		iter->ptr = iter->data = 0;
	else
		iter->ptr = a->items[0];

	iter->data = iter->ptr;
	return (iter->ptr);
}

/* array_iter_next - get the next of the array */

static void *array_iter_next(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->i++;
	if (iter->i >= a->count)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = a->items[iter->i];
	return (iter->ptr);
}
 
/* array_iter_tail - get the tail of the array */

static void *array_iter_tail(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = 0;
	iter->i = a->count - 1;
	iter->size = a->count;
	if (a->items == NULL)
		iter->ptr = iter->data = 0;
	else if (iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = a->items[iter->i];
	return (iter->ptr);
}

/* array_iter_prev - get the prev of the array */

static void *array_iter_prev(ACL_ITER *iter, struct ACL_ARRAY *a)
{
	iter->i--;
	if (iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = a->items[iter->i];
	return (iter->ptr);
}

/* grows internal buffer to satisfy required minimal capacity */

void private_array_grow(ACL_ARRAY *a, int min_capacity)
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
	if (a->items) {
		a->items = (void **) realloc(a->items, a->capacity * sizeof(void *));
	} else {
		a->items = (void **) malloc(a->capacity * sizeof(void *));
	}

	/* reset, just in case */
	memset(a->items + a->count, 0, (a->capacity - a->count) * sizeof(void *));
}

static void array_init(ACL_ARRAY *a)
{
	memset(a, 0, sizeof(ACL_ARRAY));
}

static void array_prepare_append(ACL_ARRAY *a, int app_count)
{
	acl_assert(app_count > 0);
	if (a->count + app_count > a->capacity)
		private_array_grow(a, a->count + app_count);
}

ACL_ARRAY *private_array_create(int init_size)
{
	ACL_ARRAY *a;

	a = (ACL_ARRAY *) calloc(1, sizeof(ACL_ARRAY));
	acl_assert(a);

	array_init(a);
	a->iter_head = array_iter_head;
	a->iter_next = array_iter_next;
	a->iter_tail = array_iter_tail;
	a->iter_prev = array_iter_prev;

	if(init_size > 0)
		array_prepare_append(a, init_size);

	return(a);
}

void private_array_clean(ACL_ARRAY *a, void (*free_fn)(void *))
{
	int	idx;

	for (idx = 0; idx < a->count; idx++) {
		if(free_fn != NULL && a->items[idx] != NULL)
			free_fn(a->items[idx]);
		a->items[idx] = NULL;	/* sanity set to be null */
	}
	a->count = 0;
}

void private_array_destroy(ACL_ARRAY *a, void (*free_fn)(void *))
{
	private_array_clean(a, free_fn);
	if (a->items)
		free(a->items);
	free(a);
}

int private_array_push(ACL_ARRAY *a, void *obj)
{
	if (a->count >= a->capacity)
		private_array_grow(a, a->count + 16);
	a->items[a->count++] = obj;
	return(a->count - 1);
}

void *private_array_pop(ACL_ARRAY *a)
{
	void *ptr;

	if (a->count <= 0)
		return (NULL);
	a->count--;
	ptr = a->items[a->count];
	a->items[a->count] = NULL;
	return (ptr);
}

int private_array_delete(ACL_ARRAY *a, int idx, void (*free_fn)(void*))
{
	if (a == NULL)
		return (-1);
	if (idx < 0 || idx >= a->count)
		return (-1);
	if (free_fn != NULL && a->items[idx] != NULL)
		free_fn(a->items[idx]);
	a->count--;
	if (a->count > 0)
		a->items[idx] = a->items[a->count];
	return (0);
}

int private_array_delete_obj(ACL_ARRAY *a, void *obj, void (*free_fn)(void*))
{
	int   i;

	if (a == NULL || obj == NULL)
		return (-1);
	for (i = 0; i < a->count; i++) {
		if (a->items[i] == obj) {
			return (private_array_delete(a, i, free_fn));
		}
	}
	return (-1);
}

void *private_array_index(const ACL_ARRAY *a, int idx)
{
	if(idx < 0 || idx > a->count - 1)
		return(NULL);

	return(a->items[idx]);
}

int private_array_size(const ACL_ARRAY *a)
{
	if(a == NULL)
		return(-1);
	return(a->count);
}
