#ifndef ACL_URLCODE_INCLUDE_H
#define ACL_URLCODE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_dbuf_pool.h"

/**
 * URL 编码函数
 * @param str {const char*} 源字符串
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，如果非空，则内部使用该内存池进行
 *  内存的动态分配，否则则使用 acl_mymalloc 分配动态内存
 * @return {char*} 编码后的字符串，返回值不可能为空，需要用 acl_myfree 释放
 */
ACL_API char *acl_url_encode(const char *str, ACL_DBUF_POOL *dbuf);

/**
 * URL 解码函数
 * @param str {const char*} 经URL编码后的字符串
 * @param dbuf {ACL_DBUF_POOL*} 内存池对象，如果非空，则内部使用该内存池进行
 *  内存的动态分配，否则则使用 acl_mymalloc 分配动态内存
 * @return {char*} 解码后的字符串，返回值不可能为空，需要用 acl_myfree 释放
 */
ACL_API char *acl_url_decode(const char *str, ACL_DBUF_POOL *dbuf);

#ifdef __cplusplus
}
#endif
#endif
