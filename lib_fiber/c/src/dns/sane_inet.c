#include "stdafx.h"
#include "valid_hostname.h"
#include "sane_inet.h"

const char *inet_ntop4(const unsigned char *src, char *dst, size_t size)
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

const char *sane_inet_ntoa(struct in_addr in, char *dst, size_t size)
{
	unsigned char *src = (unsigned char *) &in.s_addr;

	return (inet_ntop4(src, dst, size));
}

int is_ipv4(const char *ip)
{       
	return valid_ipv4_hostaddr(ip, 0);
}

int is_ipv6(const char *ip)
{
	return valid_ipv6_hostaddr(ip, 0);
}

int is_ip(const char *ip)
{
	return is_ipv4(ip) || is_ipv6(ip);
}

int ipv4_addr_valid(const char *addr)
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
