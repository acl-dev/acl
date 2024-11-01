#include "stdafx.h"

static bool use_sockopt = false;
static std::atomic<long long> total_count(0);

static void fiber_main(const char *addr, int count, int timeo) {
	acl::socket_stream conn;
	if (!conn.open(addr, timeo, timeo)) {
		printf("Connect %s error %s\r\n", addr, acl::last_serror());
		return;
	}

	conn.set_rw_timeout(timeo, use_sockopt);

	printf("Connect %s ok, fd=%d\r\n", addr, conn.sock_handle());

	const char s[] = "hello world!\r\n";
	char buf[1024];
	int i;

	for (i = 0; i < count; i++) {
		if (conn.write(s, sizeof(s) - 1) == -1) {
			printf("write error %s\r\n", acl::last_serror());
			break;
		}
		if (conn.read(buf, sizeof(buf) -1, false) == -1) {
			printf("read error %s\r\n", acl::last_serror());
			break;
		}
	}

	total_count += i;
}

static void thread_main(const char *addr, int nfibers, int count , int timeo) {
	for (int j = 0; j < nfibers; j++) {
		go[=] {
			fiber_main(addr, count, timeo);
		};
	}


	acl::fiber::schedule_with(acl::FIBER_EVENT_T_KERNEL);
}

static void usage(const char *procname) {
	printf("usage: %s -h [help]\r\n"
		" -s server_addr[default: 127.0.0.1:9001]\r\n"
		" -k cpus[default: 1]\r\n"
		" -c fibers count[default: 100]\r\n"
		" -n count[default: 10000]\r\n"
		" -o io timeout[default: -1]\r\n"
		" -O [if use setsockopt to set IO timeout, default: false]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int ch, cpus = 1, count = 10000, nfibers = 100, timeo = -1;
	std::string addr("127.0.0.1:9001");

	while ((ch = getopt(argc, argv, "hs:k:c:n:o:O")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'k':
			cpus = atoi(optarg);
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'o':
			timeo = atoi(optarg);
			break;
		case 'O':
			use_sockopt = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	struct timeval begin;
	gettimeofday(&begin, nullptr);

	std::vector<std::thread*> threads;
	for (int i = 0; i < cpus; i++) {
		std::thread *thr = new std::thread([&]{
			thread_main(addr.c_str(), nfibers, count, timeo);
		});
		threads.push_back(thr);
	}

	for (auto thr : threads) {
		thr->join();
		delete thr;
	}

	struct timeval end;
	gettimeofday(&end, nullptr);
	double tc = acl::stamp_sub(end, begin);

	double speed = (total_count * 1000) / (tc > 0 ? tc : 0.0001);
	printf("Total count=%lld, tc=%.2f ms, speed=%.2f\r\n",
		total_count.load(), tc, speed);

	return 0;
}
