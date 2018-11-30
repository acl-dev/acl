#include "stdafx.h"

static int __fibers_count = 2;
static int __fibers_max   = 2;
static int __oper_count   = 100;

static int __count = 0;

class fiber_http : public acl::fiber
{
public:
	fiber_http(const char* addr, const char* url)
	: addr_(addr), url_(url) {}

private:
	acl::string addr_;
	acl::string url_;

	~fiber_http(void) {}

	// @override
	void run(void)
	{
		acl::http_request req(addr_);
		acl::http_header& hdr = req.request_header();
		hdr.set_url(url_).set_method(acl::HTTP_METHOD_GET)
			.set_keep_alive(true);

		for (int i = 0; i < __oper_count; i++) {
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
		}

		if (--__fibers_count == 0) {
			printf(">>>total count=%d<<<\r\n", __count);
			acl::fiber::schedule_stop();
		}

		delete this;
	}

	bool get_response(acl::http_request& req)
	{
		char  buf[8192];

		while (true) {
			int ret = req.read_body(buf, sizeof(buf) - 1);
			if (ret == 0) {
				return true;
			} else if (ret < 0) {
				return false;
			}
			buf[ret] = 0;
			if (__count <= 10)
				printf(">>>%s<<<\r\n", buf);
		}
	}
};

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s httpd_addr\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;
	acl::string addr("127.0.0.1:9001");
	acl::string url("/");

	while ((ch = getopt(argc, argv, "hs:n:c:")) > 0) {
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
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	//gettimeofday(&__begin, NULL);

	for (i = 0; i < __fibers_count; i++) {
		acl::fiber* fb = new fiber_http(addr, url);
		fb->start();
	}

	acl::fiber::schedule();
	return 0;
}
