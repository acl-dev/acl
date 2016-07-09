#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "http_servlet.h"

#define	 STACK_SIZE	32000
static int __rw_timeout = 0;

static void http_server(ACL_FIBER *, void *ctx)
{
	acl::socket_stream *conn = (acl::socket_stream *) ctx;

	printf("start one http_server\r\n");

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(conn, &session);
	servlet.setLocalCharset("gb2312");

	while (true)
	{
		if (servlet.doRun() == false)
			break;
	}

	printf("close one connection: %d, %s\r\n", conn->sock_handle(),
		acl::last_serror());
	delete conn;
}

static void fiber_accept(ACL_FIBER *, void *ctx)
{
	const char* addr = (const char* ) ctx;
	acl::server_socket server;

	if (server.open(addr) == false)
	{
		printf("open %s error\r\n", addr);
		exit (1);
	}
	else
		printf("open %s ok\r\n", addr);

	while (true)
	{
		acl::socket_stream* client = server.accept();
		if (client == NULL)
		{
			printf("accept failed: %s\r\n", acl::last_serror());
			break;
		}

		client->set_rw_timeout(__rw_timeout);
		printf("accept one: %d\r\n", client->sock_handle());
		acl_fiber_create(http_server, client, STACK_SIZE);
	}

	exit (0);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl::string addr("127.0.0.1:9001");
	int  ch;

	while ((ch = getopt(argc, argv, "hs:r:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl_fiber_create(fiber_accept, addr.c_str(), STACK_SIZE);

	acl_fiber_schedule();
	return 0;
}
