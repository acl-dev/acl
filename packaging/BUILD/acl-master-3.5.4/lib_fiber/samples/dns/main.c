#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fiber/libfiber.h"

static char  __dns_ip[256];
static int   __dns_port = 53;
static int   __count = 0;

static void test_gethostbyname(const char *name)
{
	struct hostent *ent = gethostbyname(name);
	int i;

	printf("\r\n");
	printf("------------------------------------------\r\n");

	if (ent == NULL) {
		printf("gethostbyname error, name=%s\r\n", name);
		return;
	}

	printf("gethostbyname: h_name=%s, h_length=%d, h_addrtype=%d\r\n",
		ent->h_name, ent->h_length, ent->h_addrtype);
	for (i = 0; ent->h_addr_list[i]; i++) {
		char *addr = ent->h_addr_list[i];
		char  ip[64];
		const char *ptr;

		ptr = inet_ntop(ent->h_addrtype, addr, ip, sizeof(ip));
		printf(">>>addr: %s\r\n", ptr);
	}

	printf("------------------------------------------\r\n\r\n");
}

static void nslookup(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *name = (const char *)ctx;

	ACL_DNS_DB *res;
	ACL_ITER iter;

	if (__dns_ip[0] != 0) {
		ACL_RES *ns = acl_res_new(__dns_ip, __dns_port);
		if (ns == NULL) {
			printf(">>>acl_res_new NULL: %s\r\n", name);
			return;
		}
		printf(">>>> begin call acl_res_lookup resolve %s\r\n", name);
		res = acl_res_lookup(ns, name);
		acl_res_free(ns);
	} else {
		printf(">>>> begin call acl_gethostbyname resolve %s\r\n", name);
		res = acl_gethostbyname(name, NULL);
	}

	if (res == NULL) {
		printf("can't resolve: %s\r\n", name);
	} else {
		printf("%s\r\n", name);
		acl_foreach(iter, res) {
			ACL_HOSTNAME *h = (ACL_HOSTNAME*) iter.data;
			printf("\tip: %s, ttl: %u\r\n", h->ip, h->ttl);
		}

		acl_netdb_free(res);
	}

	--__count;
	printf("__count: %d\r\n", __count);

	test_gethostbyname(name);

	if (__count == 0) {
		printf("All are over!\r\n");
		//acl_fiber_schedule_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n addrs -a dns_ip -p dns_port\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
	char  buf[1024];
	ACL_ARGV *tokens;
	ACL_ITER  iter;

	buf[0] = 0;
	__dns_ip[0] = 0;

	while ((ch = getopt(argc, argv, "hn:a:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			snprintf(buf, sizeof(buf), "%s", optarg);
			break;
		case 'a':
			snprintf(__dns_ip, sizeof(__dns_ip), "%s", optarg);
			break;
		case 'p':
			__dns_port = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (buf[0] == 0) {
		usage(argv[0]);
		return 0;
	}

	acl_fiber_msg_stdout_enable(1);

	tokens = acl_argv_split(buf, ";, \t");
	__count = tokens->argc;

	acl_foreach(iter, tokens) {
		char* addr = (char* ) iter.data;

		acl_fiber_create(nslookup, addr, 320000);
		test_gethostbyname(addr);
	}

	acl_fiber_schedule();

	acl_argv_free(tokens);

	return 0;
}
