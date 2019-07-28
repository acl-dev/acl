#include "lib_acl.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "echo_client.h"

typedef struct ECHO_COUNTER{
	int   nloop;
	int   max_loop;
	ACL_ASTREAM *stream;
} ECHO_COUNTER;

static int   __nread    = 0;
static int   __nwrite   = 0;
static int   __nconnect_ok = 0;
static int   __nconn    = 0;

static ACL_AIO *__aio = NULL;
static char *__data;

static char *__server_addr = NULL;
static int   __max_connect = 1000;
static int   __max_loop = 1;
static int   __timeout = 60;
static int   __dlen = 256;
static int   __nconn_per_sec = 100;

/* forward functions */

static int __write_callback( ACL_ASTREAM *stream, void *context,
	const char *data, int dlen);

static int __timeout_callback(ACL_ASTREAM *stream acl_unused,
	void *context acl_unused)
{
	printf("timeout, sockfd=%d\r\n", ACL_VSTREAM_SOCK(acl_aio_vstream(stream)));
	return (-1);
}

static int __close_callback(ACL_ASTREAM *stream, void *context)
{
	ECHO_COUNTER *counter = (ECHO_COUNTER *) context;

	printf("closed, sockfd=%d\r\n", ACL_VSTREAM_SOCK(acl_aio_vstream(stream)));
	__nconn--;
	acl_myfree(counter);
	return (-1);
}

static int __read_callback(ACL_ASTREAM *stream, void *context,
	const char *data acl_unused, int dlen acl_unused)
{
	ECHO_COUNTER *counter = (ECHO_COUNTER *) context;

	__nread++;
	counter->nloop++;

	if (counter->nloop % 10000 == 0)
		printf(">>> nloop=%d, max_loop=%d\r\n",
			counter->nloop, counter->max_loop);
	if (counter->nloop >= counter->max_loop) {
		printf(">>> ok, nloop=%d, max_loop=%d\r\n",
			counter->nloop, counter->max_loop);
		return (-1);
	}

	acl_aio_fprintf(stream, "%s\n", __data);
	return (0);
}

static int __write_callback(ACL_ASTREAM *stream, void *context acl_unused,
	const char *data acl_unused, int dlen acl_unused)
{
	__nwrite++;
	acl_aio_gets(stream);
	return (0);
}

static int __connect_callback(ACL_ASTREAM *client, void *context acl_unused)
{
	__nconnect_ok++;
	acl_aio_fprintf(client, "%s\n", __data);
	return (0);
}

void echo_client_init(int name, ...)
{
	va_list ap;
	int   i, event_mode = 0;

	/* init the variable */
	__nread    = 0;
	__nwrite   = 0;
	__nconnect_ok = 0;
	__nconn    = 0;

	va_start(ap, name);

	for (; name != ECHO_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ECHO_CTL_SERV_ADDR:
			__server_addr = acl_mystrdup(va_arg(ap, char *));
			break;
		case ECHO_CTL_MAX_CONNECT:
			__max_connect = va_arg(ap, int);
			break;
		case ECHO_CTL_MAX_LOOP:
			__max_loop = va_arg(ap, int);
			break;
		case ECHO_CTL_TIMEOUT:
			__timeout = va_arg(ap, int);
			break;
		case ECHO_CTL_DATA_LEN:
			__dlen = va_arg(ap, int);
			break;
		case ECHO_CTL_NCONN_PERSEC:
			__nconn_per_sec = va_arg(ap, int);
			break;
		case ECHO_CTL_EVENT_MODE:
			event_mode = va_arg(ap, int);
			break;
		default:
			acl_msg_fatal("%s(%d): unknown arg(%d)",
					__FILE__, __LINE__, name);
		}
	}

	va_end(ap);

	assert(__aio == NULL);

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
		acl_msg_fatal("%s: unknown event(%d)", __FUNCTION__, event_mode);
		break;
	}

	__aio = acl_aio_create(event_mode);
	acl_aio_set_keep_read(__aio, 1);

	assert(__data == NULL);
	__data = (char *) acl_mycalloc(1, __dlen);
	assert(__data);

	for (i = 0; i < __dlen - 2; i++) {
		__data[i] ='0';
	}

	__dlen = (int) strlen(__data);

	if (__server_addr == NULL)
		__server_addr = acl_mystrdup("127.0.0.1:30082");
}

static void connect_pool(void)
{
	const char *myname = "connect_pool";
	ECHO_COUNTER *counter;
	int   i;

	for (i = 0; i < __max_connect; i++) {
		counter = (ECHO_COUNTER *) acl_mycalloc(1, sizeof(ECHO_COUNTER));
		counter->nloop = 0;
		counter->max_loop = __max_loop;
		counter->stream = acl_aio_connect(__aio, __server_addr, __timeout);
		if (counter->stream == NULL) {
			printf("%s(%d): acl_aio_connect(%s) error(%s)\r\n",
				myname, __LINE__, __server_addr,
				acl_last_serror());
			acl_myfree(counter);
			sleep(1);
			continue;
		}

		acl_aio_ctl(counter->stream,
			ACL_AIO_CTL_WRITE_HOOK_ADD, __write_callback, counter,
			ACL_AIO_CTL_READ_HOOK_ADD, __read_callback, counter,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, __timeout_callback, counter,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, __close_callback, counter,
			ACL_AIO_CTL_CONNECT_HOOK_ADD, __connect_callback, counter,
			ACL_AIO_CTL_TIMEOUT, __timeout,
			ACL_AIO_CTL_END);
		__nconn++;

		if (__nconn_per_sec > 0 && i % __nconn_per_sec == 0) {
			printf("%s: connect->%d\r\n", myname, i);
			sleep(1);
		}
	}

	printf("prepare connect %d ok, __timeout=%d\r\n", __nconn, __timeout);
}

static void gc_timer(int event_type acl_unused, ACL_EVENT *event acl_unused,
	void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;

	acl_mem_slice_delay_destroy();
	/* 设定定时器定时清理垃圾回收器 */
	acl_aio_request_timer(aio, gc_timer, aio, 2, 0);
}

void echo_client_start(int use_slice)
{
	time_t begin;

	/* 建立连接池 */
	connect_pool();

	begin = time(NULL);

	/* 设定定时器定时清理垃圾回收器 */
	if (use_slice)
		acl_aio_request_timer(__aio, gc_timer, __aio, 2, 0);

	while (1) {
		acl_aio_loop(__aio);
		if (__nconn <= 0)
			break;
	}

	printf(">>> OK, current thread number=0, exit now, nconn(%d)\r\n", __nconn);
	printf(">>> nconnect(%d), nread(%d), nwrite(%d), total time(%ld)\r\n",
		__nconnect_ok, __nread, __nwrite, time(NULL) - begin);

	if (__server_addr)
		acl_myfree(__server_addr);
	__server_addr = NULL;
	if (__data)
		acl_myfree(__data);
	__data = NULL;
	printf("enter any key to quit now\n");

	acl_aio_free(__aio);
	__aio = NULL;
	getchar();
}

