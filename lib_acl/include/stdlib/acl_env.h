#ifndef	ACL_ENV_INCLUDE_H
#define	ACL_ENV_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

ACL_API void acl_clean_env(char **preserve_list);
ACL_API char *acl_getenv(const char *name);
ACL_API char *acl_getenv3(const char *name, char *buf, size_t len);
ACL_API int acl_setenv(const char *name, const char *val, int overwrite);
ACL_API int acl_putenv(char *str);
ACL_API const char *acl_getenv_list(void);

#ifdef	__cplusplus
}
#endif

#endif
