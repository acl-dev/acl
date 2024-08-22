#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool stop = false;

static void echo_client(acl::socket_stream* conn, acl::fiber_tbox<bool> *box)
{
	printf("fiber-%d running, sock fd=%d\r\n",
		acl::fiber::self(), conn->sock_handle());

	char buf[8192];
	while (true) {
		int ret = conn->read(buf, sizeof(buf), false);
		if (ret == -1) {
			break;
		}
		buf[ret] = 0;

		if (strncasecmp(buf, "stop", 4) == 0) {
			stop = true;
			break;
		}
		if (conn->write(buf, ret) == -1) {
			break;
		}
		if (strncasecmp(buf, "bye", 3) == 0) {
			break;
		}
	}

	box->push(NULL);
}

static void fiber_client(acl::socket_stream* conn)
{
	acl::fiber_tbox<bool>* box = new acl::fiber_tbox<bool>;

	char buf[8192];
	int ret = conn->read(buf, sizeof(buf), false);
	if (ret == -1) {
		printf("read from fd=%d error\r\n", conn->sock_handle());
		delete conn;
		return;
	}
	if (conn->write(buf, ret) == -1) {
		printf("write to fd=%d error\r\n", conn->sock_handle());
		delete conn;
		return;
	}

	go[&] {
		echo_client(conn, box);
	};

	box->pop();
	delete box;

	printf("%s: delete fd=%d\r\n", __FUNCTION__, conn->sock_handle());
	delete conn;

	if (stop) {
		printf("Stop fiber schedule now\r\n");
		acl::fiber::schedule_stop();
	}
}

static void fiber_server(acl::server_socket& ss)
{
	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
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

	while ((ch = getopt(argc, argv, "hs:")) > 0) {
		switch (ch) {
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
	if (ss.open(addr) == false) {
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
