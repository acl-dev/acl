#include "StdAfx.h"

#ifdef ACL_WINDOWS
#pragma comment(lib, "Iphlpapi.lib")
#endif

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <ctype.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef	ACL_UNIX
#include <net/if.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "net/acl_sane_inet.h"
#include "net/acl_valid_hostname.h"

#endif

const char *acl_inet_ntop4(const unsigned char *src, char *dst, size_t size)
{
	const size_t MIN_SIZE = 16; /* space for 255.255.255.255\0 */
	int   n = 0;
	char *next = dst;

	if (size < MIN_SIZE)
		return NULL;

	do {
		unsigned char u = *src++;
		if (u > 99) {
			*next++ = '0' + u/100;
			u %= 100;
			*next++ = '0' + u/10;
			u %= 10;
		}
		else if (u > 9) {
			*next++ = '0' + u/10;
			u %= 10;
		}
		*next++ = '0' + u;
		*next++ = '.';
		n++;
	} while (n < 4);
	*--next = 0;
	return (dst);
}

const char *acl_inet_ntoa(const struct in_addr in, char *buf, size_t size)
{
#if 0
	unsigned char *src = (unsigned char *) &in.s_addr;

	return acl_inet_ntop4(src, buf, size);
#else
	struct sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	memcpy(&sin.sin_addr, &in, sizeof(sin.sin_addr));

	if (!acl_inet_ntop((const struct sockaddr*) &sin, buf, size)) {
		return NULL;
	} else {
		return buf;
	}

#endif
}

#ifdef AF_INET6
const char *acl_inet6_ntoa(const struct in6_addr in6, char *buf, size_t size)
{
	struct sockaddr_in6 sin6;

	memset(&sin6, 0, sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	memcpy(&sin6.sin6_addr, &in6, sizeof(sin6.sin6_addr));

	if (!acl_inet_ntop((const struct sockaddr*) &sin6, buf, size)) {
		return NULL;
	} else {
		return buf;
	}
}
#endif

int acl_is_ipv4(const char *ip)
{       
	return acl_valid_ipv4_hostaddr(ip, 0);
}

int acl_is_ipv6(const char *ip)
{
	return acl_valid_ipv6_hostaddr(ip, 0);
}

int acl_is_ip(const char *ip)
{
	return acl_is_ipv4(ip) || acl_is_ipv6(ip);
}

int acl_ipv4_addr_valid(const char *addr)
{
	const char *ptr = addr;
	int   n, k;

	if (addr == NULL || *addr == 0)
		return (0);
	k = 3;
	while (*ptr && *ptr != '.') {
		n = *ptr;
		if (n < '0' || n > '9')
			return (0);
		ptr++;
		k--;
		if (k < 0)
			return (0);
	}
	if (*ptr == 0)
		return (0);
	
	k = 3;
	ptr++;
	while (*ptr && *ptr != '.') {
		n = *ptr;
		if (n < '0' || n > '9')
			return (0);
		ptr++;
		k--;
		if (k < 0)
			return (0);
	}
	if (*ptr == 0)
		return (0);
	
	k = 3;
	ptr++;
	while (*ptr && *ptr != '.') {
		n = *ptr;
		if (n < '0' || n > '9')
			return (0);
		ptr++;
		k--;
		if (k < 0)
			return (0);
	}
	if (*ptr == 0)
		return (0);
	
	k = 3;
	ptr++;
	while (*ptr && *ptr != ':') {
		n = *ptr;
		if (n < '0' || n > '9')
			return (0);
		ptr++;
		k--;
		if (k < 0)
			return (0);
	}
	if (*ptr == 0)
		return (0);

	ptr++;
	n = atoi(ptr);
	if (n < 0 || n > 65535)
		return (0);
	return (1);
}

#define IPLEN	64

size_t acl_inet_ntop(const struct sockaddr *sa, char *buf, size_t size)
{
	if (sa->sa_family == AF_INET) {
		int    port;
		char   ip[IPLEN];
		struct sockaddr_in *in = (struct sockaddr_in*) sa;

		if (!inet_ntop(sa->sa_family, &in->sin_addr, ip, IPLEN)) {
			return 0;
		}

		port = ntohs(in->sin_port);
		if (port > 0) {
			snprintf(buf, size, "%s:%d", ip, port);
		} else {
			snprintf(buf, size, "%s", ip);
		}
		return sizeof(struct sockaddr_in);
#ifdef AF_INET6
	} else if (sa->sa_family == AF_INET6) {
#ifndef IF_NAMESIZE
#define IF_NAMESIZE 256
#endif

		int    port;
		char   ip[IPLEN], ifname[IF_NAMESIZE], *ptr;
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*) sa;

		if (!inet_ntop(sa->sa_family, &in6->sin6_addr, ip, IPLEN)) {
			return 0;
		}

#if defined(ACL_UNIX) || (defined(ACL_WINDOWS) && _MSC_VER >= 1600)
		ptr = (char*) if_indextoname(in6->sin6_scope_id, ifname);
		if (ptr == NULL) {
			ifname[0] = 0;
		}
# else
		ifname[0] = 0;
#endif

		port = ntohs(in6->sin6_port);
		if (port <= 0) {
			if (strcmp(ip, "::1") == 0) {
				snprintf(buf, size, "%s", ip);
			} else if (ifname[0] != 0) {
				snprintf(buf, size, "%s%%%s", ip, ifname);
			} else {
				snprintf(buf, size, "%s", ip);
			}
		} else if (strcmp(ip, "::1") == 0) {  /* for local IPV6 */
			snprintf(buf, size, "%s%c%d", ip, ACL_ADDR_SEP, port);
		} else if (ifname[0] != 0) {
			snprintf(buf, size, "%s%%%s%c%d",
				ip, ifname, ACL_ADDR_SEP, port);
		} else {
			snprintf(buf, size, "%s%c%d", ip, ACL_ADDR_SEP, port);
		}

		return sizeof(struct sockaddr_in6);
#endif
#ifdef ACL_UNIX
	} else if (sa->sa_family == AF_UNIX) {
		struct sockaddr_un *un = (struct sockaddr_un *) sa;

		ACL_SAFE_STRNCPY(buf, un->sun_path, size);
		return sizeof(struct sockaddr_un);
#endif
	} else {
		return 0;
	}
}

size_t acl_inet_pton(int af, const char *src, struct sockaddr *dst)
{
	if (af == AF_INET) {
		char   buf[256], *ptr;
		int    port = 0;
		struct sockaddr_in *in;

		ACL_SAFE_STRNCPY(buf, src, sizeof(buf));

		if ((ptr = strrchr(buf, ':'))
			|| (ptr = strrchr(buf, ACL_ADDR_SEP))) {

			*ptr++ = 0;
			port   = atoi(ptr);
		}

		in = (struct sockaddr_in *) dst;
		if (inet_pton(af, buf, &in->sin_addr) == 0) {
			return 0;
		}

		in->sin_port   = htons(port);
		dst->sa_family = AF_INET;
		return sizeof(struct sockaddr_in);
#ifdef AF_INET6
	} else if (af == AF_INET6) {
# ifdef INET_PTON_USE_GETADDRINFO
		struct addrinfo *res = acl_host_addrinfo(src, 0);
		size_t addrlen;
		if (res == NULL) {
			return 0;
		}
		addrlen = (size_t) res->ai_addrlen;
		memcpy(dst, res->ai_addr, res->ai_addrlen);
		freeaddrinfo(res);

		return addrlen;
# else
		int    port = 0;
		char   buf[256], *ptr;
		struct sockaddr_in6 *in6;

		ACL_SAFE_STRNCPY(buf, src, sizeof(buf));

		if ((ptr = strrchr(buf, ACL_ADDR_SEP))) {
			*ptr++ = 0;
			port   = atoi(ptr);
		}

		/* when '%' was appended to the IPV6's addr */
		if ((ptr = strrchr(buf, '%'))) {
			*ptr++ = 0;
		}

		in6 = (struct sockaddr_in6 *) dst;
		memset(in6, 0, sizeof(struct sockaddr_in6));

		in6->sin6_family = AF_INET6;
		in6->sin6_port   = htons(port);
#if defined(ACL_UNIX) || (defined(ACL_WINDOWS) && _MSC_VER >= 1600)
		if (ptr && *ptr && !(in6->sin6_scope_id = if_nametoindex(ptr))) {
			acl_msg_error("%s(%d): if_nametoindex error %s",
				__FUNCTION__, __LINE__, acl_last_serror());
			return 0;
		}
#endif

		if (inet_pton(af, buf, &in6->sin6_addr) == 0) {
			return 0;
		}
		return sizeof(struct sockaddr_in6);
# endif  /* !IPV6_INET_PTON_HAS_BUG */
#endif
#ifdef ACL_UNIX
	} else if (af == AF_UNIX) {
		struct sockaddr_un *un = (struct sockaddr_un *) dst;
		size_t len = strlen(src) + 1;

		if (sizeof(un->sun_path) < len) {
			len = sizeof(un->sun_path);
		}

		dst->sa_family = AF_UNIX;
# ifdef HAS_SUN_LEN
		un->sun_len    = len + 1;
# endif
		ACL_SAFE_STRNCPY(un->sun_path, src, len);
		return sizeof(struct sockaddr_un);
#endif
	} else {
		acl_msg_error("%s(%d): invalid af=%d", __FUNCTION__, __LINE__, af);
		return 0;
	}
}

size_t acl_sane_pton(const char *src, struct sockaddr *dst)
{
	int af;

	if (acl_valid_ipv4_hostaddr(src, 0)) {
		af = AF_INET;
	} else if (acl_valid_ipv6_hostaddr(src, 0)) {
		af = AF_INET6;
	} else if (acl_valid_unix(src)) {
		af = AF_UNIX;
	} else {
		return 0;
	}

	return acl_inet_pton(af, src, dst);
}
