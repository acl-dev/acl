#include "lib_acl.h"
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

static void resolve(const char* domain, int socktype, int family)
{
	ACL_ITER iter;
	int herr;
	ACL_DNS_DB* db = acl_gethostbyname2(domain, socktype, family, &herr);

	printf("%s\r\n", domain);

	if (db == NULL) {
		printf("acl_gethostbyname %s error %s\r\n",
			domain, acl_netdb_strerror(herr));
		return;
	}

	acl_foreach(iter, db) {
		const ACL_HOSTNAME* hn = (const ACL_HOSTNAME*) iter.data;
		if (hn->saddr.sa.sa_family == AF_INET) {
			printf(" ip: %s, ttl: %d, type=A\r\n", hn->ip, hn->ttl);
		} else if (hn->saddr.sa.sa_family == AF_INET6) {
			printf(" ip: %s, ttl: %d, type=AAAA\r\n", hn->ip, hn->ttl);
		} else {
			printf(" ip: %s, ttl: %d, type=unknown\r\n", hn->ip, hn->ttl);
		}
	}

	acl_netdb_free(db);
}

static void usage(const char* procname)
{
	printf("usage: %s -h help -n domain\r\n"
		" -t [use TCP if been set, or use UDP]\r\n"
		" -t [lookup for IPV6 if been set or for IPV4\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch, socktype = SOCK_DGRAM, family = PF_UNSPEC;
	char buf[256];

	snprintf(buf, sizeof(buf), "www.baidu.com");

	while ((ch = getopt(argc, argv, "hn:t46")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			snprintf(buf, sizeof(buf), "%s", optarg);
			break;
		case 't':
			socktype = SOCK_STREAM;
			break;
		case '4':
			family = PF_INET;
			break;
		case '6':
			family = PF_INET6;
			break;
		default:
			break;
		}
	}

	resolve(buf, socktype, family);
	return (0);
}
