#include "stdafx.h"
#include <cstdio>
#include <thread>
#include <memory>
#include <vector>
#include <getopt.h>

//////////////////////////////////////////////////////////////////////////////

static void client_echo(const acl::shared_stream& conn, int dlen, int max, bool zerocp) {
	char* buff = (char*) malloc(dlen);
	memset(buff, 'X', dlen);
	int flags = MSG_NOSIGNAL;
	if (zerocp) {
		flags |= MSG_ZEROCOPY;
		conn->set_zerocopy(true);
	}

	for (int i = 0; i < max; i++) {
		if (conn->send(buff, dlen, flags) == -1) {
			printf("send error %s\r\n", acl::last_serror());
			break;
		}
		if (zerocp && !conn->wait_iocp(5000)) {
			printf("wait result error %s\r\n", acl::last_serror());
			break;
		}
	}
	free(buff);
}

static void server_listen(std::shared_ptr<acl::server_socket> ss, int dlen,
	  int max, bool zerocp) {

	while (true) {
		acl::shared_stream conn = ss->shared_accept();
		if (conn == nullptr) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		go[conn, dlen, max, zerocp] {
			client_echo(conn, dlen, max, zerocp);
		};
	}

	acl::fiber::schedule_stop();
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -s server_addr[default: 127.0.0.1:8082]\r\n"
		" -e event_type[kernel|select|poll|io_uring, default: kernel]\r\n"
		" -Z [if using zerocopy, default: false]\r\n"
		" -k threads count[default: 1]\r\n"
		" -d data_length[default: 8192]\r\n"
		" -n lop_max[default: 10]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, nthreads = 1, dlen = 8192, max = 10;
	bool zerocp = false;
	acl::string addr = "127.0.0.1|8082";
	acl::string event_type("kernel");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hk:s:e:Zd:n:")) > 0) {
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
		case 'Z':
			zerocp = true;
			break;
		case 'd':
			dlen = atoi(optarg);
			break;
		case 'n':
			max = atoi(optarg);
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
			go[=] {
				server_listen(ss, dlen, max, zerocp);
			};

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
