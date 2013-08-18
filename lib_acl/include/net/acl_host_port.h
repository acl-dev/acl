#ifndef __ACL_HOST_PORT_H_INCLUDED__
#define __ACL_HOST_PORT_H_INCLUDED__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

 /* External interface. */

/**
 * [host]:port, [host]:, [host].
 * or
 * host:port, host:, host, :port, port.
 */
ACL_API const char *acl_host_port(char *buf, char **host, char *def_host,
		char **port, char *def_service);

#ifdef	__cplusplus
}
#endif

#endif

