#ifndef	ACL_HASH_INCLUD_H
#define	ACL_HASH_INCLUD_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdlib.h>

/**
 * Hash function type definition.
 * @param buf Data buffer or string to hash
 * @param len buf length
 */
typedef unsigned (*ACL_HASH_FN)(const void *buf, size_t len);

ACL_API unsigned short acl_hash_crc16(const void *buf, size_t len);
ACL_API unsigned acl_hash_crc32(const void *buf, size_t len);
ACL_API acl_uint64 acl_hash_crc64(const void *buf, size_t len);
ACL_API unsigned acl_hash_test(const void *buf, size_t len);
ACL_API unsigned acl_hash_bin(const void *buf, size_t len);
ACL_API unsigned acl_hash_func2(const void *buf, size_t len);
ACL_API unsigned acl_hash_func3(const void *buf, size_t len);
ACL_API unsigned acl_hash_func4(const void *buf, size_t len);
ACL_API unsigned acl_hash_func5(const void *buf, size_t len);
ACL_API unsigned acl_hash_func6(const void *buf, size_t len);

#ifdef	__cplusplus
}
#endif

#endif
