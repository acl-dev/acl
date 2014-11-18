#include "lib_acl.h"

#include "service.h"
#include "http_service.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

static int io_close_fn(ACL_ASTREAM *astream, void *context)
{
	const char *myname = "io_close_fn";

	CLIENT_ENTRY *entry = (CLIENT_ENTRY *) context;
	ACL_VSTREAM *stream;

	stream = acl_aio_vstream(astream);
	if (stream)
		acl_debug(22, 2) ("%s: error to stream", myname);
	else
		acl_msg_error("not connected");

	forward_complete(entry);
	return (-1);
}

static int io_timeout_fn(ACL_ASTREAM *astream, void *context acl_unused)
{
	const char *myname = "io_timeout_fn";
	ACL_VSTREAM *stream;

	stream = acl_aio_vstream(astream);
	if (stream)
		acl_msg_info("%s: timeout to stream", myname);
	else
		acl_msg_error("not connected");
	return (-1);
}

#if 0
static int write_callback(ACL_ASTREAM *stream, void *context)
{
	const char *myname = "write_callback";

	if (acl_aio_iswset(stream)) {
		acl_msg_fatal("%s: acl_aio_iswset return true", myname);
	}

	return (0);
}
#endif

static int read_callback(ACL_ASTREAM *astream, void *context,
		const char *data, int dlen)
{
	const char *myname = "read_callback";
	CLIENT_ENTRY *entry = (CLIENT_ENTRY *) context;

/*
	if (acl_aio_isrset(astream)) {
		acl_msg_fatal("%s: acl_aio_isrset return true", myname);
	}
*/

	if (dlen <= 0)
		acl_msg_fatal("%s: dlen(%d) invalid", myname, dlen);

	if (astream == entry->client) {
		if (entry->server == NULL) {
			acl_msg_error("%s: server null", myname);
			return (-1);
		}
		acl_aio_writen(entry->server, data, dlen);
	} else if (astream == entry->server) {
		if (entry->client == NULL) {
			acl_msg_error("%s: client null", myname);
			return (-1);
		}
		acl_aio_writen(entry->client, data, dlen);
	} else {
		acl_msg_error("%s: stream invalid", myname);
	}

	acl_aio_read(astream);

	return (0);
}

void tcp_start(CLIENT_ENTRY *entry)
{
	if (entry->client == NULL || entry->server == NULL)
		return;
	acl_aio_ctl(entry->client,
		ACL_AIO_CTL_READ_HOOK_ADD, read_callback, entry,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, io_close_fn, entry,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, io_timeout_fn, entry,
		ACL_AIO_CTL_TIMEOUT, entry->service->rw_timeout,
		ACL_AIO_CTL_CTX, entry,
		ACL_AIO_CTL_END);

	acl_aio_ctl(entry->server,
		ACL_AIO_CTL_READ_HOOK_ADD, read_callback, entry,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, io_close_fn, entry,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, io_timeout_fn, entry,
		ACL_AIO_CTL_TIMEOUT, entry->service->rw_timeout,
		ACL_AIO_CTL_CTX, entry,
		ACL_AIO_CTL_END);

	acl_aio_read(entry->client);
	acl_aio_read(entry->server);
}
