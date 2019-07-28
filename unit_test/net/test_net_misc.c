#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "test_net.h"

int test_mask_addr(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *ip;
	int   bits;
	unsigned int addr;
	struct in_addr in;
	char  buf[32];

	AUT_SET_STR(test_line, "ip", ip);
	AUT_SET_INT(test_line, "bits", bits);

/*
	addr = ntohl(inet_addr(ip));
*/
	addr = inet_addr(ip);
	acl_mask_addr((unsigned char *) &addr, sizeof(addr), bits);
	in.s_addr = addr;
	printf("i: %s, addr: %s\n", ip, acl_inet_ntoa(in, buf, sizeof(buf)));
	return (0);
}
