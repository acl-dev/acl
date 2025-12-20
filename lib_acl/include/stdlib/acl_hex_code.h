#ifndef ACL_HEX_CODE_INCLUDE_H
#define ACL_HEX_CODE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstring.h"

/**
 * Encode binary data as hexadecimal, converting one byte to two hexadecimal characters,
 * thus converting to text string.
 * @param buf {ACL_VSTRING*} Storage for conversion result
 * @param ptr {const char*} Binary data source
 * @param len {int} ptr data length
 * @return {ACL_VSTRING*} If conversion succeeds, same as buf
 */
ACL_API ACL_VSTRING *acl_hex_encode(ACL_VSTRING *buf, const char *ptr, int len);

/**
 * Decode hexadecimal data.
 * @param buf {ACL_VSTRING*} Storage for conversion result
 * @param ptr {const char*} Hexadecimal string
 * @param len {int} ptr data length
 * @return {ACL_VSTRING*} If decoding succeeds, same as buf, otherwise returns NULL
 */
ACL_API ACL_VSTRING *acl_hex_decode(ACL_VSTRING *buf, const char *ptr, int len);

#ifdef  __cplusplus
}
#endif

#endif
