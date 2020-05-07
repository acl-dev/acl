#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_stringops.h"
#include "net/acl_sane_inet.h"
#include "net/acl_sane_socket.h"
#include "net/acl_valid_hostname.h"
#include "net/acl_ifconf.h"

#endif

#define SAFE_COPY(x, y) ACL_SAFE_STRNCPY((x), (y), sizeof(x))

static const ACL_IFADDR *iter_head(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->dlen = -1;
	iter->key  = NULL;
	iter->klen = -1;

	iter->i = 0;
	if (iter->i >= ifconf->length) {
		iter->data = iter->ptr = NULL;
		return iter->ptr;
	}

	iter->size = ifconf->length;
	iter->ptr  = iter->data = &ifconf->addrs[0];
	return iter->ptr;
}

static const ACL_IFADDR *iter_next(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->i++;
	if (iter->i >= ifconf->length)
		iter->data = iter->ptr = NULL;
	else
		iter->data = iter->ptr = &ifconf->addrs[iter->i];
	return iter->ptr;
}

static const ACL_IFADDR *iter_tail(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->dlen = -1;
	iter->key  = NULL;
	iter->klen = -1;

	iter->i    = ifconf->length - 1;
	iter->size = ifconf->length;
	if (iter->i < 0)
		iter->data = iter->ptr = NULL;
	else
		iter->ptr = iter->data = &ifconf->addrs[iter->i];
	return iter->ptr;
}

static const ACL_IFADDR *iter_prev(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->i--;
	if (iter->i < 0)
		iter->data = iter->ptr = NULL;
	else
		iter->data = iter->ptr = &ifconf->addrs[iter->i];
	return iter->ptr;
}

static ACL_IFADDR *ifaddr_clone(const ACL_IFADDR *ifaddr, int port)
{
	ACL_IFADDR *addr = (ACL_IFADDR *) acl_mycalloc(1, sizeof(ACL_IFADDR));

	SAFE_COPY(addr->name, ifaddr->name);
#ifdef ACL_WINDOWS
	SAFE_COPY(addr->desc, ifaddr->desc);
#endif
	if (port >= 0) {
		char  sep;
		char  buf[sizeof(ifaddr->addr) + 11];
		if (ifaddr->saddr.sa.sa_family == AF_INET) {
			addr->saddr.in.sin_port = htons(port);
			sep = ':';
#ifdef AF_INET6
		} else if (ifaddr->saddr.sa.sa_family == AF_INET6) {
			addr->saddr.in6.sin6_port = htons(port);
			sep = ACL_ADDR_SEP;
#endif
		} else {
			sep = ':';  /* xxx */
		}

		/* in order to avoid the gcc7.x warning, we should copy the
		 * data into buf and copy to the addr from buf.
		 */
		snprintf(buf, sizeof(buf), "%s%c%d", ifaddr->addr, sep, port);
		SAFE_COPY(addr->addr, buf);
	} else {
		SAFE_COPY(addr->addr, ifaddr->addr);
	}
	memcpy(&addr->saddr, &ifaddr->saddr, sizeof(ACL_SOCKADDR));

	return addr;
}

static void ifaddr_copy(ACL_IFADDR *to, const ACL_IFADDR *from)
{
	SAFE_COPY(to->name, from->name);
#ifdef ACL_WINDOWS
	SAFE_COPY(to->desc, from->desc);
#endif
	SAFE_COPY(to->addr, from->addr);
	memcpy(&to->saddr, &from->saddr, sizeof(ACL_SOCKADDR));
}

static ACL_IFCONF *ifconf_create(int max)
{
	ACL_IFCONF *ifconf = (ACL_IFCONF*) acl_mymalloc(sizeof(ACL_IFCONF));
	ifconf->length = max;
	ifconf->addrs  = (ACL_IFADDR*) acl_mycalloc(max, sizeof(ACL_IFADDR));

	/* set the iterator callback */
	ifconf->iter_head = iter_head;
	ifconf->iter_next = iter_next;
	ifconf->iter_tail = iter_tail;
	ifconf->iter_prev = iter_prev;

	return ifconf;
}

#ifdef	ACL_UNIX

#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#ifdef	ACL_SUNOS5
#include <sys/sockio.h>
#endif

# if (defined(ACL_LINUX) || defined(ACL_MACOSX)) && !defined(ACL_ANDROID)
#include <ifaddrs.h>

ACL_IFCONF *acl_get_ifaddrs(void)
{
	struct ifaddrs* ifaddrs, *ifa;
	char host[NI_MAXHOST];
	ACL_IFCONF *ifconf;
	ACL_ARRAY  *addrs;
	ACL_ITER    iter;
	int   i;

	if (getifaddrs(&ifaddrs) == -1) {
		acl_msg_error("%s(%d): getifaddrs error=%s",
			__FUNCTION__, __LINE__, acl_last_serror());
		return NULL;
	}

	addrs = acl_array_create(10);

	for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
		ACL_IFADDR *addr;
		int    family, n;
		size_t len;

		if (ifa->ifa_addr == NULL) {
			continue;
		}

		if (!(ifa->ifa_flags & IFF_UP)
			&& !(ifa->ifa_flags & IFF_RUNNING)) {
			continue;
		}

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET) {
			len = sizeof(struct sockaddr_in);
#ifdef AF_INET6
		} else if (family == AF_INET6) {
			len = sizeof(struct sockaddr_in6);
#endif
		} else {
			continue;
		}

		n = getnameinfo(ifa->ifa_addr, (socklen_t) len, host, NI_MAXHOST,
				NULL, 0, NI_NUMERICHOST);
		if (n != 0) {
			acl_msg_error("%s(%d): getnameinfo error=%s, "
				"ifa_name=%s", __FUNCTION__, __LINE__,
				gai_strerror(n), ifa->ifa_name);
			continue;
		}

		addr = (ACL_IFADDR *) acl_mycalloc(1, sizeof(ACL_IFADDR));
		SAFE_COPY(addr->name, ifa->ifa_name);
		SAFE_COPY(addr->addr, host);
		memcpy(&addr->saddr, ifa->ifa_addr, len);

		acl_array_append(addrs, addr);
	}

	freeifaddrs(ifaddrs);

	if (acl_array_size(addrs) <= 0) {
		acl_array_free(addrs, NULL);
		return NULL;
	}

	ifconf = ifconf_create(acl_array_size(addrs));
	i = 0;
	acl_foreach(iter, addrs) {
		ACL_IFADDR *ifaddr = (ACL_IFADDR *) iter.data;

		ifaddr_copy(&ifconf->addrs[i], ifaddr);
		acl_myfree(ifaddr);
		i++;
	}

	acl_array_free(addrs, NULL);
	return ifconf;
}

# else

ACL_IFCONF *acl_get_ifaddrs(void)
{
	ACL_IFCONF *ifconf;
	struct ifreq *ifaces;
	int ifaces_size = 8 * sizeof(struct ifreq);
	struct ifconf param;
	int sock, i, j;

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock <= 0) {
		acl_msg_error("%s(%d): create socket error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return NULL;
	}

	ifaces = acl_mymalloc(ifaces_size);
	for (;;) {
		param.ifc_len = ifaces_size;
		param.ifc_req = ifaces;
		if (ioctl(sock, SIOCGIFCONF, &param)) {
			acl_msg_error("%s(%d): ioctl error(%s)",
				__FUNCTION__, __LINE__, acl_last_serror());
			close(sock);
			acl_myfree(ifaces);
			return NULL;
		}
		if (param.ifc_len < ifaces_size) {
			break;
		}
		acl_myfree(ifaces);
		ifaces_size *= 2;
		ifaces = acl_mymalloc(ifaces_size);
	}

	close(sock);

	ifconf = ifconf_create((int) (param.ifc_len / sizeof(struct ifreq)));
	for (i = 0, j = 0; i < ifconf->length; i++) {
		ACL_SOCKADDR *saddr = (ACL_SOCKADDR *) &ifaces[i].ifr_addr;
		ACL_IFADDR   *ifa   = (ACL_IFADDR *) &ifconf->addrs[j];
		size_t size;

		size = acl_inet_ntop(&saddr->sa, ifa->addr, sizeof(ifa->addr));
		if (size == 0) {
			continue;
		}

		SAFE_COPY(ifconf->addrs[j].name, ifaces[i].ifr_name);
		memcpy(&ifconf->addrs[j].saddr, saddr, size);

		j++;
	}

	if (j == 0) {
		acl_myfree(ifconf->addrs);
		acl_myfree(ifconf);
		return NULL;
	}

	ifconf->length = j;  /* reset the ifconf->length */
	acl_myfree(ifaces);
	return ifconf;
}
# endif

#elif defined(ACL_WINDOWS)

#pragma comment (lib, "Iphlpapi.lib")
#ifdef	MS_VC6

#include "iptypes.h"
#include "Ipifcons.h"

typedef HRESULT STDAPICALLTYPE PGAINFO(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);

ACL_IFCONF *acl_get_ifaddrs(void)
{
	IP_ADAPTER_INFO info_temp, *infos, *info;
	ACL_IFCONF *ifconf;
	ULONG len = 0;
	int j;
	HMODULE hInst;
	PGAINFO *pGAInfo;

	hInst = LoadLibrary("iphlpapi.dll");
	if(!hInst) {
		MessageBox(0, "iphlpapi.dll not supported!", "Error", 0);
		return NULL;
	}

	pGAInfo = (PGAINFO*) GetProcAddress(hInst,"GetAdaptersInfo");
	if (pGAInfo == NULL) {
		MessageBox(0, "can't find GetAdaptersInfo!", "Error", 0);
		return NULL;
	}

	if (pGAInfo(&info_temp, &len) != ERROR_BUFFER_OVERFLOW) {
		MessageBox(0, "GetAdaptersInfo error", 0);
		FreeLibrary(hInst);
		return NULL;
	}

	infos = (IP_ADAPTER_INFO *) acl_mymalloc(len);
	if (pGAInfo(infos, &len) != NO_ERROR) {
		MessageBox(0, "GetAdaptersInfo eror", 0);
		acl_myfree(infos);
		FreeLibrary(hInst);
		return NULL;
	}

	ifconf = ifconf_create((int) (len / sizeof(IP_ADAPTER_INFO) + 1));

	for (info = infos, j = 0; info != NULL; info = info->Next) {
		if (info->Type == MIB_IF_TYPE_LOOPBACK)
			continue;
		if (strcmp(info->IpAddressList.IpAddress.String, "0.0.0.0") == 0)
			continue;
		if (!acl_is_ip(info->IpAddressList.IpAddress.String))
			continue;

		SAFE_COPY(ifconf->addrs[j].name, info->AdapterName);
		SAFE_COPY(ifconf->addrs[j].desc, info->Description);
		SAFE_COPY(ifconf->addrs[j].addr,
			info->IpAddressList.IpAddress.String);

		ifconf->addrs[j].saddr.in.sin_addr.s_addr
			= inet_addr(ifconf->addrs[j].addr);
		j++;
		if (j == ifconf->length) {
			ifconf->length *= 2;
			ifconf->addrs = (ACL_IFADDR*) acl_myrealloc(ifconf->addrs,
					ifconf->length * sizeof(ACL_IFADDR));
		}
	}

	acl_myfree(infos);

	if (j == 0) {
		acl_myfree(ifconf->addrs);
		acl_myfree(ifconf);
		FreeLibrary(hInst);
		return NULL;
	}

	ifconf->length = j;  /* reset the ifconf->length */
	FreeLibrary(hInst);
	return ifconf;
}

#else  /* MS_VC6  */

#include <Iphlpapi.h>

ACL_IFCONF *acl_get_ifaddrs()
{
	IP_ADAPTER_INFO info_temp, *infos, *info;
	ACL_IFCONF *ifconf;
	ULONG len = 0;
	int   j;

	if (GetAdaptersInfo(&info_temp, &len) != ERROR_BUFFER_OVERFLOW) {
		acl_msg_error("%s(%d): GetAdaptersInfo eror(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		return NULL;
	}

	infos = (IP_ADAPTER_INFO *) acl_mymalloc(len);
	if (GetAdaptersInfo(infos, &len) != NO_ERROR) {
		acl_msg_error("%s(%d): GetAdaptersInfo eror(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
		acl_myfree(infos);
		return NULL;
	}

	ifconf = ifconf_create((int) (len / sizeof(IP_ADAPTER_INFO) + 1));

	for (info = infos, j = 0; info != NULL; info = info->Next) {
		if (info->Type == MIB_IF_TYPE_LOOPBACK)
			continue;
		if (!strcmp(info->IpAddressList.IpAddress.String, "0.0.0.0"))
			continue;
		if (!acl_is_ip(info->IpAddressList.IpAddress.String))
			continue;

		SAFE_COPY(ifconf->addrs[j].name, info->AdapterName);
		SAFE_COPY(ifconf->addrs[j].desc, info->Description);
		SAFE_COPY(ifconf->addrs[j].addr,
			info->IpAddressList.IpAddress.String);

		ifconf->addrs[j].saddr.in.sin_addr.s_addr
			= inet_addr(ifconf->addrs[j].addr);
		j++;
		if (j == ifconf->length) {
			ifconf->length *= 2;
			ifconf->addrs = (ACL_IFADDR*) acl_myrealloc(
				ifconf->addrs, ifconf->length * sizeof(ACL_IFADDR));
		}
	}

	acl_myfree(infos);

	if (j == 0) {
		acl_myfree(ifconf->addrs);
		acl_myfree(ifconf);
		return NULL;
	}

	ifconf->length = j;  /* reset the ifconf->length */
	return ifconf;
}
#endif  /* !MS_VC6 */
#else
# error "unknow OS"
#endif

void acl_free_ifaddrs(ACL_IFCONF *ifconf)
{
	if (ifconf == NULL)
		return;

	acl_myfree(ifconf->addrs);
	acl_myfree(ifconf);
}

static int match_ipv4(const char *pattern, const char *ip)
{
	/* format: xxx.xxx.xxx.* or xxx.xxx.*.* or xxx.*.*.* */
	ACL_ARGV *pattern_tokens;
	ACL_ARGV *ip_tokens;
	ACL_ITER  iter;
	int       i = 0;

#define EQ !strcmp
#define IPV4_MATCH(p) (EQ((p), "*") || EQ((p), "*.*.*.*") || EQ((p), "0.0.0.0"))
	/* for "*" or "*.*.*.*" or "0.0.0.0" */

	if (IPV4_MATCH(pattern)) {
		return 1;
	}

	pattern_tokens = acl_argv_split(pattern, ".");
	ip_tokens      = acl_argv_split(ip, ".");

	if (pattern_tokens->argc != 4) {
		acl_argv_free(pattern_tokens);
		acl_argv_free(ip_tokens);

		return 0;
	}

	if (ip_tokens->argc != 4) {
		acl_argv_free(pattern_tokens);
		acl_argv_free(ip_tokens);
		return 0;
	}

	acl_foreach(iter, ip_tokens) {
		const char *ptr = (const char *) iter.data;
		const char *arg = pattern_tokens->argv[i];
		if (strcmp(arg, "*") != 0 && strcmp(arg, ptr) != 0) {
			acl_argv_free(pattern_tokens);
			acl_argv_free(ip_tokens);
			return 0;
		}
		i++;
	}

	acl_argv_free(pattern_tokens);
	acl_argv_free(ip_tokens);
	return 1;
}

/* for ":port" or "|port" */
#define MATCH1(p) ((*(p) == ':' || *(p) == ACL_ADDR_SEP))

/* for "*:port" or "*#port" */
#define MATCH2(p) (*(p) == '*' && (*((p) + 1) == ':' || *((p) + 1) == ACL_ADDR_SEP))

/**
 * pattern for ipv4:
 *   *|port, *:port, |port, :port, *.*.*.*:port, 0.0.0.0:port, xxx.xxx.xxx.xxx:port,
 *   *, *.*.*.*, 0.0.0.0, xxx.xxx.xxx.xxx
 */
static ACL_IFADDR *ipv4_clone(const char *pattern, const ACL_IFADDR *ifaddr)
{
	char  buf[256], *ptr;
	ACL_IFADDR *addr;
	int   port = -1;

	/* for "port" */
	if (acl_alldig(pattern)) {
		if ((port = atoi(pattern)) < 0)
			return NULL;

		addr = ifaddr_clone(ifaddr, port);
		return addr;
	}

	ACL_SAFE_STRNCPY(buf, pattern, sizeof(buf));
	if ((ptr = strrchr(buf, ACL_ADDR_SEP)) || (ptr = strrchr(buf, ':')))
		*ptr++ = 0;
	else
		ptr = NULL;

	if (MATCH1(pattern) || MATCH2(pattern)) {
		if (ptr && acl_alldig(ptr))
			port = atoi(ptr);
		addr = ifaddr_clone(ifaddr, port);
		return addr;
	}

	if (!match_ipv4(buf, ifaddr->addr))
		return NULL;

	if (ptr && acl_alldig(ptr))
		port = atoi(ptr);
	addr = ifaddr_clone(ifaddr, port);
	return addr;
}

/**
 * pattern for ipv6:
 *   *|port, |port, *, port
 */
#ifdef AF_INET6
static ACL_IFADDR *ipv6_clone(const char *pattern, const ACL_IFADDR *ifaddr)
{
	char  buf[256], *ptr;
	ACL_IFADDR *addr;
	int   port = -1;

	/* for "port" */
	if (acl_alldig(pattern)) {
		if ((port = atoi(pattern)) <= 0)
			return NULL;

		addr = ifaddr_clone(ifaddr, port);
		return addr;
	}

	ACL_SAFE_STRNCPY(buf, pattern, sizeof(buf));
	if ((ptr = strrchr(buf, ACL_ADDR_SEP)))
		*ptr++ = 0;
	else
		ptr = NULL;

	if (MATCH1(pattern) || MATCH2(pattern)) {
		if (ptr && acl_alldig(ptr))
			port = atoi(ptr);
		addr = ifaddr_clone(ifaddr, port);
		return addr;
	}

	if (!acl_valid_ipv6_hostaddr(buf, 0))
		return NULL;
	if (!EQ(buf, ifaddr->addr))
		return NULL;

	if (ptr && acl_alldig(ptr))
		port = atoi(ptr);
	addr = ifaddr_clone(ifaddr, port);
	return addr;
}
#endif

static void patterns_addrs_add(ACL_ARGV *patterns,
	const ACL_IFADDR *ifaddr, ACL_ARRAY *addrs)
{
	ACL_ITER iter;
	ACL_IFADDR *addr;

	acl_foreach(iter, patterns) {
		const char  *pattern      = (const char *) iter.data;
		const struct sockaddr *sa = &ifaddr->saddr.sa;

		if (sa->sa_family == AF_INET) {
			addr = ipv4_clone(pattern, ifaddr);
			if (addr)
				acl_array_append(addrs, addr);
		}
#ifdef AF_INET6
		else if (sa->sa_family == AF_INET6) {
			addr = ipv6_clone(pattern, ifaddr);
			if (addr)
				acl_array_append(addrs, addr);
		}
#endif
	}
}

ACL_IFCONF *acl_ifconf_search(const char *patterns)
{
	ACL_IFCONF *ifconf = acl_get_ifaddrs(), *ifconf2;
	ACL_ARGV   *patterns_tokens;
	ACL_HTABLE *table;
	ACL_ARRAY  *addrs;
	ACL_ITER    iter;

	if (ifconf == NULL) {
		acl_msg_error("%s(%d), %s:  acl_get_ifaddrs error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return NULL;
	}

	addrs = acl_array_create(10);

	patterns_tokens = acl_argv_split(patterns, "\"',; \t\r\n");

	acl_foreach(iter, ifconf) {
		const ACL_IFADDR *ifaddr = (const ACL_IFADDR *) iter.data;
		patterns_addrs_add(patterns_tokens, ifaddr, addrs);
	}

#ifdef ACL_UNIX
	/* just for all unix domain path */
	acl_foreach(iter, patterns_tokens) {
		const char *pattern = (const char *) iter.data;
		ACL_IFADDR *ifaddr;

		if (acl_valid_hostaddr(pattern, 0))
			continue;

		/* the left are used as UNIX doamin except for IP addr */
		ifaddr = (ACL_IFADDR *) acl_mycalloc(1, sizeof(ACL_IFADDR));

		SAFE_COPY(ifaddr->name, pattern);
		SAFE_COPY(ifaddr->addr, pattern);
		ifaddr->saddr.sa.sa_family = AF_UNIX;
		SAFE_COPY(ifaddr->saddr.un.sun_path, pattern);
		acl_array_append(addrs, ifaddr);
	}
#endif

	acl_argv_free(patterns_tokens);
	acl_free_ifaddrs(ifconf);

	if (acl_array_size(addrs) <= 0) {
		acl_array_free(addrs, acl_myfree_fn);
		return NULL;
	}

	table   = acl_htable_create(10, 0);
	ifconf2 = ifconf_create(acl_array_size(addrs));

	/* reset the length to 0 for adding unique real addr */
	ifconf2->length = 0;

	acl_foreach(iter, addrs) {
		const ACL_IFADDR *ifaddr = (const ACL_IFADDR *) iter.data;
		if (acl_htable_find(table, ifaddr->addr) == NULL) {
			ifaddr_copy(&ifconf2->addrs[ifconf2->length], ifaddr);
			ifconf2->length++;
			acl_htable_enter(table, ifaddr->addr, NULL);
		}
	}

	acl_htable_free(table, NULL);
	acl_array_free(addrs, acl_myfree_fn);
	return ifconf2;
}
