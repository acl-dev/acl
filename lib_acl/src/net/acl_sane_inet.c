#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <ctype.h>
#include <stdlib.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef	ACL_UNIX
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
#include "net/acl_sane_inet.h"

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

const char *acl_inet_ntoa(struct in_addr in, char *dst, size_t size)
{
	unsigned char *src = (unsigned char *) &in.s_addr;

	return (acl_inet_ntop4(src, dst, size));
}

int acl_is_ip(const char *pstrip)
{       
	const char *ptr;
	int   count = 0, n = 0;
	char  ch;

	if (pstrip == NULL || *pstrip == 0)
		return(-1);

	ptr = pstrip;
	if(*ptr == '.') {        /* the first char should not be '.' */
		return(-1);
	}
	while(*ptr) {
		if (*ptr == '.') {
			ch = *(ptr + 1);
			if (ch < '0' || ch > '9') {
				return(-1);
			}
			count++;
		} else {
			ch = *ptr;
			if (ch < '0' || ch > '9') {
				return(-1);
			}
		}
		ptr++;
		n++;
		if (n > 16)
			return (-1);
	}
	if(*(ptr - 1) == '.') {  /* the last char should not be '.' */
		return(-1);
	}
	if(count != 3) {         /* 192.168.0.1 has the number '.' is 4 */
		return(-1);
	}
	return(0);
}

int acl_ipv4_valid(const char *addr)
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
	while (*ptr) {
		n = *ptr;
		if (n < '0' || n > '9')
			return (0);
		ptr++;
		k--;
		if (k < 0)
			return (0);
	}
	return (1);
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

