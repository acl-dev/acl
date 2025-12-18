#ifndef	ACL_BASE64_CODE_INCLUDE_H
#define	ACL_BASE64_CODE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

/**
 * BASE64 encoding function.
 * @param plain_in {const char*} Source plain text string
 * @param len {int} plain_in data length
 * @return {unsigned char*} BASE64 encoded data, must be freed with acl_myfree
 */
ACL_API unsigned char *acl_base64_encode(const char *plain_in, int len);

/**
 * BASE64 decoding function.
 * @param code_in {const char*} BASE64 encoded string
 * @param ppresult {char**} If decoding succeeds, stores decoded
 *  result. Caller must free the memory space with acl_myfree.
 * @return {int} -1: indicates decoding failed, *ppresult points
 *  to NULL; >0: indicates decoded data length, and *ppresult
 *  points to a dynamically allocated memory buffer storing
 *  decoded data. Caller must free *ppresult's dynamic memory with
 *  acl_myfree
 */
ACL_API int acl_base64_decode(const char *code_in, char **ppresult);


#ifdef  __cplusplus
}
#endif

#endif
