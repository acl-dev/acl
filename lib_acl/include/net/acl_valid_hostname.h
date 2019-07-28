#ifndef ACL_VALID_HOSTNAME_INCLUDE_H
#define ACL_VALID_HOSTNAME_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
 /* External interface */

#define ACL_VALID_HOSTNAME_LEN	255	/* RFC 1035 */
#define ACL_VALID_LABEL_LEN	63	/* RFC 1035 */

#define ACL_DONT_GRIPE		0
#define ACL_DO_GRIPE		1

ACL_API int acl_valid_hostname(const char *, int);
ACL_API int acl_valid_hostaddr(const char *, int);
ACL_API int acl_valid_ipv4_hostaddr(const char *, int);
ACL_API int acl_valid_ipv6_hostaddr(const char *, int);
ACL_API int acl_valid_unix(const char *);

#ifdef  __cplusplus
}
#endif

#endif

