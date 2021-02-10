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
	EVENT_THR *event_thr = (EVENT_THR *) eventp;
	acl_int64  when;

	if (delay < 0 || delay >= 4294963950LL) {
		acl_msg_panic("%s: invalid delay: %lld", __FUNCTION__, delay);
	}

	THREAD_LOCK(&event_thr->tm_mutex);

	when = event_timer_request(eventp, callback, context, delay, 0);

	THREAD_UNLOCK(&event_thr->tm_mutex);
	return when;
}

/* event_timer_cancel_thr - cancel timer */

acl_int64 event_timer_cancel_thr(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	EVENT_THR *event_thr = (EVENT_THR *) eventp;
	acl_int64  time_left;

	THREAD_LOCK(&event_thr->tm_mutex);

	time_left = event_timer_cancel(eventp, callback, context);

	THREAD_UNLOCK(&event_thr->tm_mutex);
	return time_left;
}

void event_timer_keep_thr(ACL_EVENT *eventp acl_unused,
	ACL_EVENT_NOTIFY_TIME callback acl_unused,
	void *context acl_unused, int keep acl_unused)
{
}

int  event_timer_ifkeep_thr(ACL_EVENT *eventp acl_unused,
	ACL_EVENT_NOTIFY_TIME callback acl_unused,
	void *context acl_unused)
{
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
	THREAD_LOCK(&event->tm_mutex);

	event_timer_trigger(&event->event);

	THREAD_UNLOCK(&event->tm_mutex);
}
