#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_stack.h"

#endif

static void stack_push_back(struct ACL_STACK *s, void *obj)
{
	acl_stack_append(s, obj);
}

static void stack_push_front(struct ACL_STACK *s, void *obj)
{
	acl_stack_prepend(s, obj);
}

static void *stack_pop_back(struct ACL_STACK *s)
{
	return (acl_stack_pop(s));
}

static void *stack_pop_front(struct ACL_STACK *s)
{
	int   i;
	void *obj;

	if (s->count < 1)
		return (NULL);

	obj = s->items[0];
	s->count--;
	for (i = 0; i < s->count; i++) {
		s->items[i] = s->items[i + 1];
	}
	return (obj);	
}

/* stack_iter_head - get the head of the stack */

static void *stack_iter_head(ACL_ITER *iter, struct ACL_STACK *s)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = 0;
	iter->size = s->count;;
	if (s->items == NULL || s->count <= 0)
		iter->ptr = NULL;
	else
		iter->ptr = s->items[0];

	iter->data = iter->ptr;
	return (iter->ptr);
}

/* stack_iter_next - get the next of the stack */

static void *stack_iter_next(ACL_ITER *iter, struct ACL_STACK *s)
{
	iter->i++;
	if (iter->i >= s->count)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = s->items[iter->i];
	return (iter->ptr);
}
 
/* stack_iter_tail - get the tail of the stack */

static void *stack_iter_tail(ACL_ITER *iter, struct ACL_STACK *s)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = s->count - 1;
	iter->size = s->count;
	if (s->items == NULL || iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = s->items[iter->i];
	return (iter->ptr);
}

/* stack_iter_prev - get the prev of the stack */

static void *stack_iter_prev(ACL_ITER *iter, struct ACL_STACK *s)
{
	iter->i--;
	if (iter->i < 0)
		iter->data = iter->ptr = 0;
	else
		iter->data = iter->ptr = s->items[iter->i];
	return (iter->ptr);
}

/* grows internal buffer to satisfy required minimal capacity */

static void stack_grow(ACL_STACK *s, int min_capacity)
{
	const char *myname = "stack_grow";
	const char *ptr;
	int min_delta = 16;
	int delta;

	/* don't need to grow the capacity of the array */
	if (s->capacity >= min_capacity)
		return;
	delta = min_capacity;
	/* make delta a multiple of min_delta */
	delta += min_delta - 1;
	delta /= min_delta;
	delta *= min_delta;
	/* actual grow */
	if (delta <= 0)
		return;
	s->capacity += delta;
	if (s->items) {
		s->items = (void **) acl_default_realloc(__FILE__, __LINE__,
				s->items, s->capacity * sizeof(void *));
		ptr = "realloc";
	} else {
		s->items = (void **) acl_default_malloc(__FILE__, __LINE__,
				s->capacity * sizeof(void *));
		ptr = "malloc";
	}

	if (s->items == NULL) {
		char ebuf[256];
		acl_msg_fatal("%s(%d): %s error(%s)",
			myname, __LINE__, ptr,
			acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	/* reset, just in case */
	memset(s->items + s->count, 0, (s->capacity - s->count) * sizeof(void *));
}

/* if you are going to append a known and large number of items, call this first */

void acl_stack_space(ACL_STACK *s, int count)
{
	if (s->count + count > s->capacity)
		stack_grow(s, s->count + count);
}

ACL_STACK *acl_stack_create(int init_size)
{
	const char *myname = "acl_stack_create";
	ACL_STACK *s;

	init_size = init_size > 0 ? init_size : 1;

	s = (ACL_STACK *) acl_default_calloc(__FILE__, __LINE__, 1, sizeof(ACL_STACK));
	if (s == NULL) {
		char ebuf[256];
		acl_msg_fatal("%s(%d): can't calloc, error=%s",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	acl_stack_space(s, init_size);

	s->push_back = stack_push_back;
	s->push_front = stack_push_front;
	s->pop_back = stack_pop_back;
	s->pop_front = stack_pop_front;
	s->iter_head = stack_iter_head;
	s->iter_next = stack_iter_next;
	s->iter_tail = stack_iter_tail;
	s->iter_prev = stack_iter_prev;

	return (s);
}

void acl_stack_clean(ACL_STACK *s, void (*free_fn)(void *))
{
	int   i;

	for (i = 0; i < s->count; i++) {
		if (free_fn != NULL && s->items[i] != NULL)
			free_fn(s->items[i]);
		s->items[i] = NULL;	/* sanity set to be null */
	}

	acl_default_free(__FILE__, __LINE__, s->items);
	s->items = NULL;
	s->count = 0;
}

void acl_stack_destroy(ACL_STACK *s, void (*free_fn)(void *))
{
	acl_stack_clean(s, free_fn);
	memset(s, 0, sizeof(ACL_STACK));
	acl_default_free(__FILE__, __LINE__, s);
}

void acl_stack_append(ACL_STACK *s, void *obj)
{
	if (s == NULL || obj == NULL)
		return;
	if (s->count >= s->capacity)
		stack_grow(s, s->count + 16);
	s->items[s->count++] = obj;
}

void acl_stack_prepend(ACL_STACK *s, void *obj)
{
	int   i;

	if (s == NULL || obj == NULL)
		return;
	if (s->count >= s->capacity)
		stack_grow(s, s->count + 1);

	for (i = s->count; i > 0; i--)
		s->items[i] = s->items[i - 1];
	s->items[0] = obj;
	s->count++;
}

void acl_stack_delete(ACL_STACK *s, int position, void (*free_fn)(void *))
{
	int   i;

	if (s == NULL || position < 0 || position >= s->count)
		return;
	if (free_fn != NULL && s->items[position] != NULL)
		free_fn(s->items[position]);
	s->items[position] = NULL;   /* sanity set to be null */

	for (i = position; i < s->count - 1; i++)
		s->items[i] = s->items[i + 1];
	s->count--;
}

void acl_stack_delete_obj(ACL_STACK *s, void *obj, void (*free_fn)(void *))
{
	int   i, position;

	if (s == NULL || obj == NULL)
		return;
	position = -1;
	for (i = 0; i < s->count; i++) {
		if (s->items[i] == obj) {
			position = i;
			break;
		}
	}

	if (position == -1) /* not found */
		return;
	if (free_fn != NULL && obj != NULL)
		free_fn(obj);

	/* don't need to free the obj in acl_stack_delete */
	s->items[i] = NULL;
	acl_stack_delete(s, position, NULL);
}

void *acl_stack_index(ACL_STACK *s, int idx)
{
	if (s == NULL || idx < 0 || idx > s->count - 1)
		return (NULL);

	return (s->items[idx]);
}

int acl_stack_size(const ACL_STACK *s)
{
	if (s == NULL)
		return (-1);
	return (s->count);
}

void *acl_stack_pop(ACL_STACK *s)
{
	if (s == NULL)
		return (NULL);

	if (s->count < 1)
		return (NULL);

	return (s->items[--s->count]);
}

void *acl_stack_top(ACL_STACK *s)
{
	if (s == NULL)
		return (NULL);

	if (s->count < 1)
		return (NULL);

	return (s->items[s->count - 1]);
}
