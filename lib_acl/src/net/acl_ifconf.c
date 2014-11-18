#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "net/acl_sane_inet.h"
#include "net/acl_ifconf.h"

#endif

static const ACL_IFADDR *ifaddrs_iter_head(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;

	iter->i = 0;
	iter->size = ifconf->length;
	iter->ptr = iter->data = &ifconf->addrs[0];
	return (iter->ptr);
}

static const ACL_IFADDR *ifaddrs_iter_next(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->i++;
	if (iter->i >= ifconf->length)
		iter->data = iter->ptr = NULL;
	else
		iter->data = iter->ptr = &ifconf->addrs[iter->i];
	return (iter->ptr);
}

static const ACL_IFADDR *ifaddrs_iter_tail(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
{
	iter->dlen = -1;
	iter->key = NULL;
	iter->klen = -1;

	iter->i = ifconf->length - 1;
	iter->size = ifconf->length;
	if (iter->i < 0)
		iter->data = iter->ptr = NULL;
	else
		iter->ptr = iter->data = &ifconf->addrs[iter->i];
	return (iter->ptr);
}

static const ACL_IFADDR *ifaddrs_iter_prev(ACL_ITER *iter, struct ACL_IFCONF *ifconf)
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
	const char *myname = "acl_get_ifaddrs";
	ACL_IFCONF *ifconf;
	struct ifreq *ifaces;
	int ifaces_size = 8 * sizeof(struct ifreq);
	struct ifconf param;
	int   sock;
	int   i, j;

	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock <= 0) {
		acl_msg_error("%s(%d): create socket error(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}

	ifaces = acl_mymalloc(ifaces_size);
	for (;;) {
		param.ifc_len = ifaces_size;
		param.ifc_req = ifaces;
		if (ioctl(sock, SIOCGIFCONF, &param)) {
			acl_msg_error("%s(%d): ioctl error(%s)",
				myname, __LINE__, acl_last_serror());
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
		if (ifaces[i].ifr_addr.sa_family != AF_INET
#ifdef ACL_MACOSX
		   && ifaces[i].ifr_addr.sa_family != AF_LINK
#endif
		)
			continue;
		ifconf->addrs[j].name = acl_mystrdup(ifaces[i].ifr_name);
		ifconf->addrs[j].addr = ((struct sockaddr_in *)
				&(ifaces[i].ifr_addr))->sin_addr.s_addr;
		acl_inet_ntoa(((struct sockaddr_in *) &ifaces[i].ifr_addr)->sin_addr,
				ifconf->addrs[j].ip, sizeof(ifconf->addrs[j].ip));
		j++;
	}

	if (j == 0) {
		acl_myfree(ifconf->addrs);
		acl_myfree(ifconf);
		return (NULL);
	}

	ifconf->length = j;  /* reset the ifconf->length */

	/* set the iterator callback */
	ifconf->iter_head = ifaddrs_iter_head;
	ifconf->iter_next = ifaddrs_iter_next;
	ifconf->iter_tail = ifaddrs_iter_tail;
	ifconf->iter_prev = ifaddrs_iter_prev;

	acl_myfree(ifaces);
	return (ifconf);
}

#elif defined(WIN32)

#ifdef	MS_VC6

#include "iptypes.h"
#include "Ipifcons.h"

typedef HRESULT STDAPICALLTYPE PGAINFO(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);

ACL_IFCONF *acl_get_ifaddrs()
{
	const char *myname = "acl_get_ifaddrs";
	IP_ADAPTER_INFO info_temp, *infos, *info;
	ACL_IFCONF *ifconf;
	ULONG len = 0;
	int   j;
	HMODULE hInst;
	PGAINFO *pGAInfo;

	hInst = LoadLibrary("iphlpapi.dll");
	if(!hInst) {
		MessageBox(0, "iphlpapi.dll not supported in this platform!", "Error", 0);
		return (NULL);
	}

	pGAInfo = (PGAINFO*) GetProcAddress(hInst,"GetAdaptersInfo");
	if (pGAInfo == NULL) {
		MessageBox(0, "can't find GetAdaptersInfo function!", "Error", 0);
		return (NULL);
	}

	if (pGAInfo(&info_temp, &len) != ERROR_BUFFER_OVERFLOW) {
		acl_msg_error("%s(%d): GetAdaptersInfo eror(%s)",
			myname, __LINE__, acl_last_serror());
		FreeLibrary(hInst);
		return (NULL);
	}

	infos = (IP_ADAPTER_INFO *) acl_mymalloc(len);
	if (pGAInfo(infos, &len) != NO_ERROR) {
		acl_msg_error("%s(%d): GetAdaptersInfo eror(%s)",
			myname, __LINE__, acl_last_serror());
		acl_myfree(infos);
		FreeLibrary(hInst);
		return (NULL);
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
		if (acl_is_ip(info->IpAddressList.IpAddress.String) < 0)
			continue;

		ifconf->addrs[j].name = acl_mystrdup(info->AdapterName);
		ifconf->addrs[j].desc = acl_mystrdup(info->Description);
		snprintf(ifconf->addrs[j].ip, sizeof(ifconf->addrs[j].ip),
				"%s", info->IpAddressList.IpAddress.String);
		ifconf->addrs[j].addr = inet_addr(ifconf->addrs[j].ip);
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
	ifconf->iter_head = ifaddrs_iter_head;
	ifconf->iter_next = ifaddrs_iter_next;
	ifconf->iter_tail = ifaddrs_iter_tail;
	ifconf->iter_prev = ifaddrs_iter_prev;

	FreeLibrary(hInst);
	return (ifconf);
}

#else  /* MS_VC6  */

#include <Iphlpapi.h>

ACL_IFCONF *acl_get_ifaddrs()
{
	const char *myname = "acl_get_ifaddrs";
	IP_ADAPTER_INFO info_temp, *infos, *info;
	ACL_IFCONF *ifconf;
	ULONG len = 0;
	int   j;

	if (GetAdaptersInfo(&info_temp, &len) != ERROR_BUFFER_OVERFLOW) {
		acl_msg_error("%s(%d): GetAdaptersInfo eror(%s)",
			myname, __LINE__, acl_last_serror());
		return (NULL);
	}

	infos = (IP_ADAPTER_INFO *) acl_mymalloc(len);
	if (GetAdaptersInfo(infos, &len) != NO_ERROR) {
		acl_msg_error("%s(%d): GetAdaptersInfo eror(%s)",
			myname, __LINE__, acl_last_serror());
		acl_myfree(infos);
		return (NULL);
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
		if (acl_is_ip(info->IpAddressList.IpAddress.String) < 0)
			continue;

		ifconf->addrs[j].name = acl_mystrdup(info->AdapterName);
		ifconf->addrs[j].desc = acl_mystrdup(info->Description);
		snprintf(ifconf->addrs[j].ip, sizeof(ifconf->addrs[j].ip),
				"%s", info->IpAddressList.IpAddress.String);
		ifconf->addrs[j].addr = inet_addr(ifconf->addrs[j].ip);
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
		return (NULL);
	}

	ifconf->length = j;  /* reset the ifconf->length */

	/* set the iterator callback */
	ifconf->iter_head = ifaddrs_iter_head;
	ifconf->iter_next = ifaddrs_iter_next;
	ifconf->iter_tail = ifaddrs_iter_tail;
	ifconf->iter_prev = ifaddrs_iter_prev;

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
#ifdef WIN32
		if (ifconf->addrs[i].desc != NULL)
			acl_myfree(ifconf->addrs[i].desc);
#endif
		if (ifconf->addrs[i].name != NULL)
			acl_myfree(ifconf->addrs[i].name);
	}

	acl_myfree(ifconf->addrs);
	acl_myfree(ifconf);
}
