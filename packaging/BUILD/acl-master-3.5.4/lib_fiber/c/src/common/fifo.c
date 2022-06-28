#include "stdafx.h"
#include "memory.h"
#include "iterator.h"
#include "fifo.h"

static void push_back(FIFO *fifo, void *data)
{
	fifo_push_back(fifo, data);
}

static void push_front(FIFO *fifo, void *data)
{
	fifo_push_front(fifo, data);
}

static void* pop_back(FIFO *fifo)
{
	return fifo_pop_back(fifo);
}

static void* pop_front(FIFO *fifo)
{
	return fifo_pop_front(fifo);
}

static void *iter_head(ITER *iter, struct FIFO *fifo)
{
	FIFO_INFO *ptr;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = 0;
	iter->size = fifo->cnt;
	iter->ptr = ptr = fifo->head;
	iter->data = ptr ? ptr->data : NULL;
	return iter->ptr;
}

static void *iter_next(ITER *iter, struct FIFO *fifo fiber_unused)
{
	FIFO_INFO *ptr;

	ptr = (FIFO_INFO*) iter->ptr;
	iter->ptr = ptr = ptr ? ptr->next : NULL;
	if (ptr) {
		iter->data = ptr->data;
		iter->i++;
	} else
		iter->data = NULL;
	return iter;
}

static void *iter_tail(ITER *iter, struct FIFO *fifo)
{
	FIFO_INFO *ptr;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = fifo->cnt - 1;
	iter->size = fifo->cnt;
	iter->ptr = ptr = fifo->tail;
	iter->data = ptr ? ptr->data : NULL;
	return iter->ptr;
}

static void *iter_prev(ITER *iter, struct FIFO *fifo fiber_unused)
{
	FIFO_INFO *ptr;

	ptr = (FIFO_INFO*) iter->ptr;
	iter->ptr = ptr = ptr ? ptr->prev : NULL;
	if (ptr) {
		iter->data = ptr->data;
		iter->i--;
	} else
		iter->data = NULL;
	return iter;
}

static FIFO_INFO *iter_info(ITER *iter, struct FIFO *fifo fiber_unused)
{
	return iter->ptr ? (FIFO_INFO*) iter->ptr : NULL;
}

void fifo_init(FIFO *fifo)
{
	fifo->head = NULL;
	fifo->tail = NULL;
	fifo->cnt = 0;

	fifo->push_back  = push_back;
	fifo->push_front = push_front;
	fifo->pop_back   = pop_back;
	fifo->pop_front  = pop_front;
	fifo->iter_head  = iter_head;
	fifo->iter_next  = iter_next;
	fifo->iter_tail  = iter_tail;
	fifo->iter_prev  = iter_prev;
	fifo->iter_info  = iter_info;
}

FIFO *fifo_new(void)
{
	FIFO *fifo;

	fifo = (FIFO *) mem_malloc(sizeof(*fifo));
	fifo->head = NULL;
	fifo->tail = NULL;
	fifo->cnt = 0;

	fifo->push_back  = push_back;
	fifo->push_front = push_front;
	fifo->pop_back   = pop_back;
	fifo->pop_front  = pop_front;
	fifo->iter_head  = iter_head;
	fifo->iter_next  = iter_next;
	fifo->iter_tail  = iter_tail;
	fifo->iter_prev  = iter_prev;

	return fifo;
}

void fifo_free(FIFO *fifo, void (*free_fn)(void *))
{
	void *data;

	while ((data = fifo_pop(fifo)) != NULL) {
		if (free_fn)
			free_fn(data);
	}
	mem_free(fifo);
}

FIFO_INFO *fifo_push_back(FIFO *fifo, void *data)
{
	FIFO_INFO *info;

	info = (FIFO_INFO *) mem_malloc(sizeof(*info));
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
	return info;
}

FIFO_INFO *fifo_push_front(FIFO *fifo, void *data)
{
	FIFO_INFO *info;

	info = (FIFO_INFO*) mem_malloc(sizeof(*info));
	info->data = data;

	if (fifo->head == NULL) {
		info->prev = info->next = NULL;
		fifo->head = fifo->tail = info;
	} else {
		info->next = fifo->head;
		fifo->head->prev = info;
		info->prev = NULL;
		fifo->head = info;
	}

	fifo->cnt++;
	return info;
}

void *fifo_pop_front(FIFO *fifo)
{
	FIFO_INFO *info;
	void *data;

	if (fifo->head == NULL)
		return NULL;

	info = fifo->head;
	if (fifo->head->next) {
		fifo->head->next->prev = NULL;
		fifo->head = fifo->head->next;
	} else {
		fifo->head = fifo->tail = NULL;
	}
	data = info->data;
	mem_free(info);
	fifo->cnt--;
	return data;
}

void *fifo_pop_back(FIFO *fifo)
{
	FIFO_INFO *info;
	void *data;

	if (fifo->tail == NULL)
		return NULL;

	info = fifo->tail;
	if (fifo->tail->prev) {
		fifo->tail->prev->next = NULL;
		fifo->tail = fifo->tail->prev;
	} else {
		fifo->head = fifo->tail = NULL;
	}
	data = info->data;
	mem_free(info);
	fifo->cnt--;
	return data;
}

int fifo_delete(FIFO *fifo, const void *data)
{
	FIFO_INFO *iter = fifo->head;

	while (iter) {
		if (iter->data == data) {
			if (iter->prev)
				iter->prev->next = iter->next;
			else
				fifo->head = iter->next;
			if (iter->next)
				iter->next->prev = iter->prev;
			else
				fifo->tail = iter->prev;
			mem_free(iter);
			fifo->cnt--;
			return 1;
		}

		iter = iter->next;
	}
	return 0;
}

void *fifo_head(FIFO *fifo)
{
	if (fifo->head)
		return fifo->head->data;
	else
		return NULL;
}

void *fifo_tail(FIFO *fifo)
{
	if (fifo->tail)
		return fifo->tail->data;
	else
		return NULL;
}

void fifo_free2(FIFO *fifo, void (*free_fn)(FIFO_INFO *))
{
	FIFO_INFO *info;

	while ((info = fifo_pop_info(fifo)) != NULL) {
		if (free_fn)
			free_fn(info);
	}
	mem_free(fifo);
}

void fifo_push_info_back(FIFO *fifo, FIFO_INFO *info)
{
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
}

FIFO_INFO *fifo_pop_info(FIFO *fifo)
{
	FIFO_INFO *info;

	if (fifo->head == NULL)
		return NULL;

	info = fifo->head;
	if (fifo->head->next) {
		fifo->head->next->prev = NULL;
		fifo->head = fifo->head->next;
	} else {
		fifo->head = fifo->tail = NULL;
	}

	fifo->cnt--;
	return info;
}

void fifo_delete_info(FIFO *fifo, FIFO_INFO *info)
{
	if (info->prev)
		info->prev->next = info->next;
	else
		fifo->head = info->next;
	if (info->next)
		info->next->prev = info->prev;
	else
		fifo->tail = info->prev;

	mem_free(info);
	fifo->cnt--;
}

FIFO_INFO *fifo_head_info(FIFO *fifo)
{
	if (fifo->head)
		return fifo->head;
	else
		return NULL;
}

FIFO_INFO *fifo_tail_info(FIFO *fifo)
{
	if (fifo->tail)
		return fifo->tail;
	else
		return NULL;
}

int fifo_size(FIFO *fifo)
{
	if (fifo)
		return fifo->cnt;
	else
		return 0;
}
