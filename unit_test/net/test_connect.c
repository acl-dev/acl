
#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "test_net.h"

int test_connect(AUT_LINE *test_line, void *arg acl_unused)
{
	const char *addr;
	ACL_VSTREAM *client;
	char  ip_local[32], ip_remote[32];
	
	AUT_SET_STR(test_line, "addr", addr);

	client = acl_vstream_connect(addr, ACL_BLOCKING, 0, 10, 1024);
	if (client == NULL) {
		printf("connect addr(%s) error\r\n", addr);
		return (-1);
	}

	acl_getpeername(ACL_VSTREAM_SOCK(client), ip_remote, sizeof(ip_remote));
	acl_getsockname(ACL_VSTREAM_SOCK(client), ip_local, sizeof(ip_local));

	printf("connect addr(%s) ok, local ip(%s), remote ip(%s)\r\n",
		addr, ip_local, ip_remote);
	acl_vstream_fprintf(client, "%s",
		"GET / HTTP/1.0\r\n"
		"HOST: www.test.com\r\n"
		"Connection: close\r\n"
		"\r\n");
		
	acl_vstream_close(client);
	return (0);
}

