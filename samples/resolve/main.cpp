#include "lib_acl.h"
#include <netdb.h>
#include <stdio.h>

static void test(const char *name, int use_acl)
{
	ACL_DNS_DB *res = NULL;
	ACL_HOSTNAME *h_host;
#if	!defined(MACOSX) && !defined(ACL_SUNOS5)
	struct hostent  h_buf; 
	int   errnum = 0;
	char  buf[4096];
#endif
	struct hostent *h_addrp = NULL;
	char **pptr;
	int   n; 
	ACL_ITER iter;
	struct in_addr addr;

	if (use_acl) {
		res = acl_gethostbyname(name, NULL);
		if (res == NULL) {
			printf("acl_gethostbyname %s error\n", name);
			return;
		}
	}
#if	!defined(MACOSX) && !defined(ACL_SUNOS5)
	else {
		res = acl_netdb_new(name);
		n = gethostbyname_r(name, &h_buf, buf, sizeof(buf), &h_addrp, &errnum);
		if (n) {
			printf("gethostbyname %s error\n", name);
			acl_netdb_free(res);
			return;
		}

		for (pptr = h_addrp->h_addr_list; *pptr != NULL; pptr++) {
			h_host = (ACL_HOSTNAME*) acl_mycalloc(1, sizeof(ACL_HOSTNAME));

			memset(&h_host->saddr, 0, sizeof(h_host->saddr));
			n = (int) sizeof(h_host->saddr.sin_addr) > h_addrp->h_length
				? h_addrp->h_length : (int) sizeof(h_host->saddr.sin_addr);
			memcpy(&h_host->saddr.sin_addr, *pptr, n);
			acl_inet_ntoa(h_host->saddr.sin_addr, h_host->ip, sizeof(h_host->ip));

			(void) acl_array_append(res->h_db, h_host);

			res->size++;
		}
	}
#endif

	printf("gethostbyname name: %s\n", name);
	if (res != NULL) {
		acl_foreach(iter, res) {
			const ACL_HOST_INFO *info;

			info = (const ACL_HOST_INFO*) iter.data;
			printf("\tip=%s; port=%d\n", info->ip, info->hport);
		}
		acl_netdb_free(res);
	}

	res = acl_netdb_new(name);
	memset(&addr, 0, sizeof(addr));
	addr.s_addr = inet_addr(name);

	printf("gethostbyaddr addr: %s\n", name);

	h_addrp = gethostbyaddr(&addr, sizeof(addr), AF_INET);
	if (h_addrp) {
		printf("h_name: %s\n", h_addrp->h_name);
		for (pptr = h_addrp->h_addr_list; *pptr != NULL; pptr++) {
			h_host = (ACL_HOSTNAME*) acl_mycalloc(1, sizeof(ACL_HOSTNAME));

			memset(&h_host->saddr, 0, sizeof(h_host->saddr));
			n = (int) sizeof(h_host->saddr.sin_addr) > h_addrp->h_length
				? h_addrp->h_length : (int) sizeof(h_host->saddr.sin_addr);
			memcpy(&h_host->saddr.sin_addr, *pptr, n);
			acl_inet_ntoa(h_host->saddr.sin_addr, h_host->ip, sizeof(h_host->ip));

			(void) acl_array_append(res->h_db, h_host);

			res->size++;
		}
	} else {
		printf("error: %s\n", acl_last_serror());
	}

	acl_foreach(iter, res) {
		const ACL_HOST_INFO *info;

		info = (const ACL_HOST_INFO*) iter.data;
		printf("\tip=%s; port=%d\n", info->ip, info->hport);
	}
	acl_netdb_free(res);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -a addr -r[use_acl_resolve]\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, use_acl = 0;
	char  addr[256];

	addr[0] = 0;
	while ((ch = getopt(argc, argv, "ha:r")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'a':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			use_acl = 1;
			break;
		default:
			break;
		}
	}

	if (addr[0])
		test(addr, use_acl);
	else
		usage(argv[0]);
	return (0);
}
