#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>

#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "private_fifo.h"

static void *fifo_iter_head(ACL_ITER *iter, struct ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *ptr;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = 0;
	iter->size = fifo->cnt;
	iter->ptr = ptr = fifo->head;
	iter->data = ptr ? ptr->data : NULL;
	return (iter->ptr);
}

static void *fifo_iter_next(ACL_ITER *iter, struct ACL_FIFO *fifo acl_unused)
{
	ACL_FIFO_INFO *ptr;

	ptr = (ACL_FIFO_INFO*) iter->ptr;
	iter->ptr = ptr = ptr ? ptr->next : NULL;
	if (ptr) {
		iter->data = ptr->data;
		iter->i++;
	} else
		iter->data = NULL;
	return (iter);
}

static void *fifo_iter_tail(ACL_ITER *iter, struct ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *ptr;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = fifo->cnt - 1;
	iter->size = fifo->cnt;
	iter->ptr = ptr = fifo->tail;
	iter->data = ptr ? ptr->data : NULL;
	return (iter->ptr);
}

static void *fifo_iter_prev(ACL_ITER *iter, struct ACL_FIFO *fifo acl_unused)
{
	ACL_FIFO_INFO *ptr;

	ptr = (ACL_FIFO_INFO*) iter->ptr;
	iter->ptr = ptr = ptr ? ptr->prev : NULL;
	if (ptr) {
		iter->data = ptr->data;
		iter->i--;
	} else
		iter->data = NULL;
	return (iter);
}

static ACL_FIFO_INFO *fifo_iter_info(ACL_ITER *iter, struct ACL_FIFO *fifo acl_unused)
{
	return (iter->ptr ? (ACL_FIFO_INFO*) iter->ptr : NULL);
}

void private_fifo_init(ACL_FIFO *fifo)
{
	fifo->head = NULL;
	fifo->tail = NULL;
	fifo->cnt = 0;

	fifo->iter_head = fifo_iter_head;
	fifo->iter_next = fifo_iter_next;
	fifo->iter_tail = fifo_iter_tail;
	fifo->iter_prev = fifo_iter_prev;
	fifo->iter_info = fifo_iter_info;
}

ACL_FIFO *private_fifo_new(void)
{
	ACL_FIFO *fifo;

	fifo = (ACL_FIFO *) malloc(sizeof(*fifo));
	fifo->head = NULL;
	fifo->tail = NULL;
	fifo->cnt = 0;

	fifo->iter_head = fifo_iter_head;
	fifo->iter_next = fifo_iter_next;
	fifo->iter_tail = fifo_iter_tail;
	fifo->iter_prev = fifo_iter_prev;

	return (fifo);
}

void private_fifo_free(ACL_FIFO *fifo, void (*free_fn)(void *))
{
	void *data;

	while ((data = private_fifo_pop(fifo)) != NULL) {
		if (free_fn)
			free_fn(data);
	}
	free(fifo);
}

ACL_FIFO_INFO *private_fifo_push(ACL_FIFO *fifo, void *data)
{
	ACL_FIFO_INFO *info;

	info = (ACL_FIFO_INFO *) malloc(sizeof(*info));
	info->data = data;

	if (fifo->tail == NULL) {
		info->prev = info->next = NULL;
		fifo->head = fifo->tail = info;
	} else {
		fifo->tail->next = info;
		info->prev = fifo->tail;
		info->next = NULL;
		fifo->tail = info;
	}

	fifo->cnt++;
	return (info);
}

void *private_fifo_pop(ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *info;
	void *data;

	if (fifo->head == NULL)
		return (NULL);

	info = fifo->head;
	if (fifo->head->next) {
		fifo->head->next->prev = NULL;
		fifo->head = fifo->head->next;
	} else {
		fifo->head = fifo->tail = NULL;
	}
	data = info->data;
	free(info);
	fifo->cnt--;
	return (data);
}

void private_delete_info(ACL_FIFO *fifo, ACL_FIFO_INFO *info)
{
	if (info->prev)
		info->prev->next = info->next;
	else
		fifo->head = info->next;
	if (info->next)
		info->next->prev = info->prev;
	else
		fifo->tail = info->prev;

	free(info);
	fifo->cnt--;
}

void *private_fifo_head(ACL_FIFO *fifo)
{
	if (fifo->head)
		return (fifo->head->data);
	else
		return (NULL);
}

void *private_fifo_tail(ACL_FIFO *fifo)
{
	if (fifo->tail)
		return (fifo->tail->data);
	else
		return (NULL);
}

int private_fifo_size(ACL_FIFO *fifo)
{
	if (fifo)
		return (fifo->cnt);
	else
		return (0);
}
