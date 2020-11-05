#ifndef	__RESOLVER_INCLUDE_H__
#define	__RESOLVER_INCLUDE_H__

#include "common/argv.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct HOST_LOCAL {
	char ipv4[64];
	char ipv6[64];
} HOST_LOCAL;

typedef struct SERVICE_PORT {
	char name[128];
	unsigned short port;
	ARGV *transports;
} SERVICE_PORT;

void resolver_init_once(void);
struct addrinfo *resolver_getaddrinfo(const char *name, const char *service,
	const struct addrinfo* hints);
void resolver_freeaddrinfo(struct addrinfo *res);
struct addrinfo *resolver_addrinfo_alloc(const struct sockaddr *sa);

unsigned short get_service_port(const char *name);
const HOST_LOCAL *find_from_localhost(const char *name);

#ifdef	__cplusplus
}
#endif

#endif
