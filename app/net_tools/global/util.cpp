#include "StdAfx.h"
#include <IPTypes.h>
#include <Iphlpapi.h>
#include "util.h"
#pragma comment(lib, "Iphlpapi.lib")

#ifndef VC6
typedef  FIXED_INFO_W2KSP1 FIXED_INFO;
#endif

static size_t collect(const FIXED_INFO *fi, std::vector<acl::string>& dns_list)
{
	const IP_ADDR_STRING* next = &fi->DnsServerList;
	do 
	{
		dns_list.push_back(next->IpAddress.String);
		next = next->Next;
	} while (next != NULL);

	return dns_list.size();
}

size_t util::get_dns(std::vector<acl::string>& dns_list)
{
	FIXED_INFO *fi;
	ULONG n;

	n = sizeof(FIXED_INFO);
	fi= (FIXED_INFO*) acl_mycalloc(1, n) ;

#if (NTDDI_VERSION >= NTDDI_WIN2KSP1)
	DWORD ret = ::GetNetworkParams(fi, &n);
#endif
	if (ret == ERROR_SUCCESS)
	{
		size_t n1 = collect(fi, dns_list);
		acl_myfree(fi);
		return n1;
	}
	else if (ret != ERROR_BUFFER_OVERFLOW)
	{
		acl_myfree(fi);
		return 0;
	}

	acl_myfree(fi);
	fi= (FIXED_INFO*) acl_mycalloc(1, n);

	ret = ::GetNetworkParams(fi, &n);
	if (ret != ERROR_SUCCESS)
	{
		acl_myfree(fi);
		return 0;
	}

	size_t n2 = collect(fi, dns_list);
	acl_myfree(fi);
	return n2;
}

double util::stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}
