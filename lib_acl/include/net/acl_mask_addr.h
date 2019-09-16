#ifndef ACL_MASK_ADDR_H_INCLUDED
#define ACL_MASK_ADDR_H_INCLUDED

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

/**
 * 给定网络掩码长度及IP地址，获得其网络地址
 * @param addr_bytes {unsigned char*} 给定的网络字节序 IP 地址,
 *  (可以为IPv4/IPv6), 该参数为值参型，结果存于该地址中
 * @param addr_byte_count {unsigned} addr_bytes 地址长度
 * @param network_bits {unsigned} 网络掩码的长度
 */
ACL_API void acl_mask_addr(unsigned char *addr_bytes,
		unsigned addr_byte_count, unsigned network_bits);

#ifdef  __cplusplus
}
#endif

#endif

