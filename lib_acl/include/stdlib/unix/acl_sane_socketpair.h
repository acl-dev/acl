#ifndef ACL_SANE_SOCKETPAIR_H
#define ACL_SANE_SOCKETPAIR_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

 /* External interface. */

int acl_sane_socketpair(int domain, int type, int protocol, int result[2]);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif

