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

static const ACL_IFADDR *iter_head(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->dlen = -1;
	iter->key  = NULL;
	iter->klen = -1;

	iter->i = 0;
	iter->size = ifconf->length;
	iter->ptr  = iter->data = &ifconf->addrs[0];
	return (iter->ptr);
}

static const ACL_IFADDR *iter_next(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->i++;
	if (iter->i >= ifconf->length)
		iter->data = iter->ptr = NULL;
	else
		iter->data = iter->ptr = &ifconf->addrs[iter->i];
	return (iter->ptr);
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
	return (iter->ptr);
}

static const ACL_IFADDR *iter_prev(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->i--;
	if (iter->i < 0)
		iter->data = iter->ptr = NULL;
	else
		iter->data = iter->ptr = &ifconf->addrs[iter->i];
	return (iter->ptr);
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

ACL_IFCONF *acl_get_ifaddrs()
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
		return (NULL);
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
			return (NULL);
		}
		if (param.ifc_len < ifaces_size)
			break;
		acl_myfree(ifaces);
		ifaces_size *= 2;
		ifaces = acl_mymalloc(ifaces_size);
	}

	close(sock);

	ifconf = (ACL_IFCONF*) acl_mymalloc(sizeof(ACL_IFCONF));
	ifconf->length = param.ifc_len / sizeof(struct ifreq);
	ifconf->addrs = (ACL_IFADDR*)
		acl_mycalloc(ifconf->length, sizeof(ACL_IFADDR));

	for (i = 0, j = 0; i < ifconf->length; i++) {
		ACL_SOCKADDR *saddr = (ACL_SOCKADDR *) &ifaces[i].ifr_addr;
		ACL_IFADDR   *ifa   = (ACL_IFADDR *) &ifconf->addrs[j];
		size_t size;

		size = acl_inet_ntop(&saddr->sa, ifa->ip, sizeof(ifa->ip));
		if (size == 0)
			continue;

		ifconf->addrs[j].name = acl_mystrdup(ifaces[i].ifr_name);
		memcpy(&ifconf->addrs[j].saddr, saddr, size);

		j++;
	}

	if (j == 0) {
		acl_myfree(ifconf->addrs);
		acl_myfree(ifconf);
		return NULL;
	}

	ifconf->length = j;  /* reset the ifconf->length */

	/* set the iterator callback */
	ifconf->iter_head = iter_head;
	ifconf->iter_next = iter_next;
	ifconf->iter_tail = iter_tail;
	ifconf->iter_prev = iter_prev;

	acl_myfree(ifaces);
	return ifconf;
}

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

	ifconf = (ACL_IFCONF*) acl_mymalloc(sizeof(ACL_IFCONF));
	ifconf->length = len / sizeof(IP_ADAPTER_INFO) + 1;
	ifconf->addrs = (ACL_IFADDR*)
		acl_mycalloc(ifconf->length, sizeof(ACL_IFADDR));

	for (info = infos, j = 0; info != NULL; info = info->Next) {
		if (info->Type == MIB_IF_TYPE_LOOPBACK)
			continue;
		if (strcmp(info->IpAddressList.IpAddress.String, "0.0.0.0") == 0)
			continue;
		if (!acl_is_ip(info->IpAddressList.IpAddress.String))
			continue;

		ifconf->addrs[j].name = acl_mystrdup(info->AdapterName);
		ifconf->addrs[j].desc = acl_mystrdup(info->Description);
		snprintf(ifconf->addrs[j].ip, sizeof(ifconf->addrs[j].ip),
				"%s", info->IpAddressList.IpAddress.String);
		ifconf->addrs[j].saddr.in.sin_addr.s_addr
			= inet_addr(ifconf->addrs[j].ip);
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
		return (NULL);
	}

	ifconf->length = j;  /* reset the ifconf->length */

	/* set the iterator callback */
	ifconf->iter_head = iter_head;
	ifconf->iter_next = iter_next;
	ifconf->iter_tail = iter_tail;
	ifconf->iter_prev = iter_prev;

	FreeLibrary(hInst);
	return (ifconf);
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

	ifconf = (ACL_IFCONF*) acl_mymalloc(sizeof(ACL_IFCONF));
	ifconf->length = len / sizeof(IP_ADAPTER_INFO) + 1;
	ifconf->addrs = (ACL_IFADDR*)
		acl_mycalloc(ifconf->length, sizeof(ACL_IFADDR));

	for (info = infos, j = 0; info != NULL; info = info->Next) {
		if (info->Type == MIB_IF_TYPE_LOOPBACK)
			continue;
		if (!strcmp(info->IpAddressList.IpAddress.String, "0.0.0.0"))
			continue;
		if (!acl_is_ip(info->IpAddressList.IpAddress.String))
			continue;

		ifconf->addrs[j].name = acl_mystrdup(info->AdapterName);
		ifconf->addrs[j].desc = acl_mystrdup(info->Description);
		snprintf(ifconf->addrs[j].ip, sizeof(ifconf->addrs[j].ip),
			"%s", info->IpAddressList.IpAddress.String);
		ifconf->addrs[j].saddr.in.sin_addr.s_addr
			= inet_addr(ifconf->addrs[j].ip);
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
		return (NULL);
	}

	ifconf->length = j;  /* reset the ifconf->length */

	/* set the iterator callback */
	ifconf->iter_head = iter_head;
	ifconf->iter_next = iter_next;
	ifconf->iter_tail = iter_tail;
	ifconf->iter_prev = iter_prev;

	return (ifconf);
}
#endif  /* !MS_VC6 */
#else
# error "unknow OS"
#endif

void acl_free_ifaddrs(ACL_IFCONF *ifconf)
{
	int   i;

	if (ifconf == NULL)
		return;
	for (i = 0; i < ifconf->length; i++) {
#ifdef ACL_WINDOWS
		if (ifconf->addrs[i].desc != NULL)
			acl_myfree(ifconf->addrs[i].desc);
#endif
		if (ifconf->addrs[i].name != NULL)
			acl_myfree(ifconf->addrs[i].name);
	}

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
		acl_msg_warn("%s(%d), %s: invalid pattern: %s",
			__FILE__, __LINE__, __FUNCTION__, pattern);
		acl_argv_free(pattern_tokens);
		acl_argv_free(ip_tokens);

		return 0;
	}

	if (ip_tokens->argc != 4) {
		acl_msg_warn("%s(%d), %s: invalid ip: %s",
			__FILE__, __LINE__, __FUNCTION__, ip);
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

/* for ":port" or "#port" */
#define MATCH1(p) ((*(p) == ':' || *(p) == '#'))

/* for "*:port" or "*#port" */
#define MATCH2(p) (*(p) == '*' && (*((p) + 1) == ':' || *((p) + 1) == '#'))

/**
 * pattern for ipv4:
 *   *#port, *:port, #port, :port, *.*.*.*:port, 0.0.0.0:port, xxx.xxx.xxx.xxx:port,
 *   *, *.*.*.*, 0.0.0.0, xxx.xxx.xxx.xxx
 */
static int ipv4_pattern_match_add(const char *pattern,
	const ACL_IFADDR *ifaddr, ACL_ARGV *addrs)
{
	char  buf[256], *ptr, addr[256];

	/* for "port" */
	if (acl_alldig(pattern)) {
		snprintf(addr, sizeof(addr), "%s:%s", ifaddr->ip, pattern);
		acl_argv_add(addrs, addr, NULL);
		return 1;
	}

	ACL_SAFE_STRNCPY(buf, pattern, sizeof(buf));
	if ((ptr = strrchr(buf, ACL_ADDR_SEP)) || (ptr = strrchr(buf, ':')))
		*ptr++ = 0;
	else
		ptr = NULL;

	if (MATCH1(pattern) || MATCH2(pattern)) {
		if (ptr && *ptr && acl_alldig(ptr))
			snprintf(addr, sizeof(addr), "%s:%s", ifaddr->ip, ptr);
		else
			snprintf(addr, sizeof(addr), "%s", ifaddr->ip);
		acl_argv_add(addrs, addr, NULL);
		return 1;
	}

	if (!match_ipv4(buf, ifaddr->ip))
		return 0;

	if (ptr && *ptr) {
		snprintf(addr, sizeof(addr), "%s:%s", ifaddr->ip, ptr);
		acl_argv_add(addrs, addr, NULL);
	} else
		acl_argv_add(addrs, ifaddr->ip, NULL);

	return 1;
}

/**
 * pattern for ipv6:
 *   *#port, #port, *, port
 */
#ifdef AF_INET6
static int ipv6_pattern_match_add(const char *pattern,
	const ACL_IFADDR *ifaddr, ACL_ARGV *addrs)
{
	char  buf[256], *ptr, addr[256];

	/* for "port" */
	if (acl_alldig(pattern)) {
		snprintf(addr, sizeof(addr), "%s:%s", ifaddr->ip, pattern);
		acl_argv_add(addrs, addr, NULL);
		return 1;
	}

	if (MATCH1(pattern) || MATCH2(pattern)) {
		snprintf(addr, sizeof(addr), "%s%s", ifaddr->ip, pattern);
		acl_argv_add(addrs, addr, NULL);
		return 1;
	}

	ACL_SAFE_STRNCPY(buf, pattern, sizeof(buf));
	if ((ptr = strrchr(buf, ACL_ADDR_SEP)) || (ptr = strrchr(buf, ':')))
		*ptr++ = 0;
	else
		ptr = NULL;

	if (!acl_valid_ipv6_hostaddr(buf, 0))
		return 0;

	if (ptr && *ptr) {
		snprintf(addr, sizeof(addr), "%s:%s", ifaddr->ip, pattern);
		acl_argv_add(addrs, addr, NULL);
	} else
		acl_argv_add(addrs, ifaddr->ip, NULL);

	return 1;
}
#endif

static void patterns_match_add(ACL_ARGV *patterns,
	const ACL_IFADDR *ifaddr, ACL_ARGV *addrs)
{
	ACL_ITER iter;

	acl_foreach(iter, patterns) {
		const char  *pattern      = (const char *) iter.data;
		const struct sockaddr *sa = &ifaddr->saddr.sa;

		if (sa->sa_family == AF_INET) {
			ipv4_pattern_match_add(pattern, ifaddr, addrs);
		}
#ifdef AF_INET6
		else if (sa->sa_family == AF_INET6) {
			ipv6_pattern_match_add(pattern, ifaddr, addrs);
		}
#endif
	}
}

ACL_ARGV *acl_ifconf_search(const char *patterns)
{
	ACL_IFCONF *ifconf = acl_get_ifaddrs();
	ACL_ARGV *patterns_tokens, *addrs;
	ACL_ITER  iter;

	if (ifconf == NULL) {
		acl_msg_error("%s(%d), %s:  acl_get_ifaddrs error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return NULL;
	}

	addrs = acl_argv_alloc(1);

	patterns_tokens = acl_argv_split(patterns, "\"',; \t\r\n");

	acl_foreach(iter, ifconf) {
		const ACL_IFADDR *ifaddr = (const ACL_IFADDR *) iter.data;
		patterns_match_add(patterns_tokens, ifaddr, addrs);
	}

#ifdef ACL_UNIX
	/* just for all unix domain path */
	acl_foreach(iter, patterns_tokens) {
		const char *pattern = (const char *) iter.data;
		if (*pattern == '/' || (*pattern == '.' && *pattern == '/'))
			acl_argv_add(addrs, pattern, NULL);
	}
#endif

	acl_argv_free(patterns_tokens);
	acl_free_ifaddrs(ifconf);

	return addrs;
}
