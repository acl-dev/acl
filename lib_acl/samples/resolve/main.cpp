#include "lib_acl.h"
#include <netdb.h>
#include <stdio.h>
//#include <netdb.h>
//extern int h_errno;

#if	!defined(MACOSX) && !defined(ACL_SUNOS5) && !defined(MINGW)
static ACL_DNS_DB *resolve_name(const char *name)
{
	struct hostent *h_addrp = NULL;
	struct hostent  h_buf; 
	int    err = 0, n;
	char   buf[4096];
	char **pptr;
	ACL_DNS_DB *res;
	ACL_HOSTNAME  *h_host;

	printf("%s: gethostbyname name: %s\n", __FUNCTION__, name);

	n = gethostbyname_r(name, &h_buf, buf, sizeof(buf), &h_addrp, &err);
	if (n) {
		printf("%s: gethostbyname %s error\n", __FUNCTION__, name);
		return NULL;
	}

	res = acl_netdb_new(name);
	for (pptr = h_addrp->h_addr_list; *pptr != NULL; pptr++) {
		h_host = (ACL_HOSTNAME*) acl_mycalloc(1, sizeof(ACL_HOSTNAME));

		memset(&h_host->saddr, 0, sizeof(h_host->saddr));
		if ((int) sizeof(h_host->saddr.in.sin_addr) > h_addrp->h_length)
			n = h_addrp->h_length;
		else
			n = (int) sizeof(h_host->saddr.in.sin_addr);

		memcpy(&h_host->saddr.in.sin_addr, *pptr, n);
		acl_inet_ntoa(h_host->saddr.in.sin_addr,
			h_host->ip, sizeof(h_host->ip));

		(void) acl_array_append(res->h_db, h_host);
		res->size++;
	}

	return res;
}
#endif

static ACL_DNS_DB *resolve_addr(const char *name)
{
	ACL_DNS_DB *res;
	struct hostent *h_addrp = NULL;
	struct in_addr addr;
	ACL_HOSTNAME  *h_host;
	char **pptr;
	int    n; 

	memset(&addr, 0, sizeof(addr));
	addr.s_addr = inet_addr(name);

	printf("%s: gethostbyaddr addr: %s\n", __FUNCTION__, name);

	h_addrp = gethostbyaddr((char*) &addr, sizeof(addr), AF_INET);
	if (!h_addrp) {
		printf("%s: gethostbyaddr error=%s\n",
			__FUNCTION__, hstrerror(h_errno));
		return NULL;
	}

	printf("h_name: %s\n", h_addrp->h_name);

	res = acl_netdb_new(name);

	for (pptr = h_addrp->h_addr_list; *pptr != NULL; pptr++) {
		h_host = (ACL_HOSTNAME*) acl_mycalloc(1, sizeof(ACL_HOSTNAME));

		memset(&h_host->saddr, 0, sizeof(h_host->saddr));
		if ((int) sizeof(h_host->saddr.in.sin_addr) > h_addrp->h_length)
			n = h_addrp->h_length;
		else
			n = (int) sizeof(h_host->saddr.in.sin_addr);

		memcpy(&h_host->saddr.in.sin_addr, *pptr, n);
		acl_inet_ntoa(h_host->saddr.in.sin_addr,
			h_host->ip, sizeof(h_host->ip));

		(void) acl_array_append(res->h_db, h_host);

		res->size++;
	}

	return res;
}

static void show(ACL_DNS_DB *res)
{
	ACL_ITER iter;

	if (!res)
		return;

	acl_foreach(iter, res) {
		const ACL_HOST_INFO *info;

		info = (const ACL_HOST_INFO*) iter.data;
		printf("\tip=%s; port=%d\n", info->ip, info->hport);
	}
}

static void test(const char *name, int use_acl)
{
	ACL_DNS_DB *res = NULL;

	if (use_acl) {
		printf("acl_gethostbyname name: %s\n", name);
		res = acl_gethostbyname(name, NULL);
		if (res == NULL) {
			printf("acl_gethostbyname %s error\n", name);
			return;
		}
	}
#if	!defined(MACOSX) && !defined(ACL_SUNOS5) && !defined(MINGW)
	else {
		res = resolve_name(name);
	}
#endif

	if (res) {
		show(res);
		acl_netdb_free(res);
	}

	printf("-------------------------------------------------------\r\n");

	res = resolve_addr(name);
	if (res) {
		show(res);
		acl_netdb_free(res);
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -n name -r[use_acl_resolve]\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, use_acl = 0;
	char  name[256];

	name[0] = 0;

	while ((ch = getopt(argc, argv, "hn:r")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'n':
			snprintf(name, sizeof(name), "%s", optarg);
			break;
		case 'r':
			use_acl = 1;
			break;
		default:
			break;
		}
	}

	acl_msg_stdout_enable(1);

	const char *addr = "192.168.1.1";
	printf("check ipv4 %s %d\r\n", addr, acl_valid_ipv4_hostaddr(addr, 1));
	printf("check ipv6 %s %d\r\n", addr, acl_valid_ipv6_hostaddr(addr, 1));
	addr = "192.168.1.1:80";
	printf("check ipv4 %s %d\r\n", addr, acl_valid_ipv4_hostaddr(addr, 1));
	addr = "192.168.1.1#80";
	printf("check ipv4 %s %d\r\n", addr, acl_valid_ipv4_hostaddr(addr, 1));

	addr = "fe80::ca1f:66ff:fef6:c496";
	printf("check ipv6 %s %d\r\n", addr, acl_valid_ipv6_hostaddr(addr, 1));

	addr = "fe80::ca1f:66ff:fef6:c496#80";
	printf("check ipv6 %s %d\r\n", addr, acl_valid_ipv6_hostaddr(addr, 1));

	if (name[0])
		test(name, use_acl);
	else
		usage(argv[0]);
	return (0);
}
