#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

static int start(const char* addr, int count) {
	int i;
	acl::string buf;

	for (i = 0; i < count; i++) {
		acl::http_request req(addr);
		acl::http_header& head = req.request_header();
		head.set_url("/")
			.accept_gzip(false)
			.set_host(addr)
			.set_keep_alive(false);

		acl::string tmp;
		head.build_request(tmp);
		printf(">>request: %s\r\n", tmp.c_str());
		if (!req.request(NULL, 0)) {
			printf("request error\r\n");
			break;
		}

		printf(">>>begin read body\r\n");

		if (!req.get_body(buf)) {
			printf("get response error\r\n");
			break;
		}
		if (i == 0) {
			printf("response: %s\r\n", buf.c_str());
		}
		buf.clear();
	}
	return i;
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|select|poll, default: kernel]\r\n"
		" -s server_addr[default: 127.0.0.1:8088]\r\n"
		" -c fiber_count[default: 1]\r\n"
		" -n count[default: 100]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, nfiber = 1, count = 100;
	acl::string addr = "127.0.0.1:8088", event_type("kernel");

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

	long long total = 0;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < nfiber; i++) {
		go[&] {
			total += start(addr, count);
		};
	}

	acl::fiber::schedule_with(type);

	struct timeval end;
	gettimeofday(&end, NULL);
	double cost = acl::stamp_sub(end, begin);
	printf("Total count=%lld, cost=%.2f ms, speed=%.2f\r\n",
		total, cost, (total * 1000) / (cost > 0 ? cost : 0.1));
	return 0;
}
