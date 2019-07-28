/*++
 * NAME
 *	attr_clnt 3
 * SUMMARY
 *	attribute query-reply client
 * SYNOPSIS
 *	#include <attr_clnt.h>
 *
 *	typedef int (*ATTR_CLNT_PRINT_FN) (VSTREAM *, int, va_list);
 *	typedef int (*ATTR_CLNT_SCAN_FN) (VSTREAM *, int, va_list);
 *
 *	ATTR_CLNT *attr_clnt_create(server, timeout, max_idle, max_ttl)
 *	const char *server;
 *	int	timeout;
 *	int	max_idle;
 *	int	max_ttl;
 *
 *	int	attr_clnt_request(client,
 *			send_flags, send_type, send_name, ..., ATTR_TYPE_END,
 *			recv_flags, recv_type, recv_name, ..., ATTR_TYPE_END)
 *	ATTR_CLNT *client;
 *	int	send_flags;
 *	int	send_type;
 *	const char *send_name;
 *	int	recv_flags;
 *	int	recv_type;
 *	const char *recv_name;
 *
 *	void	attr_clnt_free(client)
 *	ATTR_CLNT *client;
 *
 *	void	attr_clnt_control(client, name, value, ... ATTR_CLNT_CTL_END)
 *	ATTR_CLNT *client;
 *	int	name;
 * DESCRIPTION
 *	This module implements a client for a simple attribute-based
 *	protocol. The default protocol is described in attr_scan_plain(3).
 *
 *	attr_clnt_create() creates a client handle. See auto_clnt(3) for
 *	a description of the arguments.
 *
 *	attr_clnt_request() sends the specified request attributes and
 *	receives a reply. The reply argument specifies a name-value table.
 *	The other arguments are as described in attr_print_plain(3). The
 *	result is the number of attributes received or -1 in case of trouble.
 *
 *	attr_clnt_free() destroys a client handle and closes its connection.
 *
 *	attr_clnt_control() allows the user to fine tune the behavior of
 *	the specified client. The arguments are a list of (name, value)
 *	terminated with ATTR_CLNT_CTL_END.
 *	The following lists the names and the types of the corresponding
 *	value arguments.
 * .IP "ATTR_CLNT_CTL_PROTO(ATTR_CLNT_PRINT_FN, ATTR_CLNT_SCAN_FN)"
 *	Specifies alternatives for the attr_plain_print() and
 *	attr_plain_scan() functions.
 * DIAGNOSTICS
 *	Warnings: communication failure.
 * SEE ALSO
 *	auto_clnt(3), client endpoint management
 *	attr_scan_plain(3), attribute protocol
 *	attr_print_plain(3), attribute protocol
 * LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#include "StdAfx.h"
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <errno.h>

#include "attr.h"
#include "auto_clnt.h"
#include "attr_clnt.h"

/* Application-specific. */

struct ATTR_CLNT {
    AUTO_CLNT *auto_clnt;
    ATTR_CLNT_PRINT_FN print;
    ATTR_CLNT_SCAN_FN scan;
};

/* attr_clnt_free - destroy attribute client */

void    attr_clnt_free(ATTR_CLNT *client)
{
    auto_clnt_free(client->auto_clnt);
    acl_myfree(client);
}

/* attr_clnt_create - create attribute client */

ATTR_CLNT *attr_clnt_create(ACL_EVENT *eventp, const char *service,
	int timeout, int max_idle, int max_ttl)
{
    ATTR_CLNT *client;

    client = (ATTR_CLNT *) acl_mymalloc(sizeof(*client));
    client->auto_clnt = auto_clnt_create(eventp, service,
	    timeout, max_idle, max_ttl);
    client->scan = attr_vscan_plain;
    client->print = attr_vprint_plain;
    return (client);
}

/* attr_clnt_request - send query, receive reply */

int     attr_clnt_request(ATTR_CLNT *client, int send_flags,...)
{
    const char *myname = "attr_clnt_request";
    ACL_VSTREAM *stream;
    int     count = 0;
    va_list ap;
    int     type;
    int     recv_flags;
    int     err;
    int     ret;

    /*
     * XXX If the stream is readable before we send anything, then assume the
     * remote end disconnected.
     * 
     * XXX For some reason we can't simply call the scan routine after the print
     * routine, that messes up the argument list.
     */
#define SKIP_ARG(ap, type) { \
    (void) va_arg(ap, char *); \
    (void) va_arg(ap, type); \
}
#define SKIP_ARG2(ap, t1, t2) { \
    SKIP_ARG(ap, t1); \
    (void) va_arg(ap, t2); \
}

    for (;;) {
	errno = 0;
	if ((stream = auto_clnt_access(client->auto_clnt)) != 0
		&& acl_readable(ACL_VSTREAM_SOCK(stream)) == 0) {
	    errno = 0;
	    va_start(ap, send_flags);
	    err = (client->print(stream, send_flags, ap) == ACL_VSTREAM_EOF
		    || acl_vstream_fflush(stream) == ACL_VSTREAM_EOF);
	    va_end(ap);
	    if (err == 0) {
		va_start(ap, send_flags);
		while ((type = va_arg(ap, int)) != ATTR_TYPE_END) {
		    switch (type) {
		    case ATTR_TYPE_STR:
			SKIP_ARG(ap, char *);
			break;
		    case ATTR_TYPE_DATA:
			SKIP_ARG2(ap, ssize_t, char *);
			break;
		    case ATTR_TYPE_INT:
			SKIP_ARG(ap, int);
			break;
		    case ATTR_TYPE_LONG:
			SKIP_ARG(ap, long);
			break;
		    case ATTR_TYPE_HASH:
			(void) va_arg(ap, ACL_HTABLE *);
			break;
		    default:
			acl_msg_panic("%s: unexpected attribute type %d", myname, type);
		    }
		}
		recv_flags = va_arg(ap, int);
		ret = client->scan(stream, recv_flags, ap);
		va_end(ap);
		if (ret > 0)
		    return (ret);
	    }
	}
	if (++count >= 2 || acl_msg_verbose
		|| (errno && errno != EPIPE && errno != ENOENT && errno != ACL_ECONNRESET))
	    acl_msg_warn("%s: problem talking to server %s: %s",
		    myname, auto_clnt_name(client->auto_clnt), acl_last_serror());
	if (count >= 2)
	    return (-1);
	sleep(1);				/* XXX make configurable */
	auto_clnt_recover(client->auto_clnt);
    }
}

/* attr_clnt_control - fine control */

void    attr_clnt_control(ATTR_CLNT *client, int name,...)
{
    const char *myname = "attr_clnt_control";
    va_list ap;

    for (va_start(ap, name); name != ATTR_CLNT_CTL_END; name = va_arg(ap, int)) {
	switch (name) {
	    case ATTR_CLNT_CTL_PROTO:
		client->print = va_arg(ap, ATTR_CLNT_PRINT_FN);
		client->scan = va_arg(ap, ATTR_CLNT_SCAN_FN);
		break;
	    default:
		acl_msg_panic("%s: bad name %d", myname, name);
	}
    }
    va_end(ap);
}
