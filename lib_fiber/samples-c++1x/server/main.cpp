#include "stdafx.h"
#include <cstdio>
#include <getopt.h>

//////////////////////////////////////////////////////////////////////////////

static void client_echo(const acl::shared_stream& conn, bool readable) {
	acl::string buf;
	conn->set_rw_timeout(1, true);

	while (true) {
		if (readable) {
			struct timeval begin, end;
			gettimeofday(&begin, nullptr);
			int ret = conn->readable();
			gettimeofday(&end, nullptr);
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
			if (errno == EAGAIN) {
				continue;
			}

			break;
		}
		if (conn->write(buf) == -1) {
			printf("client write error %s\r\n", acl::last_serror());
			break;
		}

		time_t begin = time(NULL);
		acl::fiber::delay(4000);
		time_t end = time(NULL);
		printf(">>>>>>>>>wakeup now, tc=%ld seconds<<<<<<<<\n", end - begin);
	}
}

static void server_listen(acl::server_socket& ss, bool readable, bool shared) {
	while (true) {
		acl::shared_stream conn = ss.shared_accept();
		if (conn == nullptr) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		if (shared) {
			go_share(8192)[conn, readable] {
				client_echo(conn, readable);
			};
		} else {
			go[conn, readable] {
				client_echo(conn, readable);
			};
		}
	}

	acl::fiber::schedule_stop();
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|select|poll]\r\n"
		" -S [if using shared stack, default: false]\r\n"
		" -s server_addr\r\n"
		" -r [if call readable]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch;
	bool readable = false, shared = false;
	acl::string addr = "0.0.0.0|9000";
	acl::string event_type("kernel");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:e:rS")) > 0) {
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
		case 'S':
			shared = true;
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

	if (shared) {
		go_share(8192)[&] {
			server_listen(ss, readable, shared);
		};
	} else {
		go[&] {
			server_listen(ss, readable, shared);
		};
	}

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
