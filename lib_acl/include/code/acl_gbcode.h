#ifndef	ACL_GBCODE_INCLUDE_H
#define	ACL_GBCODE_INCLUDE_H

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif
#include "../stdlib/acl_define.h"

/**
 * 将GBK字符集中的简体转换为GBK字符集中的繁体
 * @param data {const char*} 简体数据
 * @param dlen {size_t} data 长度
 * @param buf {char*} 存储转换后结果，其中 buf 地址和 data 可以是同一地址
 * @param size {size_t} buf 空间大小
 */
ACL_API void acl_gbjt2ft(const char *data, size_t dlen, char *buf, size_t size);


/**
 * 将GBK字符集中的繁体转换为GBK字符集中的简体
 * @param data {const char*} 繁体数据
 * @param dlen {size_t} data 长度
 * @param buf {char*} 存储转换后结果，其中 buf 地址和 data 可以是同一地址
 * @param size {size_t} buf 空间大小
 */
ACL_API void acl_gbft2jt(const char *data, size_t dlen, char *buf, size_t size);

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */

#endif
