#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include "lib_acl.h"
#include "fiber/libfiber.h"

#define	STACK_SIZE	320000

static void client_callback(int type, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *ctx);

static int __rw_timeout = 0;
static int __stop       = 0;
static int __use_kernel = 0;

static int do_echo(ACL_VSTREAM *conn)
{
	char buf[8192];
	int  ret;

	conn->rw_timeout = __rw_timeout;
	ret = acl_vstream_gets_nonl(conn, buf, sizeof(buf) - 1);
	if (ret == ACL_VSTREAM_EOF) {
		//printf("gets error %s\r\n", acl_last_serror());
		return -1;
	}
	buf[ret] = 0;
	if (strcasecmp(buf, "stop") == 0) {
		__stop = 1;
		printf("----stop now---\r\n");
		return -1;
	}
	if (acl_vstream_fprintf(conn, "%s\r\n", buf) == ACL_VSTREAM_EOF) {
		printf("write error %s\r\n", acl_last_serror());
		return -1;
	}
	return 0;
}

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *conn = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event  = (ACL_EVENT *) conn->context;

	if (do_echo(conn) < 0 || __stop) {
		acl_vstream_close(conn);
		return;
	}

	acl_event_enable_read(event, conn, 120, client_callback, NULL);
}

static void client_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *ctx acl_unused)
{
	conn->context = event;
	acl_event_disable_readwrite(event, conn);

	//printf(">>>client_callback called, fd=%d\n", ACL_VSTREAM_SOCK(conn));
	acl_fiber_create(echo_client, conn, 160000);
}

//#define LISTEN_REACTOR

#ifdef LISTEN_REACTOR

static void listen_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *sstream, void *ctx acl_unused)
{
	char ip[64];
	ACL_VSTREAM *conn = acl_vstream_accept(sstream, ip, sizeof(ip));

	if (conn == NULL) {
		printf("accept error %s\r\n", acl_last_serror());
		acl_fiber_schedule_stop();
		return;
	}

	printf(">>>accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(conn));
	acl_event_enable_read(event, conn, 120, client_callback, NULL);
}

#else

static ACL_EVENT *__event;

static void fiber_listen(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	while (1) {
		char ip[64];
		ACL_VSTREAM *conn = acl_vstream_accept(sstream, ip, sizeof(ip));

		if (conn == NULL) {
			printf("accept error %s\r\n", acl_last_serror());
			break;
		}

		printf("%s: accept one, fd: %d\r\n",
			__FUNCTION__, ACL_VSTREAM_SOCK(conn));
		acl_event_enable_read(__event, conn, 120, client_callback, NULL);
	}

	acl_fiber_schedule_stop();
}

#endif

static void fiber_event(ACL_FIBER *fiber acl_unused, void *ctx)
{
#ifdef LISTEN_REACTOR
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
#else
	(void) ctx;
#endif
	ACL_EVENT *event;

	if (__use_kernel) {
		event = acl_event_new(ACL_EVENT_KERNEL, 0, 0, 200);
	} else {
		event = acl_event_new(ACL_EVENT_POLL, 0, 0, 200);
	}

#ifndef LISTEN_REACTOR
	__event = event;
#endif

#ifdef LISTEN_REACTOR
	printf(">>>enable listen fd: %d\r\n", ACL_VSTREAM_SOCK(sstream));
	acl_event_enable_listen(event, sstream, 0, listen_callback, NULL);
#endif

	while (!__stop) {
		acl_event_loop(event);
	}

#ifdef LISTEN_REACTOR
	acl_vstream_close(sstream);
#endif
	acl_event_free(event);

	acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n"
		" -r rw_timeout\r\n"
		" -k [use kernel event]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int  ch;

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:8282");

	while ((ch = getopt(argc, argv, "hs:r:k")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'k':
			__use_kernel = 1;
			break;
		default:
			break;
		}
	}

	sstream = acl_vstream_listen(addr, 10240);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

#ifndef LISTEN_REACTOR
	acl_fiber_create(fiber_listen, sstream, STACK_SIZE);
#endif
	acl_fiber_create(fiber_event, sstream, STACK_SIZE);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule();

	return 0;
}
