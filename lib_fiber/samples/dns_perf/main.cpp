#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <thread>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "fiber/go_fiber.hpp"

static char   __dns_ip[256];
static int    __dns_port = 53;
static int    __nloop = 10;
static acl::atomic_long __count = 0;
static int    __unit = 1000;

static void nslookup(const char* name)
{

	ACL_DNS_DB *res;
	ACL_ITER iter;
	int i;

	ACL_RES *ns = acl_res_new(__dns_ip, __dns_port);
	printf(">>>> begin call acl_res_lookup resolve %s\r\n", name);

	for (i = 0; i < __nloop; i++) {
		res = acl_res_lookup(ns, name);

		if (res == NULL) {
			printf("can't resolve: %s\r\n", name);
			continue;
		}

		if (++__count % __unit == 0) {
			char buf[128];
			snprintf(buf, sizeof(buf), "count=%llu", __count.value());
			acl_meter_time(__FUNCTION__, __LINE__, buf);
		}

		if (i >= 10) {
			acl_netdb_free(res);
			continue;
		}

		printf("%s\r\n", name);
		acl_foreach(iter, res) {
			ACL_HOSTNAME *h = (ACL_HOSTNAME*) iter.data;
			printf("\tip: %s, ttl: %u\r\n", h->ip, h->ttl);
		}

		acl_netdb_free(res);
	}

	acl_res_free(ns);
}

static void thread_main(char* domain, int cocurrent)
{
	for (int i = 0; i < cocurrent; i++) {
		go_stack(256000) [=] {
			nslookup(domain);
		};
	}

	acl::fiber::schedule();

}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -N domain\r\n"
		" -t threads_count\r\n"
		" -c cocurrent\r\n"
		" -n loop\r\n"
		" -u show_unit\r\n"
		" -s dns_ip\r\n"
		" -p dns_port\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, cocurrent = 1, nthreads = 1;
	char  domain[1024];

	domain[0] = 0;
	__dns_ip[0] = 0;

	while ((ch = getopt(argc, argv, "hn:t:c:u:N:s:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			__nloop = atoi(optarg);
			break;
		case 'u':
			__unit = atoi(optarg);
			if (__unit < 100) {
				__unit = 100;
			}
		case 'N':
			snprintf(domain, sizeof(domain), "%s", optarg);
			break;
		case 's':
			snprintf(__dns_ip, sizeof(__dns_ip), "%s", optarg);
			break;
		case 'p':
			__dns_port = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (domain[0] == 0) {
		usage(argv[0]);
		return 0;
	}

	acl_msg_stdout_enable(1);
	acl::fiber::stdout_open(true);

	struct timeval begin;
	gettimeofday(&begin, NULL);

	std::vector<std::thread*> threads;
	for (i = 0; i < nthreads; i++) {
		std::thread* thread = new std::thread(thread_main,
			domain, cocurrent);
		threads.push_back(thread);
	}

	for (std::vector<std::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->join();
		delete *it;
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double cost = acl::stamp_sub(end, begin);
	double speed = (__count.value() * 1000) / (cost > 0 ? cost : 0.1);
	printf(">>>total count=%lld, cost=%.2f ms, speed=%.2f\r\n",
		__count.value(), cost, speed);

	return 0;
}
