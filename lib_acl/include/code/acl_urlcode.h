#ifndef ACL_URLCODE_INCLUDE_H
#define ACL_URLCODE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_dbuf_pool.h"

/**
 * URL encode function.
 * @param str {const char*} Source string
 * @param dbuf {ACL_DBUF_POOL*} Memory pool; if not NULL, the result is
 *  allocated from this pool, otherwise it is allocated via acl_mymalloc
 * @return {char*} Encoded string; must be freed with acl_myfree
 *  when dbuf is NULL
 */
ACL_API char *acl_url_encode(const char *str, ACL_DBUF_POOL *dbuf);

/**
 * URL decode function.
 * @param str {const char*} URL-encoded source string
 * @param dbuf {ACL_DBUF_POOL*} Memory pool; if not NULL, the result is
 *  allocated from this pool, otherwise it is allocated via acl_mymalloc
 * @return {char*} Decoded string; must be freed with acl_myfree
 *  when dbuf is NULL
 */
ACL_API char *acl_url_decode(const char *str, ACL_DBUF_POOL *dbuf);

#ifdef __cplusplus
}
#endif
#endif
