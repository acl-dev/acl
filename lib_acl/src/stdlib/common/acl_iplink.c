#include "StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef ACL_UNIX
#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_dlink.h"
#include "stdlib/acl_iplink.h"

#endif

static int _check_ip_addr(const char *pstrip)
{
	const	char	*ptr;
	int	count = 0;

	if (pstrip == NULL || *pstrip == 0)
		return(-1);

	ptr = pstrip;
	if(*ptr == '.')		/* the first char should not be '.' */
		return(-1);
	while(*ptr) {
		if (*ptr == '.') {
			if (!isdigit((int)*(ptr + 1)))
				return(-1);
			count++;
		} else if (!isdigit((int)*ptr))
			return(-1);
		ptr++;
	}
	if(*(ptr - 1) == '.')	/* the last char should not be '.' */
		return(-1);
	if(count != 3)		/* 192.168.0.1 has the number '.' is 4 */
		return(-1);

	return(0);
}

ACL_IPLINK *acl_iplink_create(int nsize)
{
	return acl_dlink_create(nsize);
}

void acl_iplink_free(ACL_IPLINK *lnk)
{
	if (lnk)
		acl_dlink_free(lnk);
}

ACL_IPITEM *acl_iplink_lookup_item(const ACL_IPLINK *plink,
	ACL_IPITEM *pitem)
{
	return acl_dlink_lookup_by_item(plink, pitem);
}

ACL_IPITEM *acl_iplink_lookup_bin(const ACL_IPLINK *plink, unsigned int ip)
{
	return acl_dlink_lookup(plink, ip);
}

ACL_IPITEM *acl_iplink_lookup_str(const ACL_IPLINK *plink, const char *ipstr)
{
	unsigned int ip;

	ip = (unsigned int) ntohl(inet_addr(ipstr));
	return acl_iplink_lookup_bin(plink, ip);
}

ACL_IPITEM *acl_iplink_insert_bin(ACL_IPLINK *plink,
	unsigned int ip_begin, unsigned int ip_end)
{
	return acl_dlink_insert(plink, ip_begin, ip_end);
}

ACL_IPITEM *acl_iplink_insert(ACL_IPLINK *plink,
	const char *pstrip_begin, const char *pstrip_end )
{
	const char *myname = "acl_iplink_insert";
	unsigned int ip_begin, ip_end;

	if (_check_ip_addr(pstrip_begin) < 0) {
		acl_msg_error("%s: invalid ip begin(%s)", myname, pstrip_begin);
		return NULL;
	}
	if (_check_ip_addr(pstrip_end) < 0) {
		acl_msg_error("%s: invalid ip end(%s)", myname, pstrip_end);
		return NULL;
	}

	ip_begin = (unsigned int) ntohl(inet_addr(pstrip_begin));
	ip_end   = (unsigned int) ntohl(inet_addr(pstrip_end));

	return acl_iplink_insert_bin(plink, ip_begin, ip_end);
}

int acl_iplink_delete_by_ip(ACL_IPLINK *plink, const char *pstrip_begin)
{
	unsigned int ip;

	if (_check_ip_addr(pstrip_begin) < 0)
		return -1;

	ip = (unsigned int) ntohl(inet_addr(pstrip_begin));
	return acl_dlink_delete(plink, ip);
}

int acl_iplink_delete_by_item(ACL_IPLINK *plink, ACL_IPITEM *pitem)
{
	return acl_dlink_delete_by_item(plink, pitem);
}

ACL_IPITEM *acl_iplink_modify(ACL_IPLINK *plink, const char *pstrip_id,
	const char *pstrip_begin, const char *pstrip_end)
{
	unsigned int ip_begin, ip_end;

	if (_check_ip_addr(pstrip_id) < 0
	    || _check_ip_addr(pstrip_begin) < 0
	    || _check_ip_addr(pstrip_end) < 0)
		return NULL;

	ip_begin = (unsigned int) ntohl(inet_addr(pstrip_begin));
	ip_end   = (unsigned int) ntohl(inet_addr(pstrip_end));

	return acl_dlink_modify(plink, ip_begin, ip_end);
}

int acl_iplink_count_item(ACL_IPLINK *plink)
{
	return acl_array_size(plink->parray);
}

/* ++++++++++++++++++++++++++below functions are used only for test ++++++++++++ */
static char *__sane_inet_ntoa(unsigned int src, char *dst, size_t size)
{
	unsigned char *bytes;
	struct in_addr in;

	if (size < 16)
		return(NULL);

	/* XXX: may be bugfix here---zsx */
/*	in.s_addr = ntohl(src); */
	in.s_addr = src;
	bytes = (unsigned char *) &in;
#ifdef  ACL_LINUX
	snprintf (dst, 18, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
#elif defined(SUNOS5)
	snprintf (dst, 18, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
#elif defined(HP_UX)
	snprintf (dst, 18, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
#elif defined(ACL_WINDOWS)
	snprintf (dst, 18, "%d.%d.%d.%d", bytes[3], bytes[2], bytes[1], bytes[0]);
#else
	snprintf (dst, 18, "%d.%d.%d.%d", bytes[0], bytes[1], bytes[2], bytes[3]);
#endif

	return(dst);
}

int acl_iplink_list(const ACL_IPLINK *plink)
{
	int   i, n;
	ACL_IPITEM *item;
	char  buf[64];
	unsigned ip_begin, ip_end;

	n = acl_array_size(plink->parray);
	for (i = 0; i < n; i++) {
		item = (ACL_IPITEM *) acl_array_index(plink->parray, i);
		if (item == NULL)
			break;
		ip_begin = (unsigned) item->begin;
		ip_end = (unsigned) item->end;
		__sane_inet_ntoa(ip_begin, buf, sizeof(buf));
		printf("ipbegin=%s", buf);
		__sane_inet_ntoa(ip_end, buf, sizeof(buf));
		printf(", ipend=%s\n", buf);
	}

	return n;
}
