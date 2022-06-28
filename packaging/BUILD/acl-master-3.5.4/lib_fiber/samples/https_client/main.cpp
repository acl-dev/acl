#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fiber/lib_fiber.h"
#include "stamp.h"

#define STACK_SIZE	128000

static long long int __total_count = 0;
static int __total_clients         = 0;
static int __total_error_clients   = 0;
static acl::sslbase_conf* __ssl_conf;

static int __conn_timeout = 0;
static int __rw_timeout   = 0;
static int __max_loop     = 10000;
static int __max_fibers   = 100;
static int __left_fibers  = 100;
static struct timeval __begin;

static void http_client(ACL_FIBER *fiber, const char* addr)
{
	acl::string body;
	acl::http_request req(addr, 0, 0);
	acl::http_header& hdr = req.request_header();

	req.set_ssl(__ssl_conf);

	for (int i = 0; i < __max_loop; i++) {
		hdr.set_url("/")
			.set_content_type("text/plain")
			.set_keep_alive(true);

		if (!req.request(NULL, 0)) {
			printf("send request error\r\n");
			break;
		}

		if (!req.get_body(body)) {
			printf("get_body error\r\n");
			break;
		}

		if (i < 1) {
			printf(">>>fiber-%d: body: %s\r\n",
				acl_fiber_id(fiber), body.c_str());
		}

		__total_count++;
		body.clear();
		req.reset();
	}
}

static void fiber_connect(ACL_FIBER *fiber, void *ctx)
{
	const char *addr = (const char *) ctx;

	http_client(fiber, addr);

	--__left_fibers;
	printf("max_fibers: %d, left: %d\r\n", __max_fibers, __left_fibers);

	if (__left_fibers == 0) {
		double spent;
		struct timeval end;

		gettimeofday(&end, NULL);
		spent = stamp_sub(&end, &__begin);

		printf("fibers: %d, clients: %d, error: %d, count: %lld, "
			"spent: %.2f, speed: %.2f\r\n", __max_fibers,
			__total_clients, __total_error_clients,
			__total_count, spent,
			(__total_count * 1000) / (spent > 0 ? spent : 1));

		//acl_fiber_schedule_stop();
	}
}

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx)
{
	char *addr = (char *) ctx;

	for (int i = 0; i < __max_fibers; i++) {
		acl_fiber_create(fiber_connect, addr, STACK_SIZE);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -l libpath\r\n"
		" -s addr\r\n"
		" -t connt_timeout\r\n"
		" -r rw_timeout\r\n"
		" -c max_fibers\r\n"
		" -n max_loop\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  addr[256];
#ifdef __APPLE__
	acl::string libpath("../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib");
#else
	acl::string libpath("../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so");
#endif
       
	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "0.0.0.0:9001");

	while ((ch = getopt(argc, argv, "hc:n:s:t:r:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__max_fibers = atoi(optarg);
			__left_fibers = __max_fibers;
			break;
		case 't':
			__conn_timeout = atoi(optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'l':
			libpath = optarg;
			break;
		default:
			break;
		}
	}

	gettimeofday(&__begin, NULL);

	if (libpath.find("mbedtls") != NULL) {
		const std::vector<acl::string>& libs = libpath.split2("; \t");
		if (libs.size() != 3) {
			printf("invalid libpath=%s\r\n", libpath.c_str());
			return 1;
		}
		acl::mbedtls_conf::set_libpath(libs[0], libs[1], libs[2]);
		if (!acl::mbedtls_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = new acl::mbedtls_conf(false);
	} else if (libpath.find("polarssl") != NULL) {
		acl::polarssl_conf::set_libpath(libpath);
		if (!acl::polarssl_conf::load()) {
			printf("load %s error\n", libpath.c_str());
			return 1;
		}
		__ssl_conf = new acl::polarssl_conf;
	} else {
		printf("invalid ssl lib=%s\r\n", libpath.c_str());
		return 1;
	}

	acl_fiber_create(fiber_main, addr, STACK_SIZE);
	printf("call fiber_schedule\r\n");

	acl_fiber_schedule();
	delete __ssl_conf;

	return 0;
}
