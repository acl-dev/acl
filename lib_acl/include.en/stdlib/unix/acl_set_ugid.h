#ifndef ACL_SET_UGID_INCLUDE_H
#define ACL_SET_UGID_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

#include <sys/types.h>
#include <unistd.h>

/**
 * Set process user ID and group ID.
 * @param uid {uid_t} User ID
 * @param gid {gid_t} Group ID
 * @return {int} Whether operation succeeded, 0 indicates success, -1 indicates failure
 */
int acl_set_ugid(uid_t uid, gid_t gid);

/**
 * Change process user ID to specified user's ID.
 * @param user {char* } System user account name
 * @return {int} Whether operation succeeded, 0 indicates success, -1 indicates failure
 */
int acl_change_uid(const char *user);

#endif /* ACL_UNIX*/

#ifdef  __cplusplus
}
#endif


#endif
