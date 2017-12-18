#include "stdafx.h"
#include "fiber/lib_fiber.h"
#include "common.h"
#include "dns/sane_inet.h"
#include "dns/netdb.h"
#include "dns/res.h"
#include "event.h"
#include "fiber.h"

typedef int (*gethostbyname_r_fn)(const char *, struct hostent *, char *,
	size_t, struct hostent **, int *);
typedef int (*getaddrinfo_fn)(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res);
typedef void (*freeaddrinfo_fn)(struct addrinfo *res);

static gethostbyname_r_fn __sys_gethostbyname_r = NULL;
static getaddrinfo_fn     __sys_getaddrinfo     = NULL;
static freeaddrinfo_fn    __sys_freeaddrinfo    = NULL;

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

	__sys_gethostbyname_r = (gethostbyname_r_fn) dlsym(RTLD_NEXT,
			"gethostbyname_r");
	assert(__sys_gethostbyname_r);

	__sys_getaddrinfo = (getaddrinfo_fn) dlsym(RTLD_NEXT, "getaddrinfo");
	assert(__sys_getaddrinfo);

	__sys_freeaddrinfo = (freeaddrinfo_fn) dlsym(RTLD_NEXT,
			"freeaddrinfo");
	assert(__sys_freeaddrinfo);

	(void) pthread_mutex_unlock(&__lock);
}

/****************************************************************************/

static const char *__dns_ip_default = "8.8.8.8";
static char __dns_ip[128] = { 0 };
static int  __dns_port = 53;

void acl_fiber_set_dns(const char* ip, int port)
{
	if (ip == NULL || *ip == 0) {
		__dns_ip[0] = 0;
	} else {
		snprintf(__dns_ip, sizeof(__dns_ip), "%s", ip);
	}

	__dns_port = port > 0 ? port : 53;
}

#define SKIP_WHILE(cond, cp) { while (*cp && (cond)) cp++; }

static void get_dns(char *ip, size_t size)
{
	const char *filepath = "/etc/resolv.conf";
	FILE *fp;
	char buf[4096], *ptr;
	ARGV *tokens;
	static pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;

	(void) pthread_mutex_lock(&__lock);

	if (__dns_ip[0] != 0) {
		SAFE_STRNCPY(ip, __dns_ip, size);
		(void) pthread_mutex_unlock(&__lock);
		return;
	}

	fp = fopen(filepath, "r");
	if (fp == NULL) {
		(void) pthread_mutex_unlock(&__lock);
		SAFE_STRNCPY(ip, __dns_ip_default, size);
		return;
	}

	__dns_ip[0] = 0;

	while (!feof(fp)) {
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			break;
		}
		ptr = buf;
		SKIP_WHILE(*ptr == ' ' || *ptr == '\t' || *ptr == '#', ptr);
		if (*ptr == 0) {
			continue;
		}

		tokens = argv_split(ptr, " \t");
		if (tokens->argc < 2) {
			argv_free(tokens);
			continue;
		}

		if (strcasecmp(tokens->argv[0], "nameserver") != 0) {
			argv_free(tokens);
			continue;
		}

		if (!is_ip(tokens->argv[1])) {
			argv_free(tokens);
			continue;
		}

		snprintf(__dns_ip, sizeof(__dns_ip), "%s", tokens->argv[1]);
		argv_free(tokens);
		break;
	}

	fclose(fp);

	if (__dns_ip[0] == 0) {
		(void) pthread_mutex_unlock(&__lock);
		SAFE_STRNCPY(ip, __dns_ip_default, size);
		return;
	}

	SAFE_STRNCPY(ip, __dns_ip, size);
	(void) pthread_mutex_unlock(&__lock);
}

struct hostent *gethostbyname(const char *name)
{
	static __thread struct hostent ret, *result;
#define BUF_LEN	4096
	static __thread char buf[BUF_LEN];

	return gethostbyname_r(name, &ret, buf, BUF_LEN, &result, &h_errno)
		== 0 ? result : NULL;
}

int gethostbyname_r(const char *name, struct hostent *ret,
	char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
	RES *ns = NULL;
	DNS_DB *res = NULL;
	size_t n = 0, len, i = 0;
	ITER iter;
	char dns_ip[64];

#define	RETURN(x) do { \
	if (res) \
		netdb_free(res); \
	if (ns) \
		res_free(ns); \
	return (x); \
} while (0)

	if (__sys_gethostbyname_r == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		return __sys_gethostbyname_r ?  __sys_gethostbyname_r
			(name, ret, buf, buflen, result, h_errnop) : -1;
	}

	get_dns(dns_ip, sizeof(dns_ip));

	memset(ret, 0, sizeof(struct hostent));
	memset(buf, 0, buflen);

	ns = res_new(dns_ip, __dns_port);
	res = res_lookup(ns, name);

	if (res == NULL) {
		msg_error("%s(%d), %s: res_lookup NULL, name: %s,"
			" dns_ip: %s, dns_port: %d", __FILE__, __LINE__,
			__FUNCTION__, name, dns_ip, __dns_port);
		if (h_errnop) {
			*h_errnop = HOST_NOT_FOUND;
		}
		RETURN (-1);
	}

	len = strlen(name);
	n += len;
	if (n >= buflen) {
		msg_error("%s(%d), %s: n(%d) > buflen(%d)", __FILE__,
			__LINE__, __FUNCTION__, (int) n, (int) buflen);
		if (h_errnop) {
			*h_errnop = ERANGE;
		}
		RETURN (-1);
	}
	memcpy(buf, name, len);
	buf[len] = 0;
	ret->h_name = buf;
	buf += len + 1;

#define MAX_COUNT	64
	len = 8 * MAX_COUNT;
	n += len;
	if (n >= buflen) {
		msg_error("%s(%d), %s: n(%d) > buflen(%d)", __FILE__,
			__LINE__, __FUNCTION__, (int) n, (int) buflen);
		if (h_errnop) {
			*h_errnop = ERANGE;
		}
		RETURN (-1);
	}
	ret->h_addr_list = (char**) buf;
	buf += len;

	foreach(iter, res) {
		HOSTNAME *h = (HOSTNAME*) iter.data;
		struct in_addr addr;

		len = sizeof(struct in_addr);
		n += len;
		if (n > buflen) {
			break;
		}

		memset(&addr, 0, sizeof(addr));
		addr.s_addr = inet_addr(h->ip);
		memcpy(buf, &addr, len);

		if (i >= MAX_COUNT) {
			break;
		}
		ret->h_addr_list[i++] = buf;
		buf += len;
		ret->h_length += len;
	}

	if (i > 0) {
		*result = ret;
		RETURN (0);
	}

	msg_error("%s(%d), %s: i == 0, n: %d, buflen: %d",
		__FILE__, __LINE__, __FUNCTION__, (int) n, (int) buflen);

	if (h_errnop) {
		*h_errnop = ERANGE;
	}

	RETURN (-1);
}

static int get_port(const char *service, int socktype)
{
	const char *filepath = "/etc/services";
	FILE *fp;
	char buf[4096], *ptr, *sport, *transport;
	ARGV *tokens;
	int port = 0, type;

	if (service == NULL || *service == 0) {
		return 0;
	}

	if (alldig(service)) {
		return atoi(service);
	}

	fp = fopen(filepath, "r");
	if (fp == NULL) {
		return 0;
	}

	while (!feof(fp)) {
		if (fgets(buf, sizeof(buf), fp) == NULL) {
			break;
		}

		ptr = buf;
		SKIP_WHILE(*ptr == ' ' || *ptr == '\t' || *ptr == '#', ptr);
		if (*ptr == 0) {
			continue;
		}

		tokens = argv_split(ptr, " \t");
		if (tokens->argc < 2) {
			argv_free(tokens);
			continue;
		}

		sport = tokens->argv[1];
		transport = strchr(sport, '/');
		if (transport == NULL) {
			argv_free(tokens);
			continue;
		}
		*transport++ = 0;
		if (!alldig(sport)) {
			argv_free(tokens);
			continue;
		}

		if (strcasecmp(transport, "tcp") == 0) {
			type = SOCK_STREAM;
		} else if (strcasecmp(transport, "udp") == 0) {
			type = SOCK_DGRAM;
		} else {
			argv_free(tokens);
			continue;
		}

		if (type == socktype && !strcmp(tokens->argv[0], service)) {
			port = atoi(sport);
			argv_free(tokens);
			break;
		}

		argv_free(tokens);
	}

	fclose(fp);
	return port;
}

static struct addrinfo *create_addrinfo(const char *ip, short port,
	int socktype, int flags)
{
	struct addrinfo *res;
	size_t addrlen = sizeof(struct SOCK_ADDR);
	struct SOCK_ADDR *sa;

	if (is_ipv4(ip)) {
		sa = (struct SOCK_ADDR *) calloc(1, addrlen);
		sa->sa.in.sin_family      = AF_INET;
		sa->sa.in.sin_addr.s_addr = inet_addr(ip);
		sa->sa.in.sin_port        = htons(port);
	}
#ifdef AF_INET6
	else if (is_ipv6(ip)) {
		sa = (struct SOCK_ADDR *) calloc(1, addrlen);
		sa->sa.in6.sin6_family = AF_INET6;
		sa->sa.in6.sin6_port   = htons(port);
		if (inet_pton(AF_INET6, ip, &sa->sa.in6.sin6_addr) <= 0) {
			free(sa);
			return NULL;
		}
	}
#endif
	else {
		return NULL;
	}

	res = (struct addrinfo *) calloc(1, sizeof(struct addrinfo));
	res->ai_family   = sa->sa.sa.sa_family;
	res->ai_socktype = socktype;
	res->ai_flags    = flags;
	res->ai_addrlen  = (socklen_t) addrlen;
	res->ai_addr     = (struct sockaddr *) sa;

	return res;
}

int getaddrinfo(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res)
{
	RES *ns;
	DNS_DB *db;
	short port;
	ITER iter;
	char dns_ip[64];
	int  socktype = hints ? hints->ai_socktype : SOCK_STREAM;

	if (__sys_getaddrinfo == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		return __sys_getaddrinfo ?
			__sys_getaddrinfo(node, service, hints, res) : -1;
	}

	port = get_port(service, socktype);

	*res = NULL;

	if (is_ip(node)) {
		struct addrinfo *ai = create_addrinfo(node, port, socktype,
			hints ? hints->ai_flags : 0);
		if (ai) {
			ai->ai_next = *res;
			*res = ai;
			return 0;
		} else {
			return EAI_NODATA;
		}
	}

	get_dns(dns_ip, sizeof(dns_ip));

	ns = res_new(dns_ip, __dns_port);
	db = res_lookup(ns, node);
	if (db == NULL) {
		msg_error("%s(%d), %s: res_lookup NULL, node: %s,"
			" dns_ip: %s, dns_port: %d", __FILE__, __LINE__,
			__FUNCTION__, node, dns_ip, __dns_port);
		res_free(ns);
		return EAI_NODATA;
	}

	foreach(iter, db) {
		HOSTNAME *h = (HOSTNAME *) iter.data;
		struct addrinfo *ai = create_addrinfo(h->ip, port, socktype,
			hints ? hints->ai_flags : 0);
		if (ai) {
			ai->ai_next = *res;
			*res = ai;
		}
	}

	netdb_free(db);
	res_free(ns);
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
		free(tmp->ai_addr);
		free(tmp);
	}
}
