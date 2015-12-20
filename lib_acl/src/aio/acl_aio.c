#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "net/acl_net.h"
#include "aio/acl_aio.h"

#endif
#include "stdlib/acl_array.h"
#include "../event/events.h"
#include "aio.h"

void acl_aio_check(ACL_AIO *aio)
{
	aio_delay_check(aio);
}

void acl_aio_free(ACL_AIO *aio)
{
	acl_event_free(aio->event);
	acl_array_free(aio->dead_streams, NULL);
	acl_myfree(aio);
}

ACL_AIO *acl_aio_create(int event_mode)
{
	return acl_aio_create2(event_mode, 0);
}

ACL_AIO *acl_aio_create2(int event_mode, unsigned int nMsg)
{
	const char *myname = "acl_aio_create";
	char  ebuf[256];
	ACL_AIO *aio;

	aio = acl_mycalloc(1, sizeof(ACL_AIO));
	if (aio == NULL)
		acl_msg_fatal("%s: calloc error(%s)",
			myname, acl_last_strerror(ebuf, sizeof(ebuf)));

#ifdef ACL_WINDOWS
	aio->tid = acl_pthread_self();
#endif

	aio->delay_sec  = 1;
	aio->delay_usec = 0;
	aio->keep_read  = 1;
	aio->rbuf_size  = 8192;
	aio->event_mode = event_mode;

	switch (event_mode) {
	case ACL_EVENT_KERNEL:
		aio->event = acl_event_new_kernel(aio->delay_sec, aio->delay_usec);
		break;
	case ACL_EVENT_SELECT:
		aio->event = acl_event_new_select(aio->delay_sec, aio->delay_usec);
		break;
	case ACL_EVENT_POLL:
		aio->event = acl_event_new_poll(aio->delay_sec, aio->delay_usec);
		break;
	case ACL_EVENT_WMSG:
		aio->event = acl_event_new_wmsg(nMsg);
		break;
	default:
		acl_msg_fatal("%s(%d): event_mode(%d) not support",
			myname, __LINE__, event_mode);
		break;
	}
	aio->dead_streams = acl_array_create(aio->event->fdsize);
	return aio;
}

int acl_aio_event_mode(ACL_AIO *aio)
{
	return aio->event_mode;
}

int acl_aio_get_keep_read(ACL_AIO *aio)
{
	if (aio)
		return aio->keep_read;
	return 0;
}

void acl_aio_set_keep_read(ACL_AIO *aio, int onoff)
{
	if (aio)
		aio->keep_read = onoff;
}

int acl_aio_get_delay_sec(ACL_AIO *aio)
{
	if (aio)
		return (aio->delay_sec);
	return -1;
}

int acl_aio_get_delay_usec(ACL_AIO *aio)
{
	if (aio)
		return aio->delay_usec;
	return -1;
}

void acl_aio_set_delay_sec(ACL_AIO *aio, int delay_sec)
{
	if (aio) {
		aio->delay_sec = delay_sec;
		acl_event_set_delay_sec(aio->event, delay_sec);
	}
}

void acl_aio_set_delay_usec(ACL_AIO *aio, int delay_usec)
{
	if (aio) {
		aio->delay_usec = delay_usec;
		acl_event_set_delay_usec(aio->event, delay_usec);
	}
}

void acl_aio_set_check_inter(ACL_AIO *aio, int check_inter)
{
	if (aio && check_inter >= 0)
		acl_event_set_check_inter(aio->event, check_inter);
}

void acl_aio_loop(ACL_AIO *aio)
{	
	if (aio == NULL || aio->event == NULL)
		return;

	acl_event_loop(aio->event);
	aio_delay_check(aio);
}

ACL_EVENT *acl_aio_event(ACL_AIO *aio)
{
	if (aio)
		return aio->event;
	return NULL;
}

void acl_aio_set_rbuf_size(ACL_AIO *aio, int rbuf_size)
{
	aio->rbuf_size = rbuf_size;
}

/*---------------------------------------------------------------------------*/

acl_int64 acl_aio_request_timer(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME timer_fn,
	void *context, acl_int64 idle_limit, int keep)
{
	const char *myname = "acl_aio_request_timer";

	if (aio == NULL || aio->event == NULL || timer_fn == NULL)
		acl_msg_fatal("%s: input invalid", myname);

	return acl_event_request_timer(aio->event, timer_fn, context,
			idle_limit, keep);
}

acl_int64 acl_aio_cancel_timer(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME timer_fn, void *context)
{
	const char *myname = "acl_aio_cancel_timer";

	if (aio == NULL || aio->event == NULL || timer_fn == NULL)
		acl_msg_fatal("%s: input invalid", myname);

	return acl_event_cancel_timer(aio->event, timer_fn, context);
}

void acl_aio_keep_timer(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int onoff)
{
	const char *myname = "acl_aio_keep_timer";

	if (aio == NULL || aio->event == NULL)
		acl_msg_fatal("%s: input invalid", myname);
	acl_event_keep_timer(aio->event, callback, context, onoff);
}

int acl_aio_timer_ifkeep(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	const char *myname = "acl_aio_timer_ifkeep";

	if (aio == NULL || aio->event == NULL)
		acl_msg_fatal("%s: input invalid", myname);
	return acl_event_timer_ifkeep(aio->event, callback, context);
}
