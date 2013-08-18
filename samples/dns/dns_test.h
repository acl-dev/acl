#ifndef __DNS_TEST_INCLUDE_H__
#define __DNS_TEST_INCLUDE_H__

#include "lib_acl.h"
extern int dns_lookup(const char *domain, const char *dns_ip,
	unsigned short dns_port, ACL_VSTRING *sbuf);

#endif
