#include "lib_acl.h"
#include "dns_test.h"

static void usage(const char *procname)
{
	printf("usage: %s -h dns_ip -p dns_port -d domain_name\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	char  dns_ip[64], domain[128];
	int   dns_port = 53, ret;
	ACL_VSTRING *sbuf;

	dns_ip[0] = 0;
	domain[0] = 0;

	while ((ch = getopt(argc, argv, "h:p:d:")) > 0) {
		switch (ch) {
		case 'h':
			ACL_SAFE_STRNCPY(dns_ip, optarg, sizeof(dns_ip));
			break;
		case 'p':
			dns_port = atoi(optarg);
			break;
		case 'd':
			ACL_SAFE_STRNCPY(domain, optarg, sizeof(domain));
			break;
		default:
			usage(argv[0]);
			return (0);
		}
	}

	if (dns_ip[0] == 0 || domain[0] == 0) {
		usage(argv[0]);
		return (0);
	}

	sbuf = acl_vstring_alloc(128);
	ret = dns_lookup(domain, dns_ip, dns_port, sbuf);
	if (ret < 0) {
		printf("dns lookup(%s) error(%s)\r\n", domain, acl_vstring_str(sbuf));
		acl_vstring_free(sbuf);
		return (0);
	}

	printf("domain: %s\r\n%s", domain, acl_vstring_str(sbuf));
	acl_vstring_free(sbuf);
	return (0);
}

