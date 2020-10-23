#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fiber/libfiber.h"

static char   __dns_ip[256];
static int    __dns_port = 53;
static int    __nloop = 10;
static size_t __count = 0;
static int    __unit = 1000;

static void nslookup(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *name = (const char *)ctx;

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
			snprintf(buf, sizeof(buf), "count=%lu", __count);
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

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -N domain\r\n"
		" -c cocurrent\r\n"
		" -n loop\r\n"
		" -u show_unit\r\n"
		" -s dns_ip\r\n"
		" -p dns_port\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int   ch, i, cocurrent = 1;
	char  domain[1024];

	domain[0] = 0;
	__dns_ip[0] = 0;

	while ((ch = getopt(argc, argv, "hn:c:u:N:s:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
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

	acl_fiber_msg_stdout_enable(1);

	for (i = 0; i < cocurrent; i++) {
		acl_fiber_create(nslookup, domain, 320000);
	}

	acl_fiber_schedule();

	return 0;
}
