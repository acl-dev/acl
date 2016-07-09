#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fiber/lib_fiber.h"
#include "stamp.h"

static long long int __total_count = 0;
static int __total_clients         = 0;
static int __total_error_clients   = 0;
static acl::polarssl_conf __ssl_conf;

static int __conn_timeout = 0;
static int __rw_timeout   = 0;
static int __max_loop     = 10000;
static int __max_fibers   = 100;
static int __left_fibers  = 100;
static int __read_data    = 0;
static struct timeval __begin;

static void http_client(ACL_FIBER *fiber, const char* addr)
{
	acl::string body;
	acl::http_request req(addr, 0, 0);
	acl::http_header& hdr = req.request_header();

	hdr.set_url("/").set_content_type("text/plain").set_keep_alive(true);
	req.set_ssl(&__ssl_conf);

	for (int i = 0; i < __max_loop; i++)
	{
		if (req.request(NULL, 0) == false)
		{
			printf("send request error\r\n");
			break;
		}

		if (req.get_body(body) == false)
		{
			printf("get_body error\r\n");
			break;
		}

		if (i < 1)
			printf(">>>fiber-%d: body: %s\r\n",
				acl_fiber_id(fiber), body.c_str());

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

		acl_fiber_stop();
	}
}

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx)
{
	char *addr = (char *) ctx;
	int i;

	for (i = 0; i < __max_fibers; i++)
		acl_fiber_create(fiber_connect, addr, 32768);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s addr\r\n"
		" -t connt_timeout\r\n"
		" -r rw_timeout\r\n"
		" -c max_fibers\r\n"
		" -w [if wait for echo data from server, dafault: no]\r\n"
		" -n max_loop\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  addr[256];
       
	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "0.0.0.0:9001");

	while ((ch = getopt(argc, argv, "hc:n:s:t:r:w")) > 0) {
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
		case 'w':
			__read_data = 1;
			break;
		default:
			break;
		}
	}

	gettimeofday(&__begin, NULL);

	acl_fiber_create(fiber_main, addr, 32768);

	printf("call fiber_schedule\r\n");

	acl_fiber_schedule();

	return 0;
}
