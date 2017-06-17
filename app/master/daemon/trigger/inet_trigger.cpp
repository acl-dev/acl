#include "stdafx.h"

#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Process manager. */

#include "trigger.h"

struct ACL_INET_TRIGGER {
	int      fd;
	ACL_EVENT   *eventp;
	ACL_VSTREAM *stream;
	char    *service;
};

/* ACL_INET_TRIGGER_event - disconnect from peer */

static void acl_inet_trigger_event(int type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context)
{
	const char *myname = "acl_inet_trigger_event";
	struct ACL_INET_TRIGGER *ip = (struct ACL_INET_TRIGGER *) context;

	/*
	 * Disconnect.
	 */
	if (type == ACL_EVENT_TIME)
		acl_msg_warn("%s: read timeout for service %s",
			myname, ip->service);
	acl_event_disable_readwrite(event, stream);
	/*
	 * acl_event_cancel_timer(ip->eventp, acl_inet_trigger_event, context);
	 */
	if (acl_vstream_close(ip->stream) < 0)
		acl_msg_warn("%s: close %s: %s",
			myname, ip->service, strerror(errno));
	acl_myfree(ip->service);
	acl_myfree(ip);
}


/* acl_inet_trigger - wakeup INET-domain server */

int acl_inet_trigger(ACL_EVENT *eventp, const char *service,
	const char *buf, int len, int timeout)
{
	const char *myname = "acl_inet_trigger";
	struct ACL_INET_TRIGGER *ip;
	int     fd;

	if (acl_msg_verbose > 1)
		acl_msg_info("%s: service %s", myname, service);

	/*
	 * Connect...
	 */
	if ((fd = acl_inet_connect(service, ACL_BLOCKING, timeout)) < 0) {
		if (acl_msg_verbose)
			acl_msg_warn("%s: connect to %s: %s",
				myname, service, strerror(errno));
		return (-1);
	}
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	/*
	 * Stash away context.
	 */
	ip = (struct ACL_INET_TRIGGER *) acl_mymalloc(sizeof(*ip));
	ip->fd = fd;
	ip->service = acl_mystrdup(service);
	ip->stream = acl_vstream_fdopen(fd, O_RDWR, 4096,
		timeout, ACL_VSTREAM_TYPE_LISTEN_INET);
	ip->eventp = eventp;

	/*
	 * Write the request...
	 */
	if (acl_write_buf(fd, buf, len, timeout) < 0
		|| acl_write_buf(fd, "", 1, timeout) < 0)
	{
		if (acl_msg_verbose)
			acl_msg_warn("%s: write to %s: %s",
				myname, service, strerror(errno));
	}

	/*
	 * Wakeup when the peer disconnects, or when we lose patience.
	 */

	if (timeout > 0)
		acl_event_enable_read(ip->eventp, ip->stream, timeout + 100,
			acl_inet_trigger_event, (void *) ip);
	else
		acl_event_enable_read(ip->eventp, ip->stream, 0,
			acl_inet_trigger_event, (void *) ip);

	return (0);
}
