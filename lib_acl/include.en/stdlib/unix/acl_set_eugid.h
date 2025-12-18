#ifndef ACL_SET_EUGID_INCLUDE_H
#define ACL_SET_EUGID_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

#include <sys/types.h>
#include <unistd.h>

/**
 * Set process effective user ID and effective group ID.
 * @param euid {uid_t} Effective user ID
 * @param egid {gid_t} Effective group ID
 * @return {int} 0 indicates setting successful, -1 indicates setting failed
 */
int acl_set_eugid(uid_t euid, gid_t egid);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif


#endif
