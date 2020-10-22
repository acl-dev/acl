#include "stdafx.h"

static int __fibers_count = 2;
static int __fibers_max   = 2;
static int __oper_count   = 100;
static bool __show        = false;
static acl::string __host("www.baidu.com");
static int __count = 0;

class fiber_http : public acl::fiber
{
public:
	fiber_http(const char* addr, const char* url)
	: addr_(addr), url_(url) {}

	void start_alone(void)
	{
		run();
	}

private:
	acl::string addr_;
	acl::string url_;

	~fiber_http(void) {}

	// @override
	void run(void)
	{

		for (int i = 0; i < __oper_count; i++) {
			acl::http_request req(addr_);
			acl::http_header& hdr = req.request_header();

			hdr.set_url(url_)
				.set_method(acl::HTTP_METHOD_GET)
				.set_host(__host)
				.set_keep_alive(true)
				.accept_gzip(false);

			if (req.request(NULL, 0) == false) {
				printf("request %s error %s, url=%s\r\n",
					addr_.c_str(), acl::last_serror(),
					url_.c_str());
				break;
			}

			if (get_response(req) == false) {
				printf("read reply error %s\r\n",
					acl::last_serror());
				break;
			}

			++__count;
			hdr.reset();
		}

		if (--__fibers_count == 0) {
			printf(">>>total count=%d<<<\r\n", __count);
//			acl::fiber::schedule_stop();
		}

		delete this;
	}

	bool get_response(acl::http_request& req)
	{
		char  buf[8192];
		int n = 0;

		while (true) {
			int ret = req.read_body(buf, sizeof(buf) - 1);
			if (ret == 0) {
				printf(">>>read response over, n=%d\n", n);
				break;
			} else if (ret < 0) {
				return false;
			}
			buf[ret] = 0;
			n += ret;

			if (__count > 10) {
				continue;
			}

			if (__show) {
				printf(">>>%s<<<\r\n", buf);
			}
		}

		printf(">>>response body=%d\r\n", n);
		return n > 0 ? true : false;
	}
};

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s httpd_addr\r\n"
		" -H host\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -S [show debug info]\r\n"
		" -a [run one alone client]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;
	bool run_alone = false;
	acl::string addr("127.0.0.1:9001");
	acl::string url("/");

	while ((ch = getopt(argc, argv, "hs:n:c:SaH:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			__oper_count = atoi(optarg);
			break;
		case 'c':
			__fibers_count = atoi(optarg);
			__fibers_max = __fibers_count;
			break;
		case 'S':
			__show = true;
			break;
		case 'a':
			run_alone = true;
			break;
		case 'H':
			__host = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	if (run_alone) {
		fiber_http* http= new fiber_http(addr, url);
		http->start_alone();
		printf("run alone over\r\n");
		return 0;
	}

	//gettimeofday(&__begin, NULL);

	for (i = 0; i < __fibers_count; i++) {
		acl::fiber* fb = new fiber_http(addr, url);
		fb->start();
	}

	acl::fiber::schedule();
	return 0;
}
