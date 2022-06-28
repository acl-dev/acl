#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "echo_server.h"

#define	USE_GETS

/* forward functions */

static char *__data;
static int   __dlen;
static int   __echo_src;
static int   __line_length;

static int __read_callback(ACL_ASTREAM *stream, void *context,
			const char *data, int dlen);

static void default_write_fn(void *arg acl_unused, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	acl_msg_info2(fmt, ap);
	va_end(ap);
}

static void default_fflush_fn(void *arg acl_unused)
{
}

static void (*__write_fn)(void *arg, const char *fmt, ...) = default_write_fn;
static void *__write_arg = NULL;
static void (*__fflush_fn)(void *arg) = default_fflush_fn;
static void *__fflush_arg = NULL;

void echo_server_log_fn(void (*write_fn)(void *, const char *fmt, ...),
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

static int __read_callback(ACL_ASTREAM *stream, void *context acl_unused,
			const char *data, int dlen)
{
#ifdef	USE_GETS
	static int  __n = 0;

	if (__n <= 10) {
		printf(">>gets: [%s]\r\n", data);
		__n++;
	}
#endif

	if (__echo_src)
		(void) acl_aio_writen(stream, data, dlen);
	else
		acl_aio_writen(stream, __data, __dlen);

	return (0);
}

static int __accept_callback(ACL_ASTREAM *client, void *context acl_unused)
{
	char buf[64];

	if (acl_getpeername(ACL_VSTREAM_SOCK(acl_aio_vstream(client)), buf, sizeof(buf)) == 0)
		printf("connect from %s\r\n", buf);
	else
		printf("can't get client addr %s\r\n", acl_last_serror());

	acl_aio_ctl(client,
		ACL_AIO_CTL_READ_HOOK_ADD, __read_callback, NULL,
		ACL_AIO_CTL_WRITE_HOOK_ADD, __write_callback, NULL,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, __io_close, NULL,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, __io_timeout, NULL,
		ACL_AIO_CTL_TIMEOUT, 60,
		ACL_AIO_CTL_LINE_LENGTH, __line_length,
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
	char buf[64];
	int   i;

	for (i = 0; i < 10; i++) {
		cstream = acl_vstream_accept(acl_aio_vstream(sstream), NULL, 0);
		if (cstream == NULL)
			break;

		if (acl_getpeername(ACL_VSTREAM_SOCK(cstream), buf, sizeof(buf)) == 0)
			printf("connect from %s\r\n", buf);
		else
			printf("can't get client addr %s\r\n", acl_last_serror());
		cstream->rw_timeout = 0;
		acl_non_blocking(ACL_VSTREAM_SOCK(cstream), ACL_NON_BLOCKING);
		client = acl_aio_open(aio, cstream);
		acl_aio_ctl(client,
			ACL_AIO_CTL_READ_HOOK_ADD, __read_callback, NULL,
			ACL_AIO_CTL_WRITE_HOOK_ADD, __write_callback, NULL,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, __io_close, NULL,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, __io_timeout, NULL,
			ACL_AIO_CTL_TIMEOUT, 60,
			ACL_AIO_CTL_LINE_LENGTH, __line_length,
			ACL_AIO_CTL_END);
#ifdef	USE_GETS
		acl_aio_gets(client);
#else
		acl_aio_read(client);
#endif
	}
}

ACL_AIO *echo_server_start(ACL_VSTREAM *sstream, int accept_auto, int event_mode)
{
	char  myname[] = "echo_start";
	ACL_AIO *h_aio;
	ACL_ASTREAM *astream;

	switch (event_mode) {
	case ACL_EVENT_SELECT:
		printf("------- use select --------\r\n");
		break;
	case ACL_EVENT_KERNEL:
		printf("------- use kernel --------\r\n");
		break;
	case ACL_EVENT_POLL:
		printf("------- use  poll  --------\r\n");
		break;
	default:
		acl_msg_fatal("%s: unknown event(%d)", myname, event_mode);
		break;
	}

	h_aio = acl_aio_create(event_mode);
	acl_aio_set_keep_read(h_aio, 1);

	if (h_aio == NULL)
		acl_msg_fatal("%s(%d): acl_aio_create error(%s)",
			myname, __LINE__, acl_last_serror());

	astream = acl_aio_open(h_aio, sstream);
	
	if (!accept_auto) {
		acl_aio_ctl(astream, ACL_AIO_CTL_LISTEN_FN, __listen_callback,
			ACL_AIO_CTL_CTX, h_aio,
			ACL_AIO_CTL_END);
		acl_aio_listen(astream);
	} else {
		acl_aio_ctl(astream, ACL_AIO_CTL_ACCEPT_FN, __accept_callback,
			ACL_AIO_CTL_CTX, h_aio,
			ACL_AIO_CTL_END);
		acl_aio_accept(astream);
	}

	return (h_aio);
}

void echo_server_init(char *data, int dlen, int echo_src, int line_length)
{
	__data = data;
	__dlen = dlen;
	__echo_src = echo_src;
	__line_length = line_length;
}
