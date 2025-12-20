#ifndef	ACL_IFCONF_INCLUDE_H
#define	ACL_IFCONF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_argv.h"
#include "acl_sane_inet.h"

typedef struct ACL_IFADDR {
	char name[256];		/* Interface name */
#if defined(_WIN32) || defined(_WIN64)
	char desc[256];		/* Interface description */
#endif
	char addr[128];		/* Address string, represents IP address */
	ACL_SOCKADDR saddr;	/* Unified address for IPV4 & IPV6 */
} ACL_IFADDR;

typedef struct ACL_IFCONF {
	ACL_IFADDR *addrs;	/* ACL_IFADDR array */
	int  length;		/* ACL_IFADDR array length */

	/* for acl_iterator */

	/* Get iterator head pointer */
	const ACL_IFADDR *(*iter_head)(ACL_ITER*, struct ACL_IFCONF*);
	/* Get next iterator pointer */
	const ACL_IFADDR *(*iter_next)(ACL_ITER*, struct ACL_IFCONF*);
	/* Get iterator tail pointer */
	const ACL_IFADDR *(*iter_tail)(ACL_ITER*, struct ACL_IFCONF*);
	/* Get previous iterator pointer */
	const ACL_IFADDR *(*iter_prev)(ACL_ITER*, struct ACL_IFCONF*);
} ACL_IFCONF;

/**
 * Get all network interface addresses on local machine.
 * @return {ACL_IFCONF*} Return value, when non-NULL, caller must
 *  free with acl_free_ifaddrs
 */
ACL_API ACL_IFCONF *acl_get_ifaddrs(void);

/**
 * Free ACL_IFCONF memory returned by acl_get_ifaddrs().
 * @param ifconf {ACL_IFCONF*}
 */
ACL_API void acl_free_ifaddrs(ACL_IFCONF *ifconf);

/**
 * Scan local network interface IPs, match specified pattern IP
 * addresses and return.
 * Currently only supports IPV4.
 * @param pattern {const char *} Specified matching pattern,
 *  format: xxx.xxx.xxx.xxx or xxx.xxx.xxx.xxx:port, e.g.,
 *  192.168.*.*, 192.168.*.8:80, 10.*.0.*:81
 * @return {ACL_IFCONF *} Returns result structure matching
 *  pattern. If pattern contains port, automatically adds port to
 *  each IP. Return value must not be NULL, caller must free this
 *  object with acl_free_ifaddrs
 */
ACL_API ACL_IFCONF *acl_ifconf_search(const char *pattern);

/**
 * Scan local network interface IPs, return matching addresses.
 * @param patterns {const char*}
 * @param unix_path {const char*} When matching address is unix
 *  domain socket, path is full path or prefix path
 * @return {ACL_ARGV*} When non-NULL, returns array of matching
 *  address strings, caller must free with acl_argv_free; if NULL,
 *  indicates no matching addresses found
 */
ACL_API ACL_ARGV *acl_search_addrs(const char *patterns, const char *unix_path);

#ifdef	__cplusplus
}
#endif

#endif
