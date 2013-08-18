#ifndef __ACL_CHROOT_UID_H_INCLUDED__
#define __ACL_CHROOT_UID_H_INCLUDED__

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

 /* External interface. */

extern void acl_chroot_uid(const char *, const char *);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif

