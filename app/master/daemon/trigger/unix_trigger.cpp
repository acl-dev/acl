#include "stdafx.h"

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

/* Process manager. */

#include "trigger.h"

#undef	__USE_TIMER

struct ACL_UNIX_TRIGGER {
	ACL_VSTREAM *stream;
	char        *service;
};

/* acl_unix_trigger_event - disconnect from peer */

static void acl_unix_trigger_event(int type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context)
{
	const char *myname = "acl_unix_trigger_event";
	struct ACL_UNIX_TRIGGER *up = (struct ACL_UNIX_TRIGGER *) context;

	/*
	 * Disconnect.
	 */
	if (type == ACL_EVENT_TIME)
		acl_msg_warn("%s: read timeout for service %s",
			myname, up->service);
	acl_event_disable_readwrite(event, stream);
#ifdef	__USE_TIMER
	acl_event_cancel_timer(event, acl_unix_trigger_event, up);
#endif
	if (acl_vstream_close(stream) < 0)
		acl_msg_warn("%s: close %s: %s",
			myname, up->service, strerror(errno));
	acl_myfree(up->service);
	acl_myfree(up);
}

#ifdef	__USE_TIMER
static void acl_unix_trigger_timer(int type, ACL_EVENT *event, void *context)
{
	struct ACL_UNIX_TRIGGER *up = (struct ACL_UNIX_TRIGGER *) context;

	acl_unix_trigger_event(type, event, up->stream, context);
}
#endif

/* acl_unix_trigger - wakeup UNIX-domain server */

int acl_unix_trigger(ACL_EVENT *event, const char *service,
	const char *buf, int len, int timeout)
{
	const char *myname = "acl_unix_trigger";
	struct ACL_UNIX_TRIGGER *up;
	ACL_SOCKET fd;

	if (acl_msg_verbose > 0)
		acl_msg_info("%s: service %s", myname, service);

	/*
	 * Connect...
	 */
	if ((fd = acl_unix_connect(service, ACL_BLOCKING, timeout)) < 0) {
		acl_msg_warn("%s: connect to %s: %s, timeout=%d",
			myname, service, strerror(errno), timeout);
		return -1;
	}
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	/*
	 * Stash away context.
	 */
	up = (struct ACL_UNIX_TRIGGER *) acl_mymalloc(sizeof(*up));
	up->service = acl_mystrdup(service);
	up->stream = acl_vstream_fdopen(fd, O_RDWR, 4096,
			timeout, ACL_VSTREAM_TYPE_LISTEN_UNIX);

	/*
	 * Write the request...
	 */
	if (acl_vstream_writen(up->stream, buf, len) < 0
		|| acl_vstream_writen(up->stream, "", 1) < 0)
	{
		if (acl_msg_verbose)
			acl_msg_warn("%s: write to %s: %s",
				myname, service, strerror(errno));
	}

	/*
	 * Wakeup when the peer disconnects, or when we lose patience.
	 */
#ifdef	__USE_TIMER
	if (timeout > 0)
		acl_event_request_timer(event, acl_unix_trigger_timer,
			(void *) up, (timeout + 100) * 1000000);
	acl_event_enable_read(event, up->stream, 0,
		acl_unix_trigger_event, (void *) up);
#else
	if (timeout > 0)
		acl_event_enable_read(event, up->stream, timeout + 100,
			acl_unix_trigger_event, (void *) up);
	else
		acl_event_enable_read(event, up->stream, 0,
			acl_unix_trigger_event, (void *) up);
#endif

	return 0;
}
