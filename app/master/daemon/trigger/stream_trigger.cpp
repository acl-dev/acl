#include "stdafx.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

/* Process manager. */

#include "trigger.h"

#ifdef SUNOS5

struct ACL_STREAM_TRIGGER {
	int      fd;
	ACL_VSTREAM *stream;
	ACL_EVENT   *eventp;
	char    *service;
};

/* acl_stream_trigger_event - disconnect from peer */

static void acl_stream_trigger_event(int event, ACL_EVENT *eventp acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	const char *myname = "acl_stream_trigger_event";
	struct ACL_STREAM_TRIGGER *sp = (struct ACL_STREAM_TRIGGER *) context;

	/* Disconnect. */
	if (event == ACL_EVENT_TIME)
		acl_msg_warn("%s: read timeout for service %s",
			myname, sp->service);
	acl_event_disable_readwrite(sp->eventp, sp->stream);

	/*
	 * acl_event_cancel_timer(sp->eventp, acl_stream_trigger_event, context);
	 */
	if (acl_vstream_close(sp->stream) < 0)
		acl_msg_warn("%s: close %s: %s",
			myname, sp->service, strerror(errno));
	acl_myfree(sp->service);
	acl_myfree(sp);
}

/* acl_stream_trigger - wakeup stream server */

int acl_stream_trigger(ACL_EVENT *eventp, const char *service,
	const char *buf,int len, int timeout)
{
	const char *myname = "acl_stream_trigger";
	struct ACL_STREAM_TRIGGER *sp;
	int     fd;

	if (acl_msg_verbose > 1)
		acl_msg_info("%s: service %s", myname, service);

	/* Connect... */
	if ((fd = acl_stream_connect(service, ACL_BLOCKING, timeout)) < 0) {
		if (acl_msg_verbose)
			acl_msg_warn("%s: connect to %s: %s",
				myname, service, strerror(errno));
		return -1;
	}
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	/* Stash away context. */
	sp = (struct ACL_STREAM_TRIGGER *) acl_mymalloc(sizeof(*sp));
	sp->fd = fd;
	sp->service = acl_mystrdup(service);
	sp->stream = acl_vstream_fdopen(fd, O_RDWR, 4096,
			timeout, ACL_VSTREAM_TYPE_SOCK);
	sp->eventp = eventp;

	/* Write the request... */
	if (acl_write_buf(ACL_VSTREAM_SOCK(sp->stream), buf, len, timeout) < 0
		|| acl_write_buf(ACL_VSTREAM_SOCK(sp->stream), "", 1, timeout) < 0)
	{
		if (acl_msg_verbose)
			acl_msg_warn("%s: write to %s: %s",
				myname, service, strerror(errno));
	}

	/* Wakeup when the peer disconnects, or when we lose patience. */

	if (timeout > 0)
		acl_event_enable_read(sp->eventp, sp->stream, timeout + 100,
			acl_stream_trigger_event, (void *) sp);
	else
		acl_event_enable_read(sp->eventp, sp->stream, 0,
			acl_stream_trigger_event, (void *) sp);

	return 0;
}
#endif /* SUNOS5 */
