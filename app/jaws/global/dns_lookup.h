#ifndef	__DNS_LOOKUP_INCLUDE_H__
#define	__DNS_LOOKUP_INCLUDE_H__

#include "dns.h"
#include "service_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

void dns_lookup(CLIENT_ENTRY *entry, const char *domain, int port);

#ifdef	__cplusplus
}
#endif

#endif
