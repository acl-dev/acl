#ifndef ACL_VSTRING_BASE64_CODE_INCLUDE_H
#define ACL_VSTRING_BASE64_CODE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_vstring.h"

/**
 * BASE64 encoding function.
 * @param vp {ACL_VSTRING*} Storage buffer object
 * @param in {const char*} Source data
 * @param len {int} in source data length
 * @return {ACL_VSTRING*} Same as vp
 */
ACL_API ACL_VSTRING *acl_vstring_base64_encode(ACL_VSTRING *vp,
	const char *in, int len);

/**
 * BASE64 decoding function.
 * @param vp {ACL_VSTRING*} Storage buffer object
 * @param in {const char*} Encoded data string
 * @param len {int} in data length
 * @return {ACL_VSTRING*} NULL: decoding failed; !=NULL:
 *  decoding succeeded, same address as vp
 */
ACL_API ACL_VSTRING *acl_vstring_base64_decode(ACL_VSTRING *vp,
	const char *in, int len);

#ifdef  __cplusplus
}
#endif

#endif
