#ifndef __ACL_SET_UGID_INCLUDE_H__
#define __ACL_SET_UGID_INCLUDE_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

#include <sys/types.h>
#include <unistd.h>

void acl_set_ugid(uid_t uid, gid_t gid);
int acl_change_uid(char *user_name);

#endif /* ACL_UNIX*/

#ifdef  __cplusplus
}
#endif


#endif

