#ifndef	ACL_PROCESS_INCLUDE_H
#define	ACL_PROCESS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Get the full path of the currently running executable program stored in the file system.
 * @return {const char*} NULL: cannot get; != NULL: return value is the executable
 *  program's full storage path in the file system
 */
ACL_API const char *acl_process_path(void);

/**
 * Get the current working directory path.
 * @return {const char*} NULL: cannot get; != NULL: return value is the current
 *  working directory path
 */
ACL_API const char *acl_getcwd(void);

#ifdef	__cplusplus
}
#endif

#endif
