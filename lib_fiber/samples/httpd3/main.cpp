#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"
#include "http_servlet.h"

#define	STACK_SIZE	64000

static void client_callback(int type, ACL_EVENT *event,
	ACL_VSTREAM *cstream, void *ctx);

static int __rw_timeout = 0;
static int __stop       = 0;
static int __real_http  = 0;
static int __use_kernel = 0;

static int http_demo(ACL_VSTREAM *cstream, const char* res, size_t len)
{
	char  buf[8192];
	int   ret;

	cstream->rw_timeout = __rw_timeout;

	while (1) {
		ret = acl_vstream_gets_nonl(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s\r\n", acl_last_serror());
			return -1;
		}

		buf[ret] = 0;
		if (strcasecmp(buf, "stop") == 0) {
			__stop = 1;
			printf("----stop now----\r\n");
			break;
		}

		if (ret == 0)
			break;
	}

	if (acl_vstream_writen(cstream, res, len) == ACL_VSTREAM_EOF) {
		printf("write error\r\n");
		return -1;
	}

	return 0;
}

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event = (ACL_EVENT *) cstream->context;
	const char* res = "HTTP/1.1 200 OK\r\n"
		"Date: Tue, 31 May 2016 14:20:28 GMT\r\n"
		"Server: acl\r\n"
		"Content-Type: text/html\r\n"
		"Content-Length: 12\r\n"
		"Connection: Keep-Alive\r\n"
		"\r\n"
		"hello world!";
	size_t len = strlen(res);

	if (http_demo(cstream, res, len) < 0 || __stop) {
		acl_vstream_close(cstream);
		return;
	}

	//printf("%s: %d: call acl_event_enable_read again, fd: %d\r\n",
	//	__FUNCTION__, __LINE__, ACL_VSTREAM_SOCK(cstream));
	acl_event_enable_read(event, cstream, 120, client_callback, NULL);
}

static void http_server(ACL_FIBER *, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event = (ACL_EVENT *) cstream->context;
	acl::socket_stream conn;

	conn.open(cstream);
	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(&conn, &session);
	servlet.setLocalCharset("gb2312");

	if (servlet.doRun() == false) {
		return;
	}

	conn.unbind();
	acl_event_enable_read(event, cstream, 120, client_callback, NULL);
}

static void client_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *cstream, void *ctx acl_unused)
{
	cstream->context = event;
	acl_event_disable_readwrite(event, cstream);

	if (__real_http)
		acl_fiber_create(http_server, cstream, 32000);
	else
		acl_fiber_create(echo_client, cstream, 16000);
}

static void listen_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *sstream, void *ctx acl_unused)
{
	char ip[64];
	ACL_VSTREAM *cstream = acl_vstream_accept(sstream, ip, sizeof(ip));

	if (cstream == NULL) {
		printf("accept error %s\r\n", acl_last_serror());
		return;
	}

	printf(">>>accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(cstream));

	acl_event_enable_read(event, cstream, 120, client_callback, NULL);
}

static void fiber_event(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event;

	if (__use_kernel)
		event = acl_event_new(ACL_EVENT_KERNEL, 0, 1, 0);
	else
		event = acl_event_new(ACL_EVENT_POLL, 0, 1, 0);

	printf(">>>enable read fd: %d\r\n", ACL_VSTREAM_SOCK(sstream));
	acl_event_enable_listen(event, sstream, 0, listen_callback, NULL);

	while (!__stop)
		acl_event_loop(event);

	acl_vstream_close(sstream);
	acl_event_free(event);

	acl_fiber_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n"
		" -r rw_timeout\r\n"
		" -R [use real http]\r\n"
		" -k [use kernel event]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int  ch;

	acl::log::stdout_open(true);
	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9001");

	while ((ch = getopt(argc, argv, "hs:r:Rk")) > 0) {
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
		case 'k':
			__use_kernel = 1;
			break;
		default:
			break;
		}
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
	acl_fiber_schedule();

	return 0;
}
