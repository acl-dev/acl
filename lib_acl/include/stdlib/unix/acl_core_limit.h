#ifndef	ACL_CORE_LIMIT_INCLUDE_H
#define	ACL_CORE_LIMIT_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"
#ifdef ACL_UNIX

/**
 * Use this function to set the maximum size of core file when program crashes.
 * @param max {long long int} Meaning of max value range, same as system core file settings:
 *  1)   0: prohibit generating core file
 *  2) < 0: allow core file, and do not limit core file size
 *  3) > 0: allow core file, and core file size cannot exceed max
 */
void acl_set_core_limit(long long int max);

#endif  /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif
