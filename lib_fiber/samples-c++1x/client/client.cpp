#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

static size_t do_echo(acl::socket_stream& conn, int count) {
	acl::string data("hello world!\r\n");
	acl::string buf;

	size_t i;
	for (i = 0; i < (size_t) count; i++) {
		if (conn.write(data) == -1) {
			printf("client write error %s\r\n", acl::last_serror());
			break;
		}
#if 1
		struct timeval begin, end;
		gettimeofday(&begin, NULL);
		int ret = acl_readable(conn.sock_handle());
		gettimeofday(&end, NULL);
		double cost = acl::stamp_sub(end, begin);

		if (ret == 0) {
			printf("not readable, cost=%.2f\r\n", cost);
		} else if (ret == 1) {
			printf("readable, cost=%.2f\r\n", cost);
		} else {
			printf("readable error\r\n");
		}
#endif
		if (!conn.gets(buf, false)) {
			printf("client read error %s\r\n", acl::last_serror());
			break;
		}
	}

	printf(">>>count=%d<<<\r\n", i);
	return i;
}

static size_t connect_server(const char* addr, int count) {
	acl::socket_stream conn;
	if (!conn.open(addr, 10, 10)) {
		printf("connect %s error %s\r\n", addr, acl::last_serror());
		return 0;
	}

	printf(">>>connect %s ok\r\n", addr);
	return do_echo(conn, count);
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|select|poll, default: kernel]\r\n"
		" -s server_addr[default: 127.0.0.1:9000]\r\n"
		" -c fiber_count[default: 1]\r\n"
		" -n count[default: 100]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, nfiber = 1, count = 100;
	acl::string addr = "127.0.0.1:9000", event_type("kernel");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "he:s:c:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'e':
			event_type = optarg;
			break;
		case 's':
			addr = optarg;
			break;
		case 'c':
			nfiber = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::fiber_event_t type;

	if (event_type == "select") {
		type = acl::FIBER_EVENT_T_SELECT;
	} else if (event_type == "poll") {
		type = acl::FIBER_EVENT_T_POLL;
	} else {
		type = acl::FIBER_EVENT_T_KERNEL;
	}

	size_t total = 0;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < nfiber; i++) {
		go[&] {
			total += connect_server(addr, count);
		};
	}

	acl::fiber::schedule_with(type);

	struct timeval end;
	gettimeofday(&end, NULL);
	double cost = acl::stamp_sub(end, begin);
	printf("Total count=%zd, cost=%.2f ms, speed=%.2f\r\n",
		total, cost, (total * 1000) / (cost > 0 ? cost : 0.1));
	return 0;
}
