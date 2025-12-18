#ifndef ACL_MASK_ADDR_H_INCLUDED
#define ACL_MASK_ADDR_H_INCLUDED

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

/**
 * Apply network mask length to IP address bytes.
 * @param addr_bytes {unsigned char*} Byte array of IP address,
 *  (can be IPv4/IPv6), this parameter is value type, operation
 *  is performed on this address
 * @param addr_byte_count {unsigned} addr_bytes address length
 * @param network_bits {unsigned} Network mask length
 */
ACL_API void acl_mask_addr(unsigned char *addr_bytes,
		unsigned addr_byte_count, unsigned network_bits);

#ifdef  __cplusplus
}
#endif

#endif
