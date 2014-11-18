#ifndef	ACL_HASH_INCLUD_H
#define	ACL_HASH_INCLUD_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdlib.h>

/**
 * 哈希函数类型定义
 * @param buffer 需要被哈希的字符串
 * @param len s 的长度
 */
typedef unsigned (*ACL_HASH_FN)(const void *buffer, size_t len);

ACL_API unsigned acl_hash_crc32(const void *key, size_t len);
ACL_API acl_uint64 acl_hash_crc64(const void *key, size_t len);
ACL_API unsigned acl_hash_test(const void *key, size_t len);
ACL_API unsigned acl_hash_bin(const void *key, size_t len);
ACL_API unsigned acl_hash_func2(const void *key, size_t len);
ACL_API unsigned acl_hash_func3(const void *key, size_t len);
ACL_API unsigned acl_hash_func4(const void *key, size_t len);
ACL_API unsigned acl_hash_func5(const void *key, size_t len);
ACL_API unsigned acl_hash_func6(const void *key, size_t len);

#ifdef	__cplusplus
}
#endif

#endif

