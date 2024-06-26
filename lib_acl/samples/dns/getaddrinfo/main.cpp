#include "lib_acl.h"
#include <string>

static bool test_getaddrinfo(const char *name, const char *service)
{
	int    err;
	struct addrinfo hints, *res0, *res;
	char   ip[64];

	memset(&hints, 0, sizeof(hints));

	hints.ai_family   = PF_UNSPEC;
//	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

#ifdef  ACL_MACOSX
	hints.ai_flags    = AI_DEFAULT;
#elif   defined(ACL_ANDROID)
	hints.ai_flags    = AI_ADDRCONFIG;
#elif defined(ACL_WINDOWS)
	hints.ai_protocol = IPPROTO_UDP;
# if _MSC_VER >= 1500
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#else
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif
	if ((err = getaddrinfo(name, service && *service ? service : NULL, &hints, &res0))) {
		printf("%s(%d): getaddrinfo error %s, peer=%s\r\n",
			__FILE__, __LINE__, gai_strerror(err), name);
		return false;
	}

	for (res = res0; res != NULL; res = res->ai_next) {
		if (res->ai_family == AF_INET) {
			ACL_SOCKADDR *sa = (ACL_SOCKADDR*) res->ai_addr;
			if (inet_ntop(res->ai_family, &sa->in.sin_addr,
				ip, sizeof(ip)) == NULL) {
				printf("inet_ntop error\r\n");
			} else {
				printf("Addr=%s, Port=%d, type=IPv4\r\n",
					ip, ntohs(sa->in.sin_port));
			}
		} else if (res->ai_family == AF_INET6) {
			ACL_SOCKADDR *sa = (ACL_SOCKADDR*) res->ai_addr;
			if (inet_ntop(res->ai_family, &sa->in6.sin6_addr,
				ip, sizeof(ip)) == NULL) {
				printf("inet_ntop error\r\n");
			} else {
				printf("Addr=%s, Port=%d, type=IPv6\r\n",
					ip, ntohs(sa->in6.sin6_port));
			}
		} else {
			printf("Unknown ai_family=%d\r\n", res->ai_family);
		}
	}

	freeaddrinfo(res0);
	return true;
}

static bool test_acl_gethostbyname(const char *name)
{
	ACL_DNS_DB *res = acl_gethostbyname(name, NULL);
	if (res == NULL) {
		printf("acl_gethostbyname error\r\n");
		return false;
	}

	ACL_ITER iter;
	acl_foreach(iter, res) {
		const ACL_HOST_INFO *info = (const ACL_HOST_INFO*) iter.data;
		printf("IP=%s, Port=%d\r\n", info->ip, info->hport);
	}

	acl_netdb_free(res);
	return true;
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -n domain -s service -T [if use acl_gethostbyname]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch;
	bool  use_acl = false;
	std::string name, service;

	while ((ch = getopt(argc, argv, "hn:s:T")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			name = optarg;
			break;
		case 's':
			service = optarg;
			break;
		case 'T':
			use_acl = true;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	test_getaddrinfo(name.c_str(), service.c_str());

	printf("---------------------------------------------------------\r\n");

	if (use_acl) {
		test_acl_gethostbyname(name.c_str());
	}
	return 0;
}
