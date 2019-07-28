#ifndef ACL_CHROOT_UID_INCLUDE_H
#define ACL_CHROOT_UID_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

 /* External interface. */

extern int acl_chroot_uid(const char *, const char *);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif

