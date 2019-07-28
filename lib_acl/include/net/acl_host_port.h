#ifndef ACL_HOST_PORT_INCLUDE_H
#define ACL_HOST_PORT_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

 /* External interface. */

/**
 * [host]:port, [host]:, [host].
 * or
 * host:port, host:, host, :port, port.
 */
ACL_API const char *acl_host_port(char *buf, char **host,
	char *def_host, char **port, char *def_service);

ACL_API struct addrinfo *acl_host_addrinfo(const char *addr, int type);

#ifdef	__cplusplus
}
#endif

#endif

