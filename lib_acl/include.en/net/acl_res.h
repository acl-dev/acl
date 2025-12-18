#ifndef ACL_RES_INCLUDE_H
#define ACL_RES_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "acl_netdb.h"
#include <time.h>

#ifdef	ACL_UNIX
#include <netinet/in.h>
#include <sys/un.h>
#endif

/**
 * DNS query result storage structure.
 */
typedef struct ACL_RES {
	char dns_ip[64];                /**< DNS server IP address */
	unsigned short dns_port;        /**< DNS server Port */
	unsigned short cur_qid;         /**< Internal query request packet identifier */
	time_t tm_spent;                /**< Query time spent (seconds) */
	int   errnum;
#define ACL_RES_ERR_SEND	-100    /**< Send error */
#define ACL_RES_ERR_READ	-101    /**< Read error */
#define ACL_RES_ERR_RTMO	-102    /**< Receive timeout */
#define ACL_RES_ERR_NULL	-103    /**< Empty result */
#define ACL_RES_ERR_CONN	-104    /**< TCP mode connection failure */

	int transfer;                   /**< TCP/UDP transfer mode */
#define ACL_RES_USE_UDP		0       /**< UDP transfer mode */
#define ACL_RES_USE_TCP		1       /**< TCP transfer mode */

	int   conn_timeout;             /**< TCP connection
					 *   establishment timeout,
					 *   default is 10 seconds */
	int   rw_timeout;               /**< TCP/UDP read/write IO
					 *   timeout, default is 10
					 *   seconds */
} ACL_RES;

/**
 * Create a DNS query object.
 * @param dns_ip {const char*} DNS server IP address
 * @param dns_port {unsigned short} DNS server Port
 * @return {ACL_RES*} Newly created query object
 */
ACL_API ACL_RES *acl_res_new(const char *dns_ip, unsigned short dns_port);

/**
 * Set DNS query timeout.
 * @param conn_timeout {int} TCP connection establishment timeout
 * @param rw_timeout {int} TCP/UDP read/write IO timeout
 */
ACL_API void acl_res_set_timeout(int conn_timeout, int rw_timeout);

/**
 * Free a DNS query object.
 * @param res {ACL_RES*} DNS query object
 */
ACL_API void acl_res_free(ACL_RES *res);

/**
 * Query IP address list for a certain domain name.
 * @param res {ACL_RES*} DNS query object
 * @param domain {const char*} Domain name to query
 * @return {ACL_DNS_DB*} Query result
 */
ACL_API ACL_DNS_DB *acl_res_lookup(ACL_RES *res, const char *domain);

#ifdef AF_INET6
ACL_API ACL_DNS_DB *acl_res_lookup6(ACL_RES *res, const char *domain);
#endif

/**
 * Get query failure reason based on error number.
 * @param errnum {int} Error number
 * @return {const char*} Error message
 */
ACL_API const char *acl_res_strerror(int errnum);

/**
 * Get current query error message.
 * @param res {ACL_RES*} DNS query object
 * @return {const char*} Error message
 */
ACL_API const char *acl_res_errmsg(const ACL_RES *res);

#ifdef __cplusplus
}
#endif

#endif
