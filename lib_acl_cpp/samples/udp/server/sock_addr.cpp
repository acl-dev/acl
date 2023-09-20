#include "stdafx.h"
#include "sock_addr.h"

static bool host_port(char *buf, char **host, char **port)
{
	const char *ptr = acl_host_port(buf, host, "", port, (char*) NULL);

	if (ptr != NULL) {
		acl_msg_error("%s(%d): invalid addr %s, %s",
			__FILE__, __LINE__, buf, ptr);
		return false;
	}

	if (*port == NULL || atoi(*port) < 0) {
		acl_msg_error("%s(%d): invalid port: %s, addr: %s",
			__FILE__, __LINE__, *port ? *port : "null", buf);
		return false;
	}

	if (*host && **host == 0)
		*host = 0;
	if (*host == NULL)
		*host = "0";

	return true;
}

struct addrinfo *host_addrinfo(const char *addr)
{
	int    err;
	struct addrinfo hints, *res0;
	char  *buf = acl_mystrdup(addr), *host = NULL, *port = NULL;

	if (host_port(buf, &host, &port) == false) {
		acl_myfree(buf);
		return NULL;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
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
	if ((err = getaddrinfo(host, port, &hints, &res0))) {
		acl_msg_error("%s(%d): getaddrinfo error %s, peer=%s",
			__FILE__, __LINE__, gai_strerror(err), host);
		acl_myfree(buf);
		return NULL;
	}

	acl_myfree(buf);
	return res0;
}
