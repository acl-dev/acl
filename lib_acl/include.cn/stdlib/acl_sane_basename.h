#ifndef	ACL_SANE_BASENAME_INCLUDE_H
#define	ACL_SANE_BASENAME_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_vstring.h"

/**
 * 从文件全路径中提取文件名
 * @parm bp {ACL_VSTRING*} 存储结果的缓冲区，若为 NULL 则引用内部的线程局部存储
 * @return {char*} 永不为空
 */
ACL_API char *acl_sane_basename(ACL_VSTRING *bp, const char *path);

/**
 * 从文件全路径中提取文件所在目录
 * @parm bp {ACL_VSTRING*} 存储结果的缓冲区，若为 NULL 则引用内部的线程局部存储
 * @return {char*} 永不为空
 */
ACL_API char *acl_sane_dirname(ACL_VSTRING *bp, const char *path);

#ifdef	__cplusplus
}
#endif

#endif
