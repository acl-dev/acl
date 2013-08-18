#ifndef	__ACL_USERNAME_INCLUDE_H__
#define	__ACL_USERNAME_INCLUDE_H__

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

const char *acl_username(void);

#endif

#ifdef  __cplusplus
}
#endif

#endif

