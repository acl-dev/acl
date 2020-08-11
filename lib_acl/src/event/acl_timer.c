#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <time.h>
#include "stdlib/acl_ring.h"
#include "stdlib/acl_mymalloc.h"
#include "event/acl_timer.h"

#endif

#include "events.h"

#define ring_to_timer(r) \
	((ACL_TIMER_INFO *) ((char *) (r) - offsetof(ACL_TIMER_INFO, entry)))

#define first_timer(head) \
	(acl_ring_succ(head) != (head) ? ring_to_timer(acl_ring_succ(head)) : 0)

acl_int64 acl_timer_request(ACL_TIMER* timer, void *obj, acl_int64 delay)
{
	ACL_RING_ITER iter;
	ACL_TIMER_INFO *pTimerItem = NULL;

	SET_TIME(timer->present);

	acl_ring_foreach(iter, &timer->timer_header) {
		pTimerItem = ring_to_timer(iter.ptr);
		if (pTimerItem->obj == obj) {
			pTimerItem->when = timer->present + delay;
			acl_ring_detach(iter.ptr);
			break;
		}
	}

	/* If not found, schedule a new timer request. */
	if (iter.ptr == &timer->timer_header) {
		pTimerItem = (ACL_TIMER_INFO*)
			acl_mycalloc(1, sizeof(ACL_TIMER_INFO));
		pTimerItem->when = timer->present + delay;
		pTimerItem->obj = obj;
	}

	acl_ring_foreach(iter, &timer->timer_header) {
		ACL_TIMER_INFO *pItem = ring_to_timer(iter.ptr);
		if (pTimerItem->when < pItem->when)
			break;
	}
	acl_ring_prepend(iter.ptr, &pTimerItem->entry);

	return (pTimerItem->when);
}

acl_int64 acl_timer_cancel(ACL_TIMER* timer, void *obj)
{
	ACL_RING_ITER iter;
	acl_int64  time_left = -1;

	SET_TIME(timer->present);

	acl_ring_foreach(iter, &timer->timer_header) {
		ACL_TIMER_INFO *pItem = ring_to_timer(iter.ptr);
		if (pItem->obj == obj) {
			if ((time_left = pItem->when - timer->present) < 0)
				time_left = 0;
			acl_ring_detach(iter.ptr);
			acl_myfree(pItem);
			break;
		}
	}

	timer->time_left = time_left;
	return (time_left);
}

void *acl_timer_popup(ACL_TIMER* timer)
{
	ACL_TIMER_INFO *pTimerItem;
	void *obj;

	SET_TIME(timer->present);

	pTimerItem = first_timer(&timer->timer_header);
	if (pTimerItem == NULL)
		return (NULL);

	if (pTimerItem->when > timer->present)
		return (NULL);

	acl_ring_detach(&pTimerItem->entry);		/* first this */
	obj = pTimerItem->obj;
	acl_myfree(pTimerItem);

	return (obj);
}

acl_int64 acl_timer_left(ACL_TIMER* timer)
{
	ACL_TIMER_INFO *pTimerItem;
	acl_int64  time_left = -1;

	SET_TIME(timer->present);

	pTimerItem = first_timer(&timer->timer_header);
	if (pTimerItem != NULL) {
		time_left = pTimerItem->when - timer->present;
		/* 如果下一个定时任务已经到期，则将剩余时间设定为0，
		 * 以使定时器尽快触发到期定时任务
		 */
		if (time_left < 0)
			time_left = 0;
	} else {
		/* 如果当时定时器没有定时任务，则将时间值赋为 -1 */
		time_left = -1;
	}

	return (time_left);
}

void acl_timer_walk(ACL_TIMER *timer, void (*action)(ACL_TIMER_INFO *, void *), void *arg)
{
	ACL_TIMER_INFO *info;
	ACL_RING_ITER iter;

	acl_ring_foreach(iter, &timer->timer_header) {
		info = ring_to_timer(iter.ptr);
		action(info, arg);
	}
}

static const void *timer_iter_head(ACL_ITER *iter, struct ACL_TIMER *timer)
{
	ACL_TIMER_INFO *info;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->i = 0;
	iter->size = timer->timer_header.len;
	iter->ptr = acl_ring_succ(&timer->timer_header);
	if (iter->ptr == &timer->timer_header) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}
	info = ring_to_timer(iter->ptr);
	iter->data = info->obj;
	return (iter->ptr);
}

static const void *timer_iter_next(ACL_ITER *iter, struct ACL_TIMER *timer)
{
	ACL_TIMER_INFO *info;

	iter->i++;
	iter->ptr = acl_ring_succ((ACL_RING*) iter->ptr);
	if (iter->ptr == &timer->timer_header) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}
	info = ring_to_timer(iter->ptr);
	iter->data = info->obj;
	return (iter->ptr);
}

static const void *timer_iter_tail(ACL_ITER *iter, struct ACL_TIMER *timer)
{
	ACL_TIMER_INFO *info;

	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;
	iter->size = timer->timer_header.len;
	iter->i = iter->size - 1;
	iter->ptr = acl_ring_pred(&timer->timer_header);
	if (iter->ptr == &timer->timer_header) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}
	info = ring_to_timer(iter->ptr);
	iter->data = info->obj;
	return (iter->ptr);

}

static const void *timer_iter_prev(ACL_ITER *iter, struct ACL_TIMER *timer)
{
	ACL_TIMER_INFO *info;

	iter->i--;
	iter->ptr = acl_ring_pred((ACL_RING*) iter->ptr);
	if (iter->ptr == &timer->timer_header) {
		iter->data = iter->ptr = NULL;
		return (NULL);
	}
	info = ring_to_timer(iter->ptr);
	iter->data = info->obj;
	return (iter->ptr);
}

static const ACL_TIMER_INFO *timer_iter_info(ACL_ITER *iter, struct ACL_TIMER *timer)
{
	ACL_TIMER_INFO *info;

	if (iter->ptr == NULL || iter->ptr == &timer->timer_header)
		return (NULL);

	info = ring_to_timer(iter->ptr);
	return (info);
}

ACL_TIMER *acl_timer_new()
{
	ACL_TIMER *timer;

	timer = (ACL_TIMER*) acl_mycalloc(1, sizeof(ACL_TIMER));
	timer->request = acl_timer_request;
	timer->cancel = acl_timer_cancel;
	timer->popup = acl_timer_popup;

	timer->iter_head = timer_iter_head;
	timer->iter_next = timer_iter_next;
	timer->iter_tail = timer_iter_tail;
	timer->iter_prev = timer_iter_prev;
	timer->iter_info = timer_iter_info;

	acl_ring_init(&timer->timer_header);

	return (timer);
}

void acl_timer_free(ACL_TIMER* timer, void (*free_fn)(void *))
{
	ACL_TIMER_INFO *info;

	while ((info = first_timer(&timer->timer_header)) != NULL) {
		if (free_fn)
			free_fn(info->obj);
		acl_ring_detach(&info->entry);
		acl_myfree(info);
	}

	acl_myfree(timer);
}

int acl_timer_size(ACL_TIMER *timer)
{
	return (acl_ring_size(&timer->timer_header));
}
