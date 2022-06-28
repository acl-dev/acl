#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include <string.h>
#include "stdlib/acl_argv.h"
#include "net/acl_sane_inet.h"
#include "net/acl_rfc1035.h"

#endif

#include "util.h"

ACL_ARGV *res_a_create(const ACL_RFC1035_RR *answer, int n)
{
	int   i;
	struct in_addr sin_addr;
	ACL_ARGV *a;
	char ip[32];

	if (n <= 0)
		return (NULL);

	a = acl_argv_alloc(n);

	for (i = 0; i < n; i++) {
		if (answer[i].type == ACL_RFC1035_TYPE_A) {
			memcpy(&sin_addr, answer[i].rdata, 4);
			if (inet_ntop(AF_INET, &sin_addr, ip, sizeof(ip))) {
				acl_argv_add(a, ip, NULL);
			}
		}
	}

	return a;
}
