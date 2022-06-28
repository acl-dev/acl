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

#include "../event/events.h"
#include "aio.h"

static void __accept_notify_callback(int event_type,
	ACL_EVENT *event acl_unused, ACL_VSTREAM *stream, void *context)
{
	const char *myname = "__accept_notify_callback";
	ACL_ASTREAM *astream = (ACL_ASTREAM *) context;
	ACL_ASTREAM *client;
	ACL_VSTREAM *cstream;
	int   i;

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		acl_msg_error("%s: listen error, sleep 1 second(%s)",
			myname, acl_last_serror());
		sleep(1);
		/* not reached here */
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		(void) aio_timeout_callback(astream);
		return;
	}

	if ((event_type & ACL_EVENT_READ) == 0) {
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);
	}

	for (i = 0; i < astream->accept_nloop; i++) {
		/* cstream read_buf 的长度 read_buf_len 继承自监听流的
		 * read_buf_len
		 */
		cstream = acl_vstream_accept(stream, NULL, 0);
		if (cstream == NULL) {
			int   ret;

			ret = acl_last_error();
			if (ret == ACL_EAGAIN || ret == ACL_ECONNABORTED) {
				break;
			}

			acl_msg_error("%s: listen exception, error(%s)",
				myname,	acl_last_serror());

			/* TODO: the listener should be restart again */
			astream->aio->event->disable_read_fn(
				astream->aio->event, astream->stream);
			break;
		}

		client = acl_aio_open(astream->aio, cstream);
		if (astream->accept_fn(client, astream->context) < 0) {
			acl_aio_iocp_close(client);
			acl_msg_warn("%s(%d): accept_fn return < 0, "
				"close client and break, err(%s)", myname,
				__LINE__, acl_last_serror());
			break;
		}
	}
}

void acl_aio_accept(ACL_ASTREAM *astream)
{
	const char *myname = "acl_aio_accept";

	if (astream == NULL) {
		acl_msg_fatal("%s: input invalid", myname);
	}

	astream->flag |= ACL_AIO_FLAG_ISRD;
	acl_event_enable_listen(astream->aio->event, astream->stream,
		astream->timeout, __accept_notify_callback, astream);
}

static void __listen_notify_callback(int event_type,
	ACL_EVENT *event acl_unused, ACL_VSTREAM *stream acl_unused,
	void *context)
{
	const char *myname = "__listen_notify_callback";
	ACL_ASTREAM *astream = (ACL_ASTREAM *) context;
	int   i;
	
	if ((event_type & ACL_EVENT_XCPT) != 0) {
		acl_msg_error("%s: listen error, sleep 1 second(%s)",
			myname, acl_last_serror());
		sleep(1);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		(void) aio_timeout_callback(astream);
		return;
	}

	for (i = 0; i < astream->accept_nloop; i++) {
		if (astream->listen_fn(astream,	astream->context) < 0) {
			break;
		}
	}
}

void acl_aio_listen(ACL_ASTREAM *astream)
{
	const char *myname = "acl_aio_listen";

	if (astream == NULL) {
		acl_msg_fatal("%s: input invalid", myname);
	}

	astream->flag |= ACL_AIO_FLAG_ISRD;
	acl_event_enable_listen(astream->aio->event, astream->stream,
		astream->timeout, __listen_notify_callback, astream);
}
