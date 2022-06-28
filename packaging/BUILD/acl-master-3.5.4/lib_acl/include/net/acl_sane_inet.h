#ifndef	ACL_SANE_INET_INCLUDE_H
#define	ACL_SANE_INET_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <netinet/in.h>
#include <sys/un.h>
#endif

typedef union {
	struct sockaddr_storage ss;
#ifdef AF_INET6
	struct sockaddr_in6 in6;
#endif
	struct sockaddr_in in;
#ifdef ACL_UNIX
	struct sockaddr_un un;
#endif
	struct sockaddr sa;
} ACL_SOCKADDR;

/**
 * 将 socket 地址转为字符串格式，同时支持 IPV4 与 IPV6 及 UNIX 域套接口
 * @param sa {const struct sockaddr*}
 * @param buf {char*} 存储转换结果
 * @param size {size_t} buf 空间大小
 * @return {size_t} 返回 sockaddr 地址所对应地址类型的实际长度，如对于 IPV4 则
 *  对应 struct sockaddr_in 的结构体长度，对于 IPV6 则对应 struct sockaddr_in6
 *  的结构体长度，返回值 0 表示转换出错
 */
ACL_API size_t acl_inet_ntop(const struct sockaddr *sa, char *buf, size_t size);

/**
 * 将字符串表示的地址转为 socket 地址，支持 IPV4 与 IPV6 及 UNIX 域套接口
 * @param af {int} 地址类型，AF_INET（IPV4）或 AF_INET6（IPV6）
 * @param src {const char*} 字符串表示的地址，可以为 ip、ip#port 或 ipv4:port
 * @param dst {struct sockaddr*} 存储转换结果
 * @return {size_t} 返回对应  IPV4 或 IPV6 地址结构体的大小，如对于 IPV4 则
 *  对应 struct sockaddr_in 的结构体长度，对于 IPV6 则对应 struct sockaddr_in6
 *  的结构体长度，返回值 0 表示转换出错
 */
ACL_API size_t acl_inet_pton(int af, const char *src, struct sockaddr *dst);

/**
 * 将字符串表示的地址转为 socket 地址，支持 IPV4 与 IPV6 及 UNIX 域套接口，
 * 内部将自动探测所给地址字符串的地址类型，即自动区分是 IPV4 还是 IPV6
 * @param src {const char*} 字符串表示的地址，可以为 ip、ip#port 或 ipv4:port
 * @param dst {struct sockaddr*} 存储转换结果
 * @return {size_t} 返回对应  IPV4 或 IPV6 地址结构体的大小，如对于 IPV4 则
 *  对应 struct sockaddr_in 的结构体长度，对于 IPV6 则对应 struct sockaddr_in6
 *  的结构体长度，返回值 0 表示转换出错
 */
ACL_API size_t acl_sane_pton(const char *src, struct sockaddr *dst);

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
ACL_API const char *acl_inet_ntoa(const struct in_addr in, char *dst, size_t size);

#ifdef AF_INET6
ACL_API const char *acl_inet6_ntoa(const struct in6_addr in6, char *buf, size_t size);
#endif

/**
 * 判断给定的字符串是否是正确的 ip 地址
 * @param ip {const char *ip}
 * @return {int} != 0: 是; == 0: 否
 */
ACL_API int acl_is_ip(const char *ip);
ACL_API int acl_is_ipv4(const char *ip);
ACL_API int acl_is_ipv6(const char *ip);

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

