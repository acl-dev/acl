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
 * 设置程序的有效用户 ID 及有效组 ID
 * @param euid {uid_t} 有效用户 ID
 * @param egid {gid_t} 有效组 ID
 * @return {int} 0 表示设置成功，-1 表示设置失败
 */
int acl_set_eugid(uid_t euid, gid_t egid);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif


#endif

