#ifndef __SYS_PATCH_INCLUDE_H__
#define __SYS_PATCH_INCLUDE_H__

#include "lib_acl.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ACL_MS_WINDOWS
struct tm *localtime_r(const time_t *timep, struct tm *result);
#define snprintf _snprintf
#define getpid _getpid
#endif

#ifdef __cplusplus
}
#endif

#endif

