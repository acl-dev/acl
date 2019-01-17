#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

acl::atomic_long __count;
acl::atomic_long __hit;
static int __step = 10000;
static int __loop = 1;
static struct timeval __begin;

class fiber_client : public acl::fiber
{
public:
	fiber_client(int& counter, acl::tcp_keeper& keeper,
			const char* addr, int max)
	: counter_(counter)
	, keeper_(keeper)
	, addr_(addr)
	, max_(max) {}

	~fiber_client(void) {}

private:
	acl::socket_stream* peek(void)
	{
#if 1
		bool reused;
		//acl_doze(5);
		acl::socket_stream* conn = keeper_.peek(addr_, &reused);
		if (reused) {
			++__hit;
		}
		return conn;
#else
		acl::socket_stream* conn = new acl::socket_stream;
		if (conn->open(addr_, 10, 10) == false) {
			printf("connect %s error %s\r\n",
				addr_.c_str(), acl::last_serror());
			delete conn;
			return NULL;
		}
		return conn;
#endif
	}

	// @override
	void run(void)
	{
		printf("fiber-%d running\r\n", acl::fiber::self());

		for (int i = 0; i < max_; i++) {
			struct timeval begin;
			gettimeofday(&begin, NULL);

			acl::socket_stream* conn = peek();

			struct timeval end;
			gettimeofday(&end, NULL);
			double spent = acl::stamp_sub(end, begin);
			if (spent >= 1000) {
				printf("%s(%d): spent: %.2f ms\r\n",
					__FILE__, __LINE__, spent);
			}

			if (conn == NULL) {
				printf("peek connection error=%s\r\n",
					acl::last_serror());
				continue;
			}

			conn->set_tcp_solinger(true, 0);
			doit(*conn);
			delete conn;
		}

		if (--counter_ == 0) {
			struct timeval end;
			gettimeofday(&end, NULL);
			double spent = acl::stamp_sub(end, __begin);
			double speed = 1000 * __count / (spent >= 1 ? spent : 1);

			double hit_ratio;
			if (__count == 0) {
				hit_ratio = 0.0;
			} else {
				hit_ratio = ((double) (__hit.value()) * 100)
					/ (double) __count.value();
			}
			printf("hit=%lld, count=%lld\r\n", __hit.value(), __count.value());
			printf("all fiber_client over! total count=%lld, "
				"speed=%.2f, hit=%lld, hit_ratio=%.2f%%\r\n",
				__count.value(), speed, __hit.value(), hit_ratio);
		}
		printf("counter=%d\r\n", counter_);
	}

	void doit(acl::socket_stream& conn)
	{
		const char s[] = "hello world!\r\n";
		acl::string buf;
		for (int i = 0; i < __loop; i++) {
#if 1
			if (conn.write(s, sizeof(s) - 1) == -1) {
				printf("write error %s\r\n", acl::last_serror());
				break;
			}

			if (conn.gets(buf) == false) {
				printf("gets error %s\r\n", acl::last_serror());
				break;
			}
#endif

			++__count;
			if (__count % __step == 0) {
				char tmp[256];
				snprintf(tmp, sizeof(tmp), "addr=%s, fiber-%d,"
					" gets line=[%s], n=%lld",
					conn.get_peer(true), acl::fiber::self(),
					buf.c_str(), __count.value());
				acl::meter_time(__FUNCTION__, __LINE__, tmp);
			}
		}
	}

private:
	int& counter_;
	acl::tcp_keeper& keeper_;
	acl::string addr_;
	int max_;
};

class fiber_sleep : public acl::fiber
{
public:
	fiber_sleep(void) {}
	~fiber_sleep(void) {}

private:
	// @override
	void run(void)
	{
		while (true) {
			sleep(1);
		}
	}
};

class thread_client : public acl::thread
{
public:
	thread_client(acl::tcp_keeper& keeper, const char* addr,
		int nfibers, int max)
	: keeper_(keeper)
	, addr_(addr)
	, nfibers_(nfibers)
	, max_(max) {}

	~thread_client(void) {}

private:
	// @over
	void* run(void)
	{
		int counter = 0;
		std::vector<acl::fiber*> fibers;

		for (int i = 0; i < nfibers_; i++) {
			counter++;
			acl::fiber* fb = new fiber_client(counter, keeper_,
					addr_, max_);
			fibers.push_back(fb);
			fb->start();
		}

#if 0
		acl::fiber* fb = new fiber_sleep;
		fb->start();
#endif

		acl::fiber::schedule();

		for (std::vector<acl::fiber*>::iterator it = fibers.begin();
			it != fibers.end(); ++it) {

			delete *it;
		}

		return NULL;
	}

private:
	acl::tcp_keeper& keeper_;
	acl::string addr_;
	int nfibers_;
	int max_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s server_addrs\r\n"
		" -c fibers_count[default: 10]\r\n"
		" -n max_loop[default: 10]\r\n"
		" -r rtt_min[default: 0]\r\n"
		" -C conn_max[default: 200]\r\n"
		" -t conn_ttl[default: 10]\r\n"
		" -i step[default: 10000]\r\n"
		" -l loop for one connection[default: 1]\r\n"
		, procname);
}

static void append_addrs(acl::string& buf, std::vector<acl::string>& addrs)
{
	const std::vector<acl::string>& tokens = buf.split2(";, \r\r\n");
	for (std::vector<acl::string>::const_iterator cit = tokens.begin();
		cit != tokens.end(); ++cit) {

		addrs.push_back(*cit);
	}
}

int main(int argc, char *argv[])
{
	int  ch, n = 10, max = 10, rtt_min = 0, conn_max = 200, conn_ttl = 10;
	acl::string addrs_buf("127.0.0.1:8001");
	std::vector<acl::string> addrs;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl::fiber::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:c:n:i:l:r:C:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addrs_buf = optarg;
			append_addrs(addrs_buf, addrs);
			addrs_buf.clear();
			break;
		case 'c':
			n = atoi(optarg);
			break;
		case 'n':
			max = atoi(optarg);
			break;
		case 'r':
			rtt_min = atoi(optarg);
			break;
		case 'C':
			conn_max = atoi(optarg);
			break;
		case 't':
			conn_ttl = atoi(optarg);
			break;
		case 'i':
			__step = atoi(optarg);
			break;
		case 'l':
			__loop = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (!addrs_buf.empty()) {
		append_addrs(addrs_buf, addrs);
	}

	acl::log::debug_init("all:2");

	acl::tcp_keeper keeper;
	keeper.set_rtt_min(rtt_min);
	keeper.set_conn_timeout(10)
		.set_rw_timeout(10)
		.set_conn_min(10)
		.set_conn_max(conn_max)
		.set_conn_ttl(conn_ttl)
		.set_pool_ttl(10);
	keeper.start();

	gettimeofday(&__begin, NULL);

	std::vector<acl::thread*> threads;

	for (std::vector<acl::string>::const_iterator cit = addrs.begin();
		cit != addrs.end(); ++cit) {

		acl::thread* thr = new thread_client(keeper, *cit, n, max);
		thr->set_detachable(false);
		threads.push_back(thr);
		thr->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	keeper.stop();
	return 0;
}
