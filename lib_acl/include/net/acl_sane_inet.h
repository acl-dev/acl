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
 * Convert socket address to string format, supports IPV4, IPV6
 * and UNIX domain socket.
 * @param sa {const struct sockaddr*}
 * @param buf {char*} Storage for converted result
 * @param size {size_t} buf space size
 * @return {size_t} Actual length corresponding to sockaddr
 *  address type, for IPV4 corresponds to struct sockaddr_in
 *  structure length, for IPV6 corresponds to struct sockaddr_in6
 *  structure length, return value 0 indicates conversion failed
 */
ACL_API size_t acl_inet_ntop(const struct sockaddr *sa, char *buf, size_t size);

/**
 * Convert string representation of address to socket address,
 * supports IPV4, IPV6 and UNIX domain socket.
 * @param af {int} Address type: AF_INET (IPV4) or AF_INET6 (IPV6)
 * @param src {const char*} String representation of address,
 *  can be ip, ip#port or ipv4:port
 * @param dst {struct sockaddr*} Storage for converted result
 * @return {size_t} Returns size corresponding to IPV4 or IPV6
 *  address structure, for IPV4 corresponds to struct sockaddr_in
 *  structure length, for IPV6 corresponds to struct sockaddr_in6
 *  structure length, return value 0 indicates conversion failed
 */
ACL_API size_t acl_inet_pton(int af, const char *src, struct sockaddr *dst);

/**
 * Convert string representation of address to socket address,
 * supports IPV4, IPV6 and UNIX domain socket.
 * Internally automatically detects string address type,
 * automatically determines IPV4 or IPV6.
 * @param src {const char*} String representation of address,
 *  can be ip, ip#port or ipv4:port
 * @param dst {struct sockaddr*} Storage for converted result
 * @return {size_t} Returns size corresponding to IPV4 or IPV6
 *  address structure, for IPV4 corresponds to struct sockaddr_in
 *  structure length, for IPV6 corresponds to struct sockaddr_in6
 *  structure length, return value 0 indicates conversion failed
 */
ACL_API size_t acl_sane_pton(const char *src, struct sockaddr *dst);

/**
 * Convert IP address to string format.
 * @param src {const unsigned char*} struct in_addr in.s_addr byte array representation
 * @param dst {char *} Storage for converted result
 * @param size {size_t} dst space size
 * @return {const char*} NULL: error; !NULL: ok
 */
ACL_API const char *acl_inet_ntop4(const unsigned char *src, char *dst, size_t size);

/**
 * Convert IP address to string format.
 * @param in {struct in_addr}
 * @param dst {char *} Storage for converted result
 * @param size {size_t} dst space size
 * @return {const char*} NULL: error; !NULL: ok
 */
ACL_API const char *acl_inet_ntoa(struct in_addr in, char *dst, size_t size);

#ifdef AF_INET6
ACL_API const char *acl_inet6_ntoa(struct in6_addr in6, char *buf, size_t size);
#endif

/**
 * Check whether the given address string is a valid IP address.
 * @param ip {const char *ip}
 * @return {int} != 0: yes; == 0: no
 */
ACL_API int acl_is_ip(const char *ip);
ACL_API int acl_is_ipv4(const char *ip);
ACL_API int acl_is_ipv6(const char *ip);

/**
 * Check whether the given IP address is in xxx.xxx.xxx.xxx:port format.
 * @param addr {const char*} IP:PORT address
 * @return {int} 1: valid, 0: invalid
 */
ACL_API int acl_ipv4_addr_valid(const char *addr);

#ifdef	__cplusplus
}
#endif

#endif
