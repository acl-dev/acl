#include "stdafx.h"
#include <cstdio>
#include <thread>
#include <memory>
#include <vector>
#include <getopt.h>

//////////////////////////////////////////////////////////////////////////////

static void client_echo(const acl::shared_stream& conn, bool sockopt) {
	conn->set_rw_timeout(10, sockopt);
	char buf[4096];
	int ret;

	while (true) {
		ret = conn->read(buf, sizeof(buf), false);
		if (ret <= 0) {
			printf("client read error %s\r\n", acl::last_serror());
			if (errno == EAGAIN) {
				continue;
			}

			break;
		}
		if (conn->write(buf, ret) == -1) {
			printf("client write error %s\r\n", acl::last_serror());
			break;
		}
	}
}

static void server_listen(std::shared_ptr<acl::server_socket> ss, bool sockopt, bool shared) {
	while (true) {
		acl::shared_stream conn = ss->shared_accept();
		if (conn == nullptr) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		if (shared) {
			go_share(8192)[conn, sockopt] {
				client_echo(conn, sockopt);
			};
		} else {
			go[conn, sockopt] {
				client_echo(conn, sockopt);
			};
		}
	}

	acl::fiber::schedule_stop();
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|select|poll|io_uring]\r\n"
		" -S [if using shared stack, default: false]\r\n"
		" -s server_addr\r\n"
		" -O [if using setsockopt to set IO timeout, default: false]\r\n"
		" -k threads count[default: 1]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, nthreads = 1;
	bool sockopt = false, shared = false;
	acl::string addr = "127.0.0.1|9000";
	acl::string event_type("kernel");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hk:s:e:OS")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'k':
			nthreads = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		case 'e':
			event_type = optarg;
			break;
		case 'O':
			sockopt = true;
			break;
		case 'S':
			shared = true;
			break;
		default:
			break;
		}
	}

#ifdef	__APPLE__
	std::shared_ptr<acl::server_socket> ss(new acl::server_socket);
	if (!ss->open(addr)) {
		printf("open %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
#endif

	std::vector<std::thread*> threads;

	for (int i = 0; i < nthreads; i++) {
#ifdef	__linux__
		std::shared_ptr<acl::server_socket> ss(
			new acl::server_socket(acl::OPEN_FLAG_REUSEPORT, 128));
		if (!ss->open(addr)) {
			printf("open %s error %s\r\n", addr.c_str(), acl::last_serror());
			return 1;
		}
		printf("Open %s ok\r\n", addr.c_str());
#endif

		std::thread* thr = new std::thread([=] {
			if (shared) {
				go_share(8192)[=] {
					server_listen(ss, sockopt, shared);
				};
			} else {
				go[=] {
					server_listen(ss, sockopt, shared);
				};
			}

			acl::fiber_event_t type;
			if (event_type == "select") {
				type = acl::FIBER_EVENT_T_SELECT;
			} else if (event_type == "poll") {
				type = acl::FIBER_EVENT_T_POLL;
			} else if (event_type == "io_uring") {
				type = acl::FIBER_EVENT_T_IO_URING;
			} else {
				type = acl::FIBER_EVENT_T_KERNEL;
			}

			acl::fiber::schedule_with(type);
		});
		threads.push_back(thr);
	}

	for (auto thr : threads) {
		thr->join();
		delete thr;
	}
	return 0;
}
