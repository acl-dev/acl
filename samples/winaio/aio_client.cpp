#include "StdAfx.h"
#include <assert.h>
#include "lib_acl.h"
#include "aio_client.h"

typedef struct ECHO_COUNTER{
	int   nloop;
	int   max_loop;
	ACL_ASTREAM *stream;
} ECHO_COUNTER;

static int   __nread    = 0;
static int   __nwrite   = 0;
static int   __nconnect_ok = 0;
static int   __nconn    = 0;

static char  __data[1024];
static int   __dlen;
static int   __nconn_per_sec = 100;

static int __write_callback(ACL_ASTREAM *stream, void *context,
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
	const char *data, int dlen acl_unused)
{
	ECHO_COUNTER *counter = (ECHO_COUNTER *) context;

	__nread++;
	counter->nloop++;

	if (counter->nloop < 100)
		printf(">>%s\r\n", data);
	if (counter->nloop % 10000 == 0)
		printf(">>> nloop=%d, max_loop=%d\r\n",
		counter->nloop, counter->max_loop);
	if (counter->nloop >= counter->max_loop) {
		printf(">>> ok, nloop=%d, max_loop=%d\r\n",
			counter->nloop, counter->max_loop);
		return (-1);
	} else
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
	printf(">>>>connect ok\n");
	acl_aio_fprintf(client, "%s\n", __data);
	return (0);
}

void aio_client_start(ACL_AIO *aio, const char *addr, int max_connect)
{
	const char *myname = "connect_pool";
	ECHO_COUNTER *counter;
	int   timeout = 10;
	int   i;

	for (i = 0; i < sizeof(__data) - 1; i++) {
		__data[i] ='x';
	}
	__data[i] = 0;
	__dlen = (int) strlen(__data);

	for (i = 0; i < max_connect; i++) {
		counter = (ECHO_COUNTER *) acl_mycalloc(1, sizeof(ECHO_COUNTER));
		counter->nloop = 0;
		counter->max_loop = 100;
		counter->stream = acl_aio_connect(aio, addr, timeout);
		if (counter->stream == NULL) {
			printf("%s(%d): acl_aio_connect(%s) error(%s)\r\n",
				myname, __LINE__, addr, acl_last_serror());
			acl_myfree(counter);
			continue;
		}

		acl_aio_ctl(counter->stream,
			ACL_AIO_CTL_WRITE_HOOK_ADD, __write_callback, counter,
			ACL_AIO_CTL_READ_HOOK_ADD, __read_callback, counter,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, __timeout_callback, counter,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, __close_callback, counter,
			ACL_AIO_CTL_CONNECT_HOOK_ADD, __connect_callback, counter,
			ACL_AIO_CTL_TIMEOUT, timeout,
			ACL_AIO_CTL_END);
		__nconn++;

		if (__nconn_per_sec > 0 && i % __nconn_per_sec == 0)
			printf("%s: connect->%d\r\n", myname, i);
	}

	printf("prepare connect %d ok, __timeout=%d\r\n", __nconn, timeout);
}

void aio_client_init(void)
{
	FILE *fp;

	AllocConsole();
	fp = freopen("CONOUT$","w+t",stdout);
}