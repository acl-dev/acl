
#include "stdlib/acl_define.h"
#include <stdlib.h>
#include "stdlib/acl_malloc.h"
#include "stdlib/acl_msg.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "fifo.h"

typedef struct FIFO_INFO FIFO_INFO;

struct FIFO_INFO {
	void *data;
	FIFO_INFO *prev;
	FIFO_INFO *next;
};

struct FIFO_ITER {
	FIFO *fifo;
	FIFO_INFO *info_curr;
};

struct FIFO {
	FIFO_INFO *head;
	FIFO_INFO *tail;
	FIFO_ITER  iter;
	int   cnt;
};

FIFO *fifo_new(void)
{
	FIFO *fifo;

	fifo = (FIFO *) acl_default_calloc(__FILE__, __LINE__, 1, sizeof(*fifo));
	fifo->iter.fifo = fifo;
	fifo->iter.info_curr = NULL;

	return (fifo);
}

void fifo_free(FIFO *fifo, void (*free_fn)(void *))
{
	void *data;

	if (fifo) {
		while ((data = fifo_pop(fifo)) != NULL) {
			if (free_fn)
				free_fn(data);
		}

		acl_default_free(__FILE__, __LINE__, fifo);
	}
}

void fifo_push(FIFO *fifo, void *data)
{
	const char *myname = "fifo_push";
	FIFO_INFO *info;

	if (fifo == NULL) {
		acl_msg_error("%s: fifo null", myname);
		return;
	}

	if (data == NULL) {
		acl_msg_error("%s: data null", myname);
		return;
	}

	info = (FIFO_INFO *) acl_default_malloc(__FILE__, __LINE__, sizeof(*info));
	info->data = data;
	info->prev = NULL;
	info->next = NULL;

	if (fifo->head == NULL) {
		fifo->head = info;
		fifo->tail = info;
	} else {
		fifo->tail->next = info;
		info->prev = fifo->tail;
		fifo->tail = info;
	}

	fifo->cnt++;
}

void *fifo_pop(FIFO *fifo)
{
	const char *myname = "fifo_pop";
	FIFO_INFO *info;
	void *data;

	if (fifo == NULL) {
		acl_msg_error("%s: fifo null", myname);
		return (NULL);
	}

	info = fifo->head;
	if (info == NULL)
		return (NULL);

	if (fifo->head->next)
		fifo->head->next->prev = NULL;
	fifo->head = fifo->head->next;
	if (fifo->head == NULL)
		fifo->tail = NULL;

	data = info->data;
	acl_default_free(__FILE__, __LINE__, info);

	fifo->cnt--;

	return (data);
}

void *fifo_head(FIFO *fifo)
{
	if (fifo && fifo->head)
		return (fifo->head->data);
	else
		return (NULL);
}

void *fifo_tail(FIFO *fifo)
{
	if (fifo && fifo->tail)
		return (fifo->tail->data);
	else
		return (NULL);
}

FIFO_ITER *fifo_iterator_head(FIFO *fifo)
{
	if (fifo && fifo->head) {
		fifo->iter.info_curr = fifo->head;
		return (&fifo->iter);
	} else
		return (NULL);
}

FIFO_ITER *fifo_iterator_next(FIFO_ITER *iter)
{
	if (iter == NULL)
		return (NULL);

	iter->info_curr = iter->info_curr->next;
	if (iter->info_curr == NULL)
		return (NULL);
	return (iter);
}

FIFO_ITER *fifo_iterator_tail(FIFO *fifo)
{
	if (fifo && fifo->tail) {
		fifo->iter.info_curr = fifo->tail;
		return (&fifo->iter);
	} else
		return (NULL);
}

FIFO_ITER *fifo_iterator_prev(FIFO_ITER *iter)
{
	if (iter == NULL)
		return (NULL);

	iter->info_curr = iter->info_curr->prev;
	if (iter->info_curr == NULL)
		return (NULL);
	return (iter);
}

void *fifo_iterator_data(FIFO_ITER *iter)
{
	if (iter == NULL)
		return (NULL);

	return (iter->info_curr->data);
}

FIFO_ITER *fifo_iterator_delete(FIFO_ITER *iter, void (*free_fn)(void *))
{
	FIFO_INFO *info = iter->info_curr, *prev, *next;
	FIFO *fifo;

	if (iter == NULL || info == NULL) {
		iter->info_curr = NULL;
		return (NULL);
	}

	fifo = iter->fifo;

	prev = info->prev;
	next = info->next;

	fifo->cnt--;

	if (info == fifo->head) {
		fifo->head = next;
		if (fifo->head == NULL)
			fifo->tail = NULL;
	} else if (info == fifo->tail) {
		fifo->tail = prev;
		if (fifo->tail == NULL)
			fifo->head = NULL;
	}

	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;

	if (info->data && free_fn)
		free_fn(info->data);
	acl_default_free(__FILE__, __LINE__, info);

	if (next == NULL)
		return (NULL);
	iter->info_curr = next;
	return (iter);
}

int fifo_size(FIFO *fifo)
{
	if (fifo)
		return (fifo->cnt);
	else
		return (0);
}
