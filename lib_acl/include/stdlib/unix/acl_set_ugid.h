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
 * 调用程序的用户 ID 及组 ID
 * @param uid {uid_t} 用户 ID
 * @param gid {gid_t} 组 ID
 * @return {int} 设置是否成功，0 表示成功，-1 表示失败
 */
int acl_set_ugid(uid_t uid, gid_t gid);

/**
 * 修改程序的用户 ID 为指定用户的 ID
 * @param user {char* } 系统用户账号名
 * @return {int} 设置是否成功，0 表示成功，-1 表示失败
 */
int acl_change_uid(const char *user);

#endif /* ACL_UNIX*/

#ifdef  __cplusplus
}
#endif


#endif

