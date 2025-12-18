#ifndef ACL_MAKE_DIRS_INCLUDE_H
#define ACL_MAKE_DIRS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Function: Create multi-level directory structure
 *  For example, create "/tmp/dir1/dir2" (for unix) or "C:\test\test1\test2" (for win32)
 * @param path A multi-level directory path
 * @param perms Directory permissions (e.g., 0755, 0777, 0644 ...)
 * @return 0: OK;  -1: Err
 */
ACL_API int acl_make_dirs(const char *path, int perms);

#ifdef	__cplusplus
}
#endif

#endif
