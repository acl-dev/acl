#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static void fiber_client(acl::socket_stream* conn)
{
	printf("fiber-%d running\r\n", acl::fiber::self());

	char buf[8192];
	while (true)
	{
		int ret = conn->read(buf, sizeof(buf), false);
		if (ret == -1)
			break;
		if (conn->write(buf, ret) == -1)
			break;
	}

	delete conn;
}

static void fiber_server(acl::server_socket& ss)
{
	while (true)
	{
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		printf("accept ok, fd: %d\r\n", conn->sock_handle());

		go[=] {
			fiber_client(conn);
		};
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	go[&] {
		fiber_server(ss);
	};

	acl::fiber::schedule();	// start fiber schedule

	return 0;
}
