#ifndef	ACL_GBCODE_INCLUDE_H
#define	ACL_GBCODE_INCLUDE_H

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif
#include "../stdlib/acl_define.h"

/**
 * Convert simplified Chinese characters in a GBK string to traditional Chinese.
 * @param data {const char*} Input data buffer (GBK encoded)
 * @param dlen {size_t} Length of data
 * @param buf {char*} Output buffer; its address can be the same
 *  as data for in-place conversion
 * @param size {size_t} Size of buf
 */
ACL_API void acl_gbjt2ft(const char *data, size_t dlen, char *buf, size_t size);


/**
 * Convert traditional Chinese characters in a GBK string to simplified Chinese.
 * @param data {const char*} Input data buffer (GBK encoded)
 * @param dlen {size_t} Length of data
 * @param buf {char*} Output buffer; its address can be the same
 *  as data for in-place conversion
 * @param size {size_t} Size of buf
 */
ACL_API void acl_gbft2jt(const char *data, size_t dlen, char *buf, size_t size);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */

#endif
