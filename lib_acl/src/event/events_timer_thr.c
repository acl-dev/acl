#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_ring.h"

#endif

#include "events.h"

/* event_timer_request_thr - (re)set timer */

acl_int64 event_timer_request_thr(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context,
	acl_int64 delay, int keep acl_unused)
{
	const char *myname = "event_timer_request_thr";
	EVENT_THR *event_thr = (EVENT_THR *) eventp;
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer = NULL;

	if (delay < 0 || delay >= 4294963950LL)
		acl_msg_panic("%s: invalid delay: %lld", myname, delay);

	THREAD_LOCK(&event_thr->tm_mutex);

	/*
	 * Make sure we schedule this event at the right time.
	 */
	SET_TIME(eventp->present);

	/*
	 * See if they are resetting an existing timer request. If so, take the
	 * request away from the timer queue so that it can be inserted at the
	 * right place.
	 */
	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			timer->when = eventp->present + delay;
			acl_ring_detach(iter.ptr);
			break;
		}
	}

	/*
	 * If not found, schedule a new timer request.
	 */
	if (iter.ptr == &eventp->timer_head) {
		timer = (ACL_EVENT_TIMER *) acl_mymalloc(sizeof(ACL_EVENT_TIMER));
		if (timer == NULL) {
			acl_msg_panic("%s: can't mymalloc for timer", myname);
		}
		timer->when       = eventp->present + delay;
		timer->delay      = delay;
		timer->callback   = callback;
		timer->context    = context;
		timer->event_type = ACL_EVENT_TIME;
		acl_ring_init(&timer->tmp);
	}

	/*
	 * Insert the request at the right place. Timer requests are kept sorted
	 * to reduce lookup overhead in the event loop.
	 */
	acl_ring_foreach(iter, &eventp->timer_head) {
		if (timer->when < RING_TO_TIMER(iter.ptr)->when) {
			break;
		}
	}

	acl_ring_prepend(iter.ptr, &timer->ring);
	THREAD_UNLOCK(&event_thr->tm_mutex);
	return timer->when;
}

/* event_timer_cancel_thr - cancel timer */

acl_int64 event_timer_cancel_thr(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	EVENT_THR *event_thr = (EVENT_THR *) eventp;
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer;
	acl_int64  time_left = -1;

	THREAD_LOCK(&event_thr->tm_mutex);

	/*
	 * See if they are canceling an existing timer request. Do not complain
	 * when the request is not found. It might have been canceled from some
	 * other thread.
	 */

	SET_TIME(eventp->present);

	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			if ((time_left = timer->when - eventp->present) < 0) {
				time_left = 0;
			}
			acl_ring_detach(&timer->ring);
			acl_ring_detach(&timer->tmp);
			acl_myfree(timer);
			break;
		}
	}

	THREAD_UNLOCK(&event_thr->tm_mutex);

	return (time_left);
}

void event_timer_keep_thr(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int keep)
{
	EVENT_THR *event_thr = (EVENT_THR *) eventp;
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer;

	THREAD_LOCK(&event_thr->tm_mutex);
	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			timer->keep = keep;
			break;
		}
	}
	THREAD_UNLOCK(&event_thr->tm_mutex);
}

int  event_timer_ifkeep_thr(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context)
{
	EVENT_THR *event_thr = (EVENT_THR *) eventp;
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer;

	THREAD_LOCK(&event_thr->tm_mutex);
	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			THREAD_UNLOCK(&event_thr->tm_mutex);
			return timer->keep;
		}
	}
	THREAD_UNLOCK(&event_thr->tm_mutex);
	return 0;
}

/*
 * Deliver timer events. Requests are sorted: we can stop when we
 * reach the future or the list end. Allow the application to update
 * the timer queue while it is being called back. To this end, we
 * repeatedly pop the first request off the timer queue before
 * delivering the event to the application.
 */
void event_timer_trigger_thr(EVENT_THR *event)
{
	ACL_EVENT *eventp = &event->event;
	ACL_EVENT_TIMER *timer;
	ACL_RING_ITER iter;
	ACL_RING *ring;
	ACL_EVENT_NOTIFY_TIME timer_fn;
	void *timer_arg;

	SET_TIME(eventp->present);

	THREAD_LOCK(&event->tm_mutex);

	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = RING_TO_TIMER(iter.ptr);
		if (timer->when > eventp->present) {
			break;
		}

		acl_ring_prepend(&eventp->timers, &timer->tmp);
	}

	THREAD_UNLOCK(&event->tm_mutex);

	while ((ring = acl_ring_pop_head(&eventp->timers)) != NULL) {
		timer     = TMP_TO_TIMER(ring);
		timer_fn  = timer->callback;
		timer_arg = timer->context;

		if (timer->delay > 0 && timer->keep) {
			timer->ncount++;
			eventp->timer_request(eventp, timer->callback,
				timer->context, timer->delay, timer->keep);
		} else {
			acl_ring_detach(&timer->ring);  /* first this */
			acl_myfree(timer);
		}

		timer_fn(ACL_EVENT_TIME, eventp, timer_arg);
	}
}
