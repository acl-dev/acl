#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

static void client_echo(acl::socket_stream* conn) {
	acl::string buf;
	while (true) {
		int fd = conn->sock_handle();
		struct timeval begin, end;
		gettimeofday(&begin, NULL);
		int ret = acl_readable(fd);
		gettimeofday(&end, NULL);
		double cost = acl::stamp_sub(end, begin);

		if (ret == 0) {
			printf("not readable, cost=%.2f\r\n", cost);
		} else if (ret == 1) {
			printf("readable, cost=%.2f\r\n", cost);
		} else {
			printf("readable error\r\n");
		}

		if (!conn->gets(buf, false)) {
			printf("client read error %s\r\n", acl::last_serror());
			break;
		}
		if (conn->write(buf) == -1) {
			printf("client write error %s\r\n", acl::last_serror());
			break;
		}
	}
	delete conn;
}

static void server_listen(acl::server_socket& ss) {
	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		go[=] {
			client_echo(conn);
		};
	}

	acl::fiber::schedule_stop();
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s server_addr\r\n", procname);
}

int main(int argc, char *argv[]) {
	int  ch;
	acl::string addr = "127.0.0.1:9000";

	acl::acl_cpp_init();
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
	if (!ss.open(addr)) {
		printf("open %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}

	go[&] {
		server_listen(ss);
	};

	acl::fiber::schedule();
	return 0;
}
