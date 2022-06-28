#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

static void client_echo(acl::socket_stream* conn, bool readable) {
	acl::string buf;
	while (true) {
		if (readable) {
			struct timeval begin, end;
			gettimeofday(&begin, NULL);
			int ret = acl_readable(conn->sock_handle());
			gettimeofday(&end, NULL);
			double cost = acl::stamp_sub(end, begin);

			if (ret == 0) {
				printf("not readable, cost=%.2f\r\n", cost);
			} else if (ret == 1) {
				printf("readable, cost=%.2f\r\n", cost);
			} else {
				printf("readable error\r\n");
			}
		}

		if (!conn->gets(buf, false)) {
			printf("client read error %s\r\n", acl::last_serror());
			break;
		}
		if (conn->write(buf) == -1) {
			printf("client write error %s\r\n", acl::last_serror());
			break;
		}
		//acl::fiber::delay(1000);
	}
	delete conn;
}

static void server_listen(acl::server_socket& ss, bool readable) {
	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		go[=] {
			client_echo(conn, readable);
		};
	}

	acl::fiber::schedule_stop();
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|select|poll]\r\n"
		" -s server_addr\r\n"
		" -r [if call readable]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch;
	bool readable = false;
	acl::string addr = "0.0.0.0:9000";
	acl::string event_type("kernel");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:e:r")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'e':
			event_type = optarg;
			break;
		case 'r':
			readable = true;
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
		server_listen(ss, readable);
	};

	acl::fiber_event_t type;
	if (event_type == "select") {
		type = acl::FIBER_EVENT_T_SELECT;
	} else if (event_type == "poll") {
		type = acl::FIBER_EVENT_T_POLL;
	} else {
		type = acl::FIBER_EVENT_T_KERNEL;
	}

	acl::fiber::schedule_with(type);

	return 0;
}
