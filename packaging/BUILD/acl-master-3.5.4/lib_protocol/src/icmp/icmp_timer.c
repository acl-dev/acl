#include "StdAfx.h"
#include "icmp_struct.h"
#include "icmp_private.h"

typedef struct TimerItem
{
	ICMP_PKT *pkt;
	ACL_RING entry;		/* 内部用的定时链 */
	time_t when;		/* 被触发的时间截 */
} TimerItem;

#define RING_TO_TIMER(r) \
	((TimerItem *) ((char *) (r) - offsetof(TimerItem, entry)))

#define FIRST_TIMER(head) \
	(acl_ring_succ(head) != (head) ? RING_TO_TIMER(acl_ring_succ(head)) : 0)

static time_t timer_request(ICMP_TIMER* timer, ICMP_PKT *pkt, int delay)
{
	ACL_RING_ITER iter;
	TimerItem *pTimerItem = NULL;

	time(&timer->present);

	acl_ring_foreach(iter, &timer->timer_header) {
		pTimerItem = RING_TO_TIMER(iter.ptr);
		if (pTimerItem->pkt == pkt) {
			pTimerItem->when = timer->present + delay;
			acl_ring_detach(iter.ptr);
			break;
		}
	}

	/* If not found, schedule a new timer request. */
	if (iter.ptr == &timer->timer_header) {
		pTimerItem = (TimerItem*) acl_mycalloc(1, sizeof(TimerItem));
		pTimerItem->when = timer->present + delay;
		pTimerItem->pkt = pkt;
	}

	acl_ring_foreach(iter, &timer->timer_header) {
		TimerItem *pItem = RING_TO_TIMER(iter.ptr);
		if (pTimerItem->when < pItem->when)
			break;
	}
	acl_ring_prepend(iter.ptr, &pTimerItem->entry);

	return (pTimerItem->when);
}

static time_t timer_cancel(ICMP_TIMER* timer, ICMP_PKT *pkt)
{
	ACL_RING_ITER iter;
	time_t  time_left = -1;

	time(&timer->present);

	acl_ring_foreach(iter, &timer->timer_header) {
		TimerItem *pItem = RING_TO_TIMER(iter.ptr);
		if (pItem->pkt == pkt) {
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

static ICMP_PKT* timer_find_delete(ICMP_TIMER* timer, unsigned short seq)
{
	ACL_RING_ITER iter;
	time_t  time_left = -1;
	ICMP_PKT* pkt  = NULL;

	acl_ring_foreach(iter, &timer->timer_header) {
		TimerItem *pItem = RING_TO_TIMER(iter.ptr);
		if (pItem->pkt->hdr.seq == seq) {
			pkt = pItem->pkt;

			if ((time_left = pItem->when - timer->present) < 0)
				time_left = 0;
			acl_ring_detach(iter.ptr);
			acl_myfree(pItem);
			break;
		}
	}

	return (pkt);
}

static ICMP_PKT* timer_popup(ICMP_TIMER* timer)
{
	TimerItem *pTimerItem;
	ICMP_PKT *pkt;

	time(&timer->present);

	pTimerItem = FIRST_TIMER(&timer->timer_header);
	if (pTimerItem == NULL)
		return (NULL);

	if (pTimerItem->when > timer->present)
		return (NULL);

	acl_ring_detach(&pTimerItem->entry);		/* first this */
	pkt = pTimerItem->pkt;
	acl_myfree(pTimerItem);

	return (pkt);
}

ICMP_TIMER *icmp_timer_new()
{
	ICMP_TIMER *timer;

	timer = (ICMP_TIMER*) acl_mycalloc(1, sizeof(ICMP_TIMER));
	timer->request = timer_request;
	timer->cancel = timer_cancel;
	timer->popup = timer_popup;
	timer->find_delete = timer_find_delete;

	acl_ring_init(&timer->timer_header);

	return (timer);
}

void icmp_timer_free(ICMP_TIMER* timer)
{
	acl_myfree(timer);
}
