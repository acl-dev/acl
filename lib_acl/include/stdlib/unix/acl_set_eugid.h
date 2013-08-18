#ifndef __ACL_SET_EUGID_INCLUDE_H__
#define __ACL_SET_EUGID_INCLUDE_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

#include <sys/types.h>
#include <unistd.h>

void acl_set_eugid(uid_t euid, gid_t egid);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif


#endif

