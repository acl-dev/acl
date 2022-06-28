#ifndef	__SANE_INET_INCLUDE_H__
#define	__SANE_INET_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * 将IP地址转换成字符串格式
 * @param src {const unsigned char*} struct in_addr in.s_addr 的连续内存表示
 * @param dst {char *} 存储转换结果
 * @param size {size_t} dst 的空间大小
 * @return {const char*} NULL: error; !NULL: ok
 */
const char *inet_ntop4(const unsigned char *src, char *dst, size_t size);

/**
 * 将IP地址转换成字符串格式
 * @param in {struct in_addr}
 * @param dst {char *} 存储转换结果
 * @param size {size_t} dst 的空间大小
 * @return {const char*} NULL: error; !NULL: ok
 */
const char *sane_inet_ntoa(struct in_addr in, char *dst, size_t size);

/**
 * 判断给定的字符串是否是正确的 ip 地址
 * @param ip {const char *ip}
 * @return {int} != 0: 是; == 0: 否
 */
int is_ip(const char *ip);
int is_ipv4(const char *ip);
int is_ipv6(const char *ip);

/**
 * 判断所给的 ip 地址是否符合 xxx.xxx.xxx.xxx:port 格式
 * @param addr {const char*} IP:PORT 地址
 * @return {int} 1: 符合, 0: 不符合
 */
int ipv4_addr_valid(const char *addr);

#ifdef	__cplusplus
}
#endif

#endif

