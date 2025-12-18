#ifndef ACL_VALID_HOSTNAME_INCLUDE_H
#define ACL_VALID_HOSTNAME_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
 /* External interface */

#define ACL_VALID_HOSTNAME_LEN	255	/* RFC 1035 */
#define ACL_VALID_LABEL_LEN	63	/* RFC 1035 */

#define ACL_DONT_GRIPE		0
#define ACL_DO_GRIPE		1

/**
 * Check if the specified name is a valid host name.
 * @param name {const char*} the specified host name.
 * @param gripe {int} if log the warning info if name is invalid.
 * @return {int} return 1 if valid, 0 if invalid.
 */
ACL_API int acl_valid_hostname(const char *name, int gripe);

/**
 * Check if the specified address if a valid IPv4 or IPv6 address.
 * @param addr {const char*} the specified address.
 * @param gripe {int} if log the warning info if name is invalid.
 * @return {int} return 1 if valid, 0 if invalid.
 */
ACL_API int acl_valid_hostaddr(const char *addr, int gripe);

/**
 * Check if the specified address if a valid IPv4 or IPv6 address.
 * @param addr {const char*} the specified address.
 * @param gripe {int} if log the warning info if name is invalid.
 * @return {int} return 0 if not a valid address, else if
 *  1: wildcard address format, such as |port, *|port, or *.*.*.*|port
 *  2: IPv4 address format, ipv4:port, ipv4|port
 *  3: IPv6 address format, [ipv6]:port, ipv6|port
 */
ACL_API int acl_check_hostaddr(const char *addr, int gripe);
#define ACL_HOSTADDR_TYPE_NONE		0
#define ACL_HOSTADDR_TYPE_WILDCARD	1
#define ACL_HOSTADDR_TYPE_IPV4		2
#define ACL_HOSTADDR_TYPE_IPV6		3

/**
 * Check if the specified address if a valid IPv4 address.
 * @param addr {const char*} the specified address.
 * @param gripe {int} if log the warning info if name is invalid.
 * @return {int} return 1 if valid, 0 if invalid.
 */
ACL_API int acl_valid_ipv4_hostaddr(const char *addr, int gripe);

/**
 * Check if the specified address if a valid IPv6 address.
 * @param addr {const char*} the specified address.
 * @param gripe {int} if log the warning info if name is invalid.
 * @return {int} return 1 if valid, 0 if invalid.
 */
ACL_API int acl_valid_ipv6_hostaddr(const char *addr, int gripe);

/**
 * Check if the specified address if a valid unix domain address.
 * @param addr {const char*} the specified address.
 * @return {int} return 1 if valid, 0 if invalid.
 */
ACL_API int acl_valid_unix(const char *addr);

/**
 * Parse the specified address into ip and port.
 * @param addr {const char*} the specified address.
  * @param ip {char*} will save the ip address.
 * @param size {size_t} ip buffer size.
 * @param port {int*} will save port if not NULL.
 * @return {int} return 1 if parsing successfully, 0 if error.
 */
ACL_API int acl_parse_hostaddr(const char *addr, char *ip, size_t size, int *port);

/**
  * Parse the specified address into ip and port as the IPv4 format.
  * @param addr {const char*} the specified address.
  * @param ip {char*} will save the ip address.
  * @param size {size_t} ip buffer size.
  * @param port {int*} will save port if not NULL.
  * @return {int} return 1 if parsing successfully, 0 if error.
  */
ACL_API int acl_parse_ipv4_hostaddr(const char *addr, char *ip, size_t size, int *port);

/**
  * Parse the specified address into ip and port as the IPv6 format.
  * @param addr {const char*} the specified address.
  * @param ip {char*} will save the ip address.
  * @param size {size_t} ip buffer size.
  * @param port {int*} will save port if not NULL.
  * @return {int} return 1 if parsing successfully, 0 if error.
  */
ACL_API int acl_parse_ipv6_hostaddr(const char *addr, char *ip, size_t size, int *port);

#ifdef  __cplusplus
}
#endif

#endif
