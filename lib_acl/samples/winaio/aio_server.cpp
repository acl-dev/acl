#include "StdAfx.h"
#include "lib_acl.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "aio_server.h"

#define	USE_GETS

/* forward functions */

static char  __data[1024];
static int   __dlen;
static int   __echo_src;

static int __gets_callback(ACL_ASTREAM *stream, void *context,
			const char *data, int dlen);

static void default_write_fn(void *arg acl_unused, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	acl_msg_info2(fmt, ap);
	va_end(ap);
}

static void default_fflush_fn(void *arg)
{
	arg = arg;
}

static void (*__write_fn)(void *arg, const char *fmt, ...) = default_write_fn;
static void *__write_arg = NULL;
static void (*__fflush_fn)(void *arg) = default_fflush_fn;
static void *__fflush_arg = NULL;

void aio_server_log_fn(void (*write_fn)(void *, const char *fmt, ...),
			void *write_arg,
			void (*fflush_fn)(void *),
			void *fflush_arg)
{
	if (write_fn) {
		__write_fn = write_fn;
		__write_arg = write_arg;
	}

	if (fflush_fn) {
		__fflush_fn = fflush_fn;
		__fflush_arg = fflush_arg;
	}
}

/*---------------------------------------------------------------------------*/
static int __io_close(ACL_ASTREAM *astream, void *context acl_unused)
{
	const char *myname = "__io_close";
	ACL_VSTREAM *stream = acl_aio_vstream(astream);

	__write_fn(__write_arg, "%s, %s(%d): client(%d) exception!",
		__FILE__, myname, __LINE__, ACL_VSTREAM_SOCK(stream));

	return (-1);
}

static int __io_timeout(ACL_ASTREAM *astream, void *context acl_unused)
{
	const char *myname = "__io_timeout";
	ACL_VSTREAM *stream = acl_aio_vstream(astream);

	__write_fn(__write_arg, "%s, %s(%d): client(%d) timeout!",
		__FILE__, myname, __LINE__, ACL_VSTREAM_SOCK(stream));

	return (-1);
}
static int __write_callback(ACL_ASTREAM *client, void *context acl_unused,
			const char *data acl_unused, int dlen acl_unused)
{
#ifdef	USE_GETS
	acl_aio_gets(client);
#else
	acl_aio_read(client);
#endif
	return (0);
}

static int __gets_callback(ACL_ASTREAM *stream, void *context acl_unused,
			const char *data, int dlen)
{
	if (__echo_src)
		(void) acl_aio_writen(stream, data, dlen);
	else
		acl_aio_writen(stream, __data, __dlen);

	return (0);
}

static int __accept_callback(ACL_ASTREAM *client, void *context acl_unused)
{
	acl_aio_ctl(client, ACL_AIO_CTL_READ_HOOK_ADD, __gets_callback, NULL,
		ACL_AIO_CTL_WRITE_HOOK_ADD, __write_callback, NULL,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, __io_close, NULL,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, __io_timeout, NULL,
		ACL_AIO_CTL_END);
#ifdef	USE_GETS
	acl_aio_gets(client);
#else
	acl_aio_read(client);
#endif

	return (0);
}

static void __listen_callback(ACL_ASTREAM *sstream, void *context)
{
	ACL_VSTREAM *cstream;
	ACL_ASTREAM *client;
	ACL_AIO *aio = (ACL_AIO *) context;
	int   i;

	for (i = 0; i < 10; i++) {
		cstream = acl_vstream_accept(acl_aio_vstream(sstream), NULL, 0);
		if (cstream == NULL)
			break;
		cstream->rw_timeout = 0;
		acl_non_blocking(ACL_VSTREAM_SOCK(cstream), ACL_NON_BLOCKING);
		client = acl_aio_open(aio, cstream);
		acl_aio_ctl(client,
			ACL_AIO_CTL_READ_HOOK_ADD, __gets_callback, NULL,
			ACL_AIO_CTL_WRITE_HOOK_ADD, __write_callback, NULL,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, __io_close, NULL,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, __io_timeout, NULL,
			ACL_AIO_CTL_END);
#ifdef	USE_GETS
		acl_aio_gets(client);
#else
		acl_aio_read(client);
#endif
	}
}

static ACL_ASTREAM *__listener = NULL;

void aiho_server_start(ACL_AIO *aio, const char *addr, int accept_auto, int echo_src)
{
	const char *myname = "echo_start";
	ACL_VSTREAM *sstream;
	ACL_ASTREAM *astream;

	memset(__data, 'X', sizeof(__data));
	__data[sizeof(__data) - 1] = 0;
	__data[sizeof(__data) - 2] = '\n';
	__data[sizeof(__data) - 3] = '\r';
	__dlen = (int) strlen(__data);
	__echo_src = echo_src;
	sstream = acl_vstream_listen(addr, 128);
	assert(sstream);

	acl_aio_set_keep_read(aio, 1);

	astream = acl_aio_open(aio, sstream);
	__listener = astream;

	printf(">>>listen: %s ...\r\n", addr);

	if (!accept_auto) {
		acl_aio_ctl(astream, ACL_AIO_CTL_LISTEN_FN, __listen_callback,
			ACL_AIO_CTL_CTX, aio,
			ACL_AIO_CTL_END);
		acl_aio_listen(astream);
	} else {
		acl_aio_ctl(astream, ACL_AIO_CTL_ACCEPT_FN, __accept_callback,
			ACL_AIO_CTL_CTX, aio,
			ACL_AIO_CTL_END);
		acl_aio_accept(astream);
	}
}

void aio_server_end(void)
{
	if (__listener) {
		acl_aio_iocp_close(__listener);
		acl_aio_loop(__listener->aio);
		__listener = NULL;
	}
}