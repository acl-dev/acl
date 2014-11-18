#ifndef	ACL_SANE_INET_INCLUDE_H
#define	ACL_SANE_INET_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <netinet/in.h>
#endif

/**
 * 将IP地址转换成字符串格式
 * @param src {const unsigned char*} struct in_addr in.s_addr 的连续内存表示
 * @param dst {char *} 存储转换结果
 * @param size {size_t} dst 的空间大小
 * @return {const char*} NULL: error; !NULL: ok
 */
ACL_API const char *acl_inet_ntop4(const unsigned char *src, char *dst, size_t size);

/**
 * 将IP地址转换成字符串格式
 * @param in {struct in_addr}
 * @param dst {char *} 存储转换结果
 * @param size {size_t} dst 的空间大小
 * @return {const char*} NULL: error; !NULL: ok
 */
ACL_API const char *acl_inet_ntoa(struct in_addr in, char *dst, size_t size);

/**
 * 判断给定的字符串是否是正确的 ip 地址
 * @param ip {const char *ip}
 * @return {int} 0: 是; -1: 否
 */
ACL_API int acl_is_ip(const char *ip);

/**
 * 判断所给的 ip 地址是否符合 xxx.xxx.xxx.xxx 格式
 * @param addr {const char*} IP 地址
 * @return {int} 1: 符合, 0: 不符合
 */
ACL_API int acl_ipv4_valid(const char *addr);

/**
 * 判断所给的 ip 地址是否符合 xxx.xxx.xxx.xxx:port 格式
 * @param addr {const char*} IP:PORT 地址
 * @return {int} 1: 符合, 0: 不符合
 */
ACL_API int acl_ipv4_addr_valid(const char *addr);

#ifdef	__cplusplus
}
#endif

#endif

