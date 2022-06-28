#ifndef ACL_MAKE_DIRS_INCLUDE_H
#define ACL_MAKE_DIRS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 功能: 创建多级目录结构
 *  如创建 "/tmp/dir1/dir2" (for unix) 或 "C:\test\test1\test2" (for win32)
 * @param path 一级或多级目录路径
 * @param perms 创建权限(如: 0755, 0777, 0644 ...)
 * @return 0: OK;  -1: Err
 */
ACL_API int acl_make_dirs(const char *path, int perms);

#ifdef	__cplusplus
}
#endif

#endif

