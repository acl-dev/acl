#ifndef	__ACL_SAFE_INCLUDE_H__
#define	__ACL_SAFE_INCLUDE_H__

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
