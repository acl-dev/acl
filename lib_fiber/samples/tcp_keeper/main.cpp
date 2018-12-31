#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

acl::atomic_long __count;
static int __step = 10000;
static int __loop = 1;

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
	// @override
	void run(void)
	{
		printf("fiber-%d running\r\n", acl::fiber::self());

		for (int i = 0; i < max_; i++) {
			acl::socket_stream* conn = keeper_.peek(addr_);
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
			printf("all fiber_client over!\r\n");
		}
	}

	void doit(acl::socket_stream& conn)
	{
		const char s[] = "hello world!\r\n";
		acl::string buf;
		for (int i = 0; i < __loop; i++) {
			if (conn.write(s, sizeof(s) - 1) == -1) {
				printf("write error %s\r\n", acl::last_serror());
				break;
			}

			if (conn.gets(buf) == false) {
				printf("gets error %s\r\n", acl::last_serror());
				break;
			}

			if (++__count % __step == 0) {
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
		" -n fibers_count[default: 10]\r\n"
		" -m max_loop[default: 10]\r\n"
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
	int  ch, n = 10, max = 10;
	acl::string addrs_buf("127.0.0.1:8001");
	std::vector<acl::string> addrs;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:n:m:i:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addrs_buf = optarg;
			append_addrs(addrs_buf, addrs);
			addrs_buf.clear();
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'm':
			max = atoi(optarg);
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

	acl::fiber::stdout_open(true);
	acl::tcp_keeper keeper;
	keeper.set_conn_timeout(10)
		.set_rw_timeout(10)
		.set_conn_max(10)
		.set_conn_ttl(10)
		.set_pool_ttl(10);
	keeper.start();

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
