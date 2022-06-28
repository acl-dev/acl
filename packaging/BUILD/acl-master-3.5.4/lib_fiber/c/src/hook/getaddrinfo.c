#include "stdafx.h"
#include "dns/sane_inet.h"
#include "dns/resolver.h"
#include "common.h"
#include "fiber.h"
#include "hook.h"

#if defined(SYS_WIN)
#pragma comment(lib, "Iphlpapi.lib")
#endif

static struct addrinfo *create_addrinfo(const char *ip, short port,
	int iptype, int socktype, int flags)
{
	struct addrinfo *res;
	SOCK_ADDR sa;

	if (iptype == AF_INET) {
		sa.in.sin_family      = AF_INET;
		sa.in.sin_addr.s_addr = inet_addr(ip);
		sa.in.sin_port        = htons(port);
#ifdef AF_INET6
	} else if (iptype == AF_INET6) {
		char   buf[256], *ptr;
		struct sockaddr_in6 *in6;

		SAFE_STRNCPY(buf, ip, sizeof(buf));

		if ((ptr = strrchr(buf, '|'))) {
			*ptr++ = 0;
			port   = atoi(ptr);
		}

		/* when '%' was appended to the IPV6's addr */
		if ((ptr = strrchr(buf, '%'))) {
			*ptr++ = 0;
		}

		in6 = (struct sockaddr_in6 *) &sa;
		memset(in6, 0, sizeof(struct sockaddr_in6));
		sa.in6.sin6_family = AF_INET6;
		sa.in6.sin6_port   = htons(port);

		if (ptr && *ptr) {
			if (!(in6->sin6_scope_id = if_nametoindex(ptr))) {
				return NULL;
			}
		}
		if (inet_pton(AF_INET6, buf, &sa.in6.sin6_addr) <= 0) {
			return NULL;
		}
#endif
	} else {
		msg_error("%s: unknown ip type=%d", __FUNCTION__ , iptype);
		return NULL;
	}

	res = resolver_addrinfo_alloc(&sa.sa);
	res->ai_socktype = socktype;
	res->ai_flags    = flags;

	return res;
}

static struct addrinfo *check_local(const char *node, const char *service,
	const struct addrinfo *hints)
{
	const HOST_LOCAL *host;
	const char *ipaddr;
	int iptype;

	if (is_ipv4(node)) {
		iptype = AF_INET;
		ipaddr = node;
#ifdef	AF_INET6
	} else if (is_ipv6(node)) {
		iptype = AF_INET6;
		ipaddr = node;
#endif
	} else if ((host = find_from_localhost(node)) == NULL) {
		return NULL;
#ifdef	AF_INET6
	} else if (hints->ai_family == AF_INET6 && host->ipv6[0]) {
		iptype = hints->ai_family;
		ipaddr = host->ipv6;
#endif
	} else if (host->ipv4[0]) {
		iptype = AF_INET;
		ipaddr = host->ipv4;
#ifdef	AF_INET6
	} else if (host->ipv6[0]) {
		iptype = AF_INET6;
		ipaddr = host->ipv6;
#endif
	} else {
		return NULL;
	}

	if (ipaddr && *ipaddr) {
		int  port = get_service_port(service);
		int  socktype = hints ? hints->ai_socktype : SOCK_STREAM;
		struct addrinfo *ai = create_addrinfo(ipaddr, port, iptype,
		      socktype, hints ? hints->ai_flags : 0);
		if (ai) {
			ai->ai_next = NULL;
			return ai;
		}
	}
	return NULL;
}

int WINAPI acl_fiber_getaddrinfo(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res)
{
	struct addrinfo hints_tmp;

	if (sys_getaddrinfo == NULL) {
		hook_once();
	}

#ifndef	EAI_NODATA
# if	defined(NO_DATA)
#  define EAI_NODATA NO_DATA
# else
#  define EAI_NODATA 7
# endif
#endif
	if (!var_hook_sys_api) {
		return sys_getaddrinfo ? (*sys_getaddrinfo)
			(node, service, hints, res) : EAI_NODATA;
	}

	resolver_init_once();

	if (hints == NULL) {
		memset(&hints_tmp, 0, sizeof(hints_tmp));
		hints_tmp.ai_family   = PF_UNSPEC;
		hints_tmp.ai_socktype = SOCK_STREAM;  /* use TCP as default */
#ifdef	__APPLE__
		hints_tmp.ai_flags    = AI_DEFAULT;
#elif	defined(ANDROID)
		hints_tmp.ai_flags    = AI_ADDRCONFIG;
#elif	defined(SYS_WIN)
		hints_tmp.ai_protocol = IPPROTO_TCP;
# if _MSC_VER >= 1500
		hints_tmp.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#elif	!defined(__FreeBSD__)
		hints_tmp.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif
		hints = &hints_tmp;
	}

	*res = check_local(node, service, hints);
	if (*res != NULL) {
		return 0;
	}

	*res = resolver_getaddrinfo(node, service, hints);
	if (*res == NULL) {
		return EAI_NODATA;
	}

	return 0;
}

void WINAPI acl_fiber_freeaddrinfo(struct addrinfo *res)
{
	if (sys_freeaddrinfo == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		if (sys_freeaddrinfo) {
			(*sys_freeaddrinfo)(res);
		}
		return;
	}

	resolver_freeaddrinfo(res);
}

#ifdef SYS_UNIX

int getaddrinfo(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res)
{
	return acl_fiber_getaddrinfo(node, service, hints, res);
}

void freeaddrinfo(struct addrinfo *res)
{
	acl_fiber_freeaddrinfo(res);
}

#endif
