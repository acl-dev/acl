#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/libfiber.h"
#include "http_servlet.h"

#define	STACK_SIZE	320000

static void client_callback(int type, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *ctx);

static int __rw_timeout   = 0;
static int __stop         = 0;
static int __real_http    = 0;
static int __use_kernel   = 0;
static int __accept_alone = 0;

static int http_demo(ACL_VSTREAM *conn, const char* res, size_t len)
{
	char  buf[8192];
	int   ret;

	conn->rw_timeout = __rw_timeout;

	while (1) {
		ret = acl_vstream_gets_nonl(conn, buf, sizeof(buf) -1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s\r\n", acl_last_serror());
			return -1;
		}
		if (ret > 0) {
			buf[ret] = 0;
			if (strcasecmp(buf, "stop") == 0) {
				__stop = 1;
			}
			break;
		}
	}

	while (1) {
		ret = acl_vstream_gets_nonl(conn, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s\r\n", acl_last_serror());
			return -1;
		}

		if (ret == 0) {
			break;
		}

		buf[ret] = 0;
		if (strcasecmp(buf, "stop") == 0) {
			__stop = 1;
			printf("----stop now----\r\n");
			break;
		}
	}

	if (acl_vstream_writen(conn, res, len) == ACL_VSTREAM_EOF) {
		printf("write error\r\n");
		return -1;
	}

	return 0;
}

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *conn = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event = (ACL_EVENT *) conn->context;
	const char* res = "HTTP/1.1 200 OK\r\n"
		"Date: Tue, 31 May 2016 14:20:28 GMT\r\n"
		"Server: acl\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 12\r\n"
		"Connection: Keep-Alive\r\n"
		"\r\n"
		"hello world!";
	size_t len = strlen(res);

	if (http_demo(conn, res, len) < 0 || __stop) {
		acl_vstream_close(conn);
		return;
	}

	//printf("%s: %d: call acl_event_enable_read again, fd: %d\r\n",
	//	__FUNCTION__, __LINE__, ACL_VSTREAM_SOCK(conn));
	acl_event_enable_read(event, conn, 120, client_callback, NULL);
}

static void http_server(ACL_FIBER *, void *ctx)
{
	ACL_VSTREAM *client = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event = (ACL_EVENT *) client->context;
	acl::socket_stream conn;

	conn.open(client);
	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(&conn, &session);
	servlet.setLocalCharset("gb2312");

	if (servlet.doRun() == false) {
		return;
	}

	conn.unbind();
	acl_event_enable_read(event, client, 120, client_callback, NULL);
}

static void client_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *ctx acl_unused)
{
	conn->context = event;
	acl_event_disable_readwrite(event, conn);

	if (__real_http) {
		acl_fiber_create(http_server, conn, 128000);
	} else {
		acl_fiber_create(echo_client, conn, 64000);
	}
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event = (ACL_EVENT *) sstream->context;

	while (true) {
		char ip[64];
		ACL_VSTREAM *conn = acl_vstream_accept(sstream, ip, sizeof(ip));
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		printf(">>>%s: accept one, fd: %d\r\n",
			__FUNCTION__, ACL_VSTREAM_SOCK(conn));
		acl_event_enable_read(event, conn, 120, client_callback, NULL);
	}

	acl_fiber_schedule_stop();
}

static void listen_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *sstream, void *ctx acl_unused)
{
	char ip[64];
	ACL_VSTREAM *conn = acl_vstream_accept(sstream, ip, sizeof(ip));

	if (conn== NULL) {
		printf("accept error %s\r\n", acl_last_serror());
		return;
	}

	printf(">>>accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(conn));
	acl_event_enable_read(event, conn, 120, client_callback, NULL);
}

static void fiber_event(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event;

	if (__use_kernel) {
		event = acl_event_new(ACL_EVENT_KERNEL, 0, 1, 0);
	} else {
		event = acl_event_new(ACL_EVENT_POLL, 0, 1, 0);
	}

	if (__accept_alone) {
		sstream->context = event;
		acl_fiber_create(fiber_accept, sstream, STACK_SIZE);
	} else {
		acl_event_enable_listen(event, sstream, 0, listen_callback, NULL);
	}

	while (!__stop) {
		acl_event_loop(event);
	}

	acl_vstream_close(sstream);
	acl_event_free(event);

	acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n"
		" -e fiber_event_type[kernel|poll|select]\r\n"
		" -r rw_timeout\r\n"
		" -R [use real http]\r\n"
		" -A [use fiber to accept]\r\n"
		" -k [use kernel event]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int  ch, fiber_event_type = FIBER_EVENT_KERNEL;

	acl::log::stdout_open(true);
	acl_fiber_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9001");

	while ((ch = getopt(argc, argv, "hs:r:RAke:")) > 0) {
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
		case 'R':
			__real_http = 1;
			break;
		case 'A':
			__accept_alone = 1;
			break;
		case 'k':
			__use_kernel = 1;
			break;
		case 'e':
			if (strcasecmp(optarg, "kernel") == 0) {
				fiber_event_type = FIBER_EVENT_KERNEL;
			} else if (strcasecmp(optarg, "poll") == 0) {
				fiber_event_type = FIBER_EVENT_POLL;
			} else if (strcasecmp(optarg, "select") == 0) {
				fiber_event_type = FIBER_EVENT_SELECT;
			}
		default:
			break;
		}
	}

	if (fiber_event_type != FIBER_EVENT_KERNEL) {
		__use_kernel = 0;
	}

	sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_event, sstream, STACK_SIZE);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule_with(fiber_event_type);

	return 0;
}
