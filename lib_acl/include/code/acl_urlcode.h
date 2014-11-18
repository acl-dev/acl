#ifndef ACL_URLCODE_INCLUDE_H
#define ACL_URLCODE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

/**
 * URL 编码函数
 * @param str {const char*} 源字符串
 * @return {char*} 编码后的字符串，返回值不可能为空，需要用 acl_myfree 释放
 */
ACL_API char *acl_url_encode(const char *str);

/**
 * URL 解码函数
 * @param str {const char*} 经URL编码后的字符串
 * @return {char*} 解码后的字符串，返回值不可能为空，需要用 acl_myfree 释放
 */
ACL_API char *acl_url_decode(const char *str);

#ifdef __cplusplus
}
#endif
#endif
