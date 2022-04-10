#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static bool tried = false, is_sleeping = false;
static acl::socket_stream* first = NULL, *second = NULL;

static void fiber_client(acl::socket_stream* conn)
{
	conn->set_rw_timeout(20);
	int fd = conn->sock_handle();

	printf("fiber-%d running, fiber=%p\r\n", acl::fiber::self(), acl_fiber_running());

	char buf[8192];
	while (true) {
		printf(">>>%s: fiber=%p, begin read, fd=%d, %d\n",
			__FUNCTION__, acl_fiber_running(),
			conn->sock_handle(), fd);

		int ret = conn->read(buf, sizeof(buf), false);

		printf(">>>%s: fiber=%p,  read=%d, fd=%d, %d\n",
			__FUNCTION__, acl_fiber_running(), ret,
			conn->sock_handle(), fd);

		if (ret == -1) {
			printf("fiber-%d, read error %s from fd %d, %d, fiber=%p\r\n",
				acl::fiber::self(), acl::last_serror(),
				conn->sock_handle(), fd, acl_fiber_running());
			break;
		}

		if (conn->write(buf, ret) == -1) {
			printf("fiber-%d, write error %s to fd %d\r\n",
				acl::fiber::self(), acl::last_serror(),
				conn->sock_handle());
			break;
		}

		if (conn == first) {
			if (second) {
				printf("---close another fd=%d, me fiber=%p\n",
					second->sock_handle(), acl_fiber_running());
				int sock = second->sock_handle();
				if (is_sleeping) {
					second->unbind_sock();
				}
				close(sock);
				//second->close();
				printf("---close another fd=%d ok, fiber=%p\n",
					sock, acl_fiber_running());
				second = NULL;
			} else {
				printf("---another fd closed\r\n");
			}
		}

		//continue;

		if (conn == second) {
			time_t begin = time(NULL);
			printf("second fiber-%d, %p, fd=%d sleep\n",
				acl::fiber::self(), acl_fiber_running(),
				conn->sock_handle());
			is_sleeping = true;
			sleep(15);
			is_sleeping = false;
			printf("second fiber-%d, %p, fd=%d wakeup, diff=%ld second\n",
				acl::fiber::self(), acl_fiber_running(),
				conn->sock_handle(), time(NULL) - begin);
		}
	}

	printf("\r\n>>fiber=%p, delete conn, fd=%d\n",
		acl_fiber_running(), conn->sock_handle());
	delete conn;
	printf(">>fiber=%p, delete conn ok: %s\n\n",
		acl_fiber_running(), acl::last_serror());
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

		if (first == NULL) {
			first = conn;
		} else if (second == NULL) {
			if (!tried) {
				second = conn;
				tried = true;
			}
		}

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
	acl::fiber::stdout_open(true);

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
	if (!ss.open(addr)) {
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
