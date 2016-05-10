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
#include "event/acl_events.h"

#endif

#include "events.h"

/* event_timer_request - (re)set timer */

acl_int64 event_timer_request(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, acl_int64 delay, int keep)
{
	const char *myname = "event_timer_request";
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer = NULL;

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
		timer = ACL_RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			timer->when = eventp->present + delay;
			timer->keep = keep;
			timer->nrefer++;
			acl_ring_detach(iter.ptr);
			timer->nrefer--;
			break;
		}
	}

	/*
	 * If not found, schedule a new timer request.
	 */
	if (iter.ptr == &eventp->timer_head) {
		timer = (ACL_EVENT_TIMER *) acl_mymalloc(sizeof(ACL_EVENT_TIMER));
		if (timer == NULL)
			acl_msg_panic("%s: can't mymalloc for timer", myname);
		timer->when = eventp->present + delay;
		timer->delay = delay;
		timer->callback = callback;
		timer->context = context;
		timer->event_type = ACL_EVENT_TIME;
		timer->nrefer = 1;
		timer->ncount = 0;
		timer->keep = keep;
	}

	/*
	 * Insert the request at the right place. Timer requests are kept sorted
	 * to reduce lookup overhead in the event loop.
	 */

	acl_ring_foreach(iter, &eventp->timer_head)
		if (timer->when < ACL_RING_TO_TIMER(iter.ptr)->when)
			break;
	if (iter.ptr == &timer->ring)
		acl_msg_fatal("%s: ring invalid", myname);

	acl_ring_prepend(iter.ptr, &timer->ring);

	return (timer->when);
}

/* event_timer_cancel - cancel timer */

acl_int64 event_timer_cancel(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	const char *myname = "event_timer_cancel";
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer;
	acl_int64  time_left = -1;

	/*
	 * See if they are canceling an existing timer request. Do not complain
	 * when the request is not found. It might have been canceled from some
	 * other thread.
	 */

	SET_TIME(eventp->present);

	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = ACL_RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			if ((time_left = timer->when - eventp->present) < 0)
				time_left = 0;
			acl_ring_detach(&timer->ring);
			timer->nrefer--;
			if (timer->nrefer != 0)
				acl_msg_fatal("%s(%d): timer's nrefer(%d) != 0",
					myname, __LINE__, timer->nrefer);
			acl_myfree(timer);
			break;
		}
	}
	if (acl_msg_verbose > 2)
		acl_msg_info("%s: 0x%p 0x%p %lld", myname,
			callback, context, time_left);
	return (time_left);
}

void event_timer_keep(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int keep)
{
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer;

	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = ACL_RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context) {
			timer->keep = keep;
			break;
		}
	}
}

int  event_timer_ifkeep(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context)
{
	ACL_RING_ITER iter;
	ACL_EVENT_TIMER *timer;

	acl_ring_foreach(iter, &eventp->timer_head) {
		timer = ACL_RING_TO_TIMER(iter.ptr);
		if (timer->callback == callback && timer->context == context)
			return timer->keep;
	}

	return 0;
}
