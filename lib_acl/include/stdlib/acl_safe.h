#ifndef	ACL_SAFE_INCLUDE_H
#define	ACL_SAFE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

ACL_API int acl_unsafe(void);
ACL_API char *acl_safe_getenv(const char*name);

#ifdef	__cplusplus
}
#endif

#endif
