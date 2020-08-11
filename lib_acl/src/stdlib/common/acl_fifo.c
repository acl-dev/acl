#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_fifo.h"

#endif

static void fifo_push_back(ACL_FIFO *fifo, void *data)
{
	acl_fifo_push_back(fifo, data);
}

static void fifo_push_front(ACL_FIFO *fifo, void *data)
{
	acl_fifo_push_front(fifo, data);
}

static void* fifo_pop_back(ACL_FIFO *fifo)
{
	return acl_fifo_pop_back(fifo);
}

static void* fifo_pop_front(ACL_FIFO *fifo)
{
	return acl_fifo_pop_front(fifo);
}

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
	return iter->ptr;
}

static void *fifo_iter_next(ACL_ITER *iter, struct ACL_FIFO *fifo acl_unused)
{
	ACL_FIFO_INFO *ptr;

	ptr = (ACL_FIFO_INFO*) iter->ptr;
	iter->ptr = ptr = ptr ? ptr->next : NULL;
	if (ptr) {
		iter->data = ptr->data;
		iter->i++;
	} else {
		iter->data = NULL;
	}
	return iter;
}

static void *fifo_iter_tail(ACL_ITER *iter, struct ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *ptr;

	iter->dlen = -1;
	iter->key  = NULL;
	iter->klen = -1;
	iter->i    = fifo->cnt - 1;
	iter->size = fifo->cnt;
	iter->ptr  = ptr = fifo->tail;
	iter->data = ptr ? ptr->data : NULL;
	return iter->ptr;
}

static void *fifo_iter_prev(ACL_ITER *iter, struct ACL_FIFO *fifo acl_unused)
{
	ACL_FIFO_INFO *ptr;

	ptr = (ACL_FIFO_INFO*) iter->ptr;
	iter->ptr = ptr = ptr ? ptr->prev : NULL;
	if (ptr) {
		iter->data = ptr->data;
		iter->i--;
	} else {
		iter->data = NULL;
	}
	return iter;
}

static ACL_FIFO_INFO *fifo_iter_info(ACL_ITER *iter, struct ACL_FIFO *fifo acl_unused)
{
	return iter->ptr ? (ACL_FIFO_INFO*) iter->ptr : NULL;
}

void acl_fifo_init(ACL_FIFO *fifo)
{
	fifo->head       = NULL;
	fifo->tail       = NULL;
	fifo->cnt        = 0;
	fifo->slice      = NULL;

	fifo->push_back  = fifo_push_back;
	fifo->push_front = fifo_push_front;
	fifo->pop_back   = fifo_pop_back;
	fifo->pop_front  = fifo_pop_front;
	fifo->iter_head  = fifo_iter_head;
	fifo->iter_next  = fifo_iter_next;
	fifo->iter_tail  = fifo_iter_tail;
	fifo->iter_prev  = fifo_iter_prev;
	fifo->iter_info  = fifo_iter_info;
}

ACL_FIFO *acl_fifo_new(void)
{
	return acl_fifo_new1(NULL);
}

ACL_FIFO *acl_fifo_new1(ACL_SLICE_POOL *slice)
{
	ACL_FIFO *fifo;

	if (slice) {
		fifo = (ACL_FIFO *) acl_slice_pool_alloc(__FILE__, __LINE__,
				slice, sizeof(*fifo));
		fifo->slice = slice;
	} else {
		fifo = (ACL_FIFO *) acl_mymalloc(sizeof(*fifo));
		fifo->slice = NULL;
	}
	fifo->head = NULL;
	fifo->tail = NULL;
	fifo->cnt  = 0;

	fifo->push_back  = fifo_push_back;
	fifo->push_front = fifo_push_front;
	fifo->pop_back   = fifo_pop_back;
	fifo->pop_front  = fifo_pop_front;
	fifo->iter_head  = fifo_iter_head;
	fifo->iter_next  = fifo_iter_next;
	fifo->iter_tail  = fifo_iter_tail;
	fifo->iter_prev  = fifo_iter_prev;

	return fifo;
}

void acl_fifo_free(ACL_FIFO *fifo, void (*free_fn)(void *))
{
	void *data;

	while ((data = acl_fifo_pop(fifo)) != NULL) {
		if (free_fn) {
			free_fn(data);
		}
	}
	if (fifo->slice) {
		acl_slice_pool_free(__FILE__, __LINE__, fifo);
	} else {
		acl_myfree(fifo);
	}
}

ACL_FIFO_INFO *acl_fifo_push_back(ACL_FIFO *fifo, void *data)
{
	ACL_FIFO_INFO *info;

	if (fifo->slice) {
		info = (ACL_FIFO_INFO *) acl_slice_pool_alloc(__FILE__, __LINE__,
				fifo->slice, sizeof(*info));
	} else {
		info = (ACL_FIFO_INFO *) acl_mymalloc(sizeof(*info));
	}
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

ACL_FIFO_INFO *acl_fifo_push_front(ACL_FIFO *fifo, void *data)
{
	ACL_FIFO_INFO *info;

	if (fifo->slice) {
		info = (ACL_FIFO_INFO *) acl_slice_pool_alloc(__FILE__, __LINE__,
				fifo->slice, sizeof(*info));
	} else {
		info = (ACL_FIFO_INFO*) acl_mymalloc(sizeof(*info));
	}
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

void *acl_fifo_pop_front(ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *info;
	void *data;

	if (fifo->head == NULL) {
		return NULL;
	}

	info = fifo->head;
	if (fifo->head->next) {
		fifo->head->next->prev = NULL;
		fifo->head = fifo->head->next;
	} else {
		fifo->head = fifo->tail = NULL;
	}

	data = info->data;

	if (fifo->slice) {
		acl_slice_pool_free(__FILE__, __LINE__, info);
	} else {
		acl_myfree(info);
	}

	fifo->cnt--;
	return data;
}

void *acl_fifo_pop_back(ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *info;
	void *data;

	if (fifo->tail == NULL) {
		return NULL;
	}

	info = fifo->tail;
	if (fifo->tail->prev) {
		fifo->tail->prev->next = NULL;
		fifo->tail = fifo->tail->prev;
	} else {
		fifo->head = fifo->tail = NULL;
	}
	data = info->data;
	if (fifo->slice) {
		acl_slice_pool_free(__FILE__, __LINE__, info);
	} else {
		acl_myfree(info);
	}
	fifo->cnt--;
	return data;
}

int acl_fifo_delete(ACL_FIFO *fifo, const void *data)
{
	ACL_FIFO_INFO *iter = fifo->head;

	while (iter) {
		if (iter->data == data) {
			if (iter->prev) {
				iter->prev->next = iter->next;
			} else {
				fifo->head = iter->next;
			}
			if (iter->next) {
				iter->next->prev = iter->prev;
			} else {
				fifo->tail = iter->prev;
			}
			if (fifo->slice) {
				acl_slice_pool_free(__FILE__, __LINE__, iter);
			} else {
				acl_myfree(iter);
			}
			fifo->cnt--;
			return 1;
		}

		iter = iter->next;
	}
	return 0;
}

void *acl_fifo_head(ACL_FIFO *fifo)
{
	if (fifo->head) {
		return fifo->head->data;
	} else {
		return NULL;
	}
}

void *acl_fifo_tail(ACL_FIFO *fifo)
{
	if (fifo->tail) {
		return fifo->tail->data;
	} else {
		return NULL;
	}
}

void acl_fifo_free2(ACL_FIFO *fifo, void (*free_fn)(ACL_FIFO_INFO *))
{
	ACL_FIFO_INFO *info;

	while ((info = acl_fifo_pop_info(fifo)) != NULL) {
		if (free_fn) {
			free_fn(info);
		}
	}
	if (fifo->slice) {
		acl_slice_pool_free(__FILE__, __LINE__, fifo);
	} else {
		acl_myfree(fifo);
	}
}

void acl_fifo_push_info_back(ACL_FIFO *fifo, ACL_FIFO_INFO *info)
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

ACL_FIFO_INFO *acl_fifo_pop_info(ACL_FIFO *fifo)
{
	ACL_FIFO_INFO *info;

	if (fifo->head == NULL) {
		return NULL;
	}

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

void acl_fifo_delete_info(ACL_FIFO *fifo, ACL_FIFO_INFO *info)
{
	if (info->prev) {
		info->prev->next = info->next;
	} else {
		fifo->head = info->next;
	}
	if (info->next) {
		info->next->prev = info->prev;
	} else {
		fifo->tail = info->prev;
	}

	if (fifo->slice) {
		acl_slice_pool_free(__FILE__, __LINE__, info);
	} else {
		acl_myfree(info);
	}
	fifo->cnt--;
}

ACL_FIFO_INFO *acl_fifo_head_info(ACL_FIFO *fifo)
{
	if (fifo->head) {
		return fifo->head;
	} else {
		return NULL;
	}
}

ACL_FIFO_INFO *acl_fifo_tail_info(ACL_FIFO *fifo)
{
	if (fifo->tail) {
		return fifo->tail;
	} else {
		return NULL;
	}
}

int acl_fifo_size(ACL_FIFO *fifo)
{
	if (fifo) {
		return fifo->cnt;
	} else {
		return 0;
	}
}
