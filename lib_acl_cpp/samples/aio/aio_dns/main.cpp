#include "lib_acl.h"
#ifndef WIN32
#include <getopt.h>
#endif
#include <iostream>
#include "acl_cpp/acl_cpp_init.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stdlib/dns_service.hpp"

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -t[use thread]\n", procname);
}

static int __ncount = 0;

class dns_result : public acl::dns_result_callback
{
public:
	dns_result(const char* domain)
	: dns_result_callback(domain) {}

	~dns_result(void) {}

	// @override
	void destroy(void)
	{
		delete this;
		__ncount--;
	}

	// @override
	void on_result(const char* domain,  const acl::dns_res& res)
	{
		std::cout << "result: domain: " << domain;
		if (res.ips_.size() == 0) {
			std::cout << ": null" << std::endl;
			return;
		}

		std::cout << std::endl;

		std::list<acl::string>::const_iterator cit = res.ips_.begin();
		for (; cit != res.ips_.end(); cit++) {
			std::cout << "\t" << (*cit).c_str();
		}
		std::cout << std::endl;
	}
};

int main(int argc, char* argv[])
{
	int   ch, nthreads = 2;

	while ((ch = getopt(argc, argv, "ht:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			nthreads = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	acl::aio_handle handle;

	acl::dns_service* server = new acl::dns_service(nthreads);

	// 使消息服务器监听 127.0.0.1 的地址
	if (!server->open(&handle)) {
		delete server;
		std::cout << "open server error!" << std::endl;
		getchar();
		return 1;
	}

	const char* domain = "www.baidu.com";
	dns_result* result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	domain = "www.sina.com";
	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	domain = "www.51iker.com";
	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	domain = "www.hexun.com";
	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	domain = "www.com.dummy";
	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	result = new dns_result(domain);
	server->lookup(result);
	__ncount++;

	while (true) {
		if (handle.check() == false) {
			std::cout << "stop now!" << std::endl;
			break;
		}
		if (__ncount == 0) {
			break;
		}
	}

	delete server;
	handle.check();

	std::cout << "server stopped! enter any key to exit ..." << std::endl;
	getchar();
	return 0;
}
