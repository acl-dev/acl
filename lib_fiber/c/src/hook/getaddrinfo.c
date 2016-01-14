#include "stdafx.h"
#include <netdb.h>
#include "dns/dns.h"
#include "dns/sane_inet.h"
#include "common.h"
#include "hook.h"
#include "fiber.h"

typedef int (*getaddrinfo_fn)(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res);
typedef void (*freeaddrinfo_fn)(struct addrinfo *res);

static getaddrinfo_fn  __sys_getaddrinfo  = NULL;
static freeaddrinfo_fn __sys_freeaddrinfo = NULL;

static void hook_init(void)
{
	static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) pthread_mutex_lock(&__lock);

	if (__called) {
		(void) pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_getaddrinfo = (getaddrinfo_fn) dlsym(RTLD_NEXT, "getaddrinfo");
	assert(__sys_getaddrinfo);

	__sys_freeaddrinfo = (freeaddrinfo_fn) dlsym(RTLD_NEXT,
			"freeaddrinfo");
	assert(__sys_freeaddrinfo);

	(void) pthread_mutex_unlock(&__lock);
}

/****************************************************************************/

static struct addrinfo *create_addrinfo(const char *ip, short port,
	int socktype, int flags)
{
	struct addrinfo *res;
	size_t addrlen = sizeof(struct SOCK_ADDR);
	struct SOCK_ADDR sa;

	if (is_ipv4(ip)) {
		sa.sa.in.sin_family      = AF_INET;
		sa.sa.in.sin_addr.s_addr = inet_addr(ip);
		sa.sa.in.sin_port        = htons(port);
	}
#ifdef AF_INET6
	else if (is_ipv6(ip)) {
		sa.sa.in6.sin6_family = AF_INET6;
		sa.sa.in6.sin6_port   = htons(port);
		if (inet_pton(AF_INET6, ip, &sa.sa.in6.sin6_addr) <= 0) {
			return NULL;
		}
	}
#endif
	else {
		return NULL;
	}

	res = (struct addrinfo *) calloc(1, sizeof(*res) + addrlen);
	res->ai_family   = sa.sa.sa.sa_family;
	res->ai_socktype = socktype;
	res->ai_flags    = flags;
	res->ai_addrlen  = (socklen_t) addrlen;
	res->ai_addr     = (struct sockaddr *) 
		memcpy((unsigned char *) res + sizeof(*res), &sa, addrlen);

	return res;
}

static void saveaddrinfo(struct dns_addrinfo *ai, struct addrinfo **res)
{
	struct addrinfo *ent = NULL;

	while (1) {
		int err = dns_ai_nextent(&ent, ai);
		if (err != 0 || ent == NULL) {
			break;
		}

		if (ent->ai_family != AF_INET && ent->ai_family != AF_INET6) {
			free(ent);
			continue;
		}

		if (*res == NULL) {
			*res = ent;
			(*res)->ai_next = NULL;
		} else {
			ent->ai_next = *res;
			*res = ent;
		}
	}
}

int getaddrinfo(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res)
{
	int err;

	if (__sys_getaddrinfo == NULL) {
		hook_init();
	}

#ifndef	EAI_NODATA
# if	defined(NO_DATA)
#  define EAI_NODATA NO_DATA
# else
#  define EAI_NODATA 7
# endif
#endif
	if (!var_hook_sys_api) {
		return __sys_getaddrinfo ? __sys_getaddrinfo
			(node, service, hints, res) : EAI_NODATA;
	}

	if (var_dns_conf == NULL || var_dns_hosts == NULL) {
		dns_init();
	}

	*res = NULL;
	if (is_ip(node)) {
		int  port = service ? atoi(service) : -1;
		int  socktype = hints ? hints->ai_socktype : SOCK_STREAM;
		struct addrinfo *ai = create_addrinfo(node, port, socktype,
			hints ? hints->ai_flags : 0);
		if (ai) {
			ai->ai_next = NULL;
			*res = ai;
			return 0;
		} else {
			return EAI_NODATA;
		}
	}

	struct dns_addrinfo *dai;
	struct dns_resolver *resolver;

	if (!(resolver = dns_res_open(var_dns_conf, var_dns_hosts,
		var_dns_hints, NULL, dns_opts(), &err))) {

		msg_error("%s(%d): dns_res_open error=%s",
			__FUNCTION__, __LINE__, dns_strerror(err));
		return EAI_SYSTEM;
	}

	dai = dns_ai_open(node, service, DNS_T_A, hints, resolver, &err);
	if (dai == NULL) {
		dns_res_close(resolver);
		msg_error("%s(%d): dns_res_close error=%s",
			__FUNCTION__, __LINE__, dns_strerror(err));
		return EAI_SERVICE;
	}

	saveaddrinfo(dai, res);

	dns_res_close(resolver);
	dns_ai_close(dai);

	if (*res == NULL) {
		return EAI_NODATA;
	}

	return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
	if (__sys_freeaddrinfo == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		if (__sys_freeaddrinfo) {
			__sys_freeaddrinfo(res);
		}
		return;
	}

	while (res) {
		struct addrinfo *tmp = res;
		res = res->ai_next;
		free(tmp);
	}
}
