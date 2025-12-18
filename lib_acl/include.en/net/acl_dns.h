#ifndef	ACL_DNS_INCLUDE_H
#define	ACL_DNS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../stdlib/acl_htable.h"
#include "../stdlib/acl_cache2.h"
#ifdef  ACL_UNIX
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include "../event/acl_events.h"
#include "../aio/acl_aio.h"
#include "acl_rfc1035.h"
#include "acl_sane_inet.h"
#include "acl_netdb.h"

/* Error code definitions for DNS queries */

#define	ACL_DNS_OK		0
#define	ACL_DNS_OK_CACHE	1
#define	ACL_DNS_ERR_FMT		-1
#define	ACL_DNS_ERR_SVR		-2
#define	ACL_DNS_ERR_NO_EXIST	-3
#define	ACL_DNS_ERR_NO_SUPPORT	-4
#define	ACL_DNS_ERR_DENY	-5
#define	ACL_DNS_ERR_YX		-6
#define	ACL_DNS_ERR_YXRR	-7
#define	ACL_DNS_ERR_NXRR	-8
#define	ACL_DNS_ERR_NO_AUTH	-9
#define	ACL_DNS_ERR_NOT_ZONE	-10
#define	ACL_DNS_ERR_UNPACK	-15
#define	ACL_DNS_ERR_TIMEOUT	-16
#define	ACL_DNS_ERR_EXIST	-17
#define	ACL_DNS_ERR_BUILD_REQ	-18

typedef struct ACL_DNS_ADDR {
	char  ip[64];			/* DNS server IP address */
	unsigned short port;		/* DNS server port */
	ACL_SOCKADDR   addr;		/* DNS address */
	int   addr_len;			/* addr size */
	int   mask_length;		/* DNS server's network mask length (> 0 && < 32) */
	struct in_addr in;		/* addr network segment address */
} ACL_DNS_ADDR;

typedef struct ACL_DNS {
	ACL_AIO *aio;			/* Async IO object */
	unsigned short qid;		/* Query request ID identifier */
	ACL_ASTREAM *astream;		/* Async stream */

	ACL_ARRAY *groups;		/* Group list */
	ACL_ARRAY *dns_list;		/* DNS server address list */
	unsigned   dns_idx;		/* Current dns_list array index in use */
	ACL_DNS_ADDR addr_from;		/* Source DNS address */
	ACL_HTABLE *lookup_table;	/* Query table */
	ACL_CACHE2 *dns_cache;		/* Cache for DNS query results */
	int   timeout;			/* Timeout value for each query (seconds) */
	int   retry_limit;		/* Retry count when query times out */
	unsigned int flag;		/* Flag bits */
#define	ACL_DNS_FLAG_ALLOC	    	(1 << 0)	/* Async object is dynamically allocated */
#define	ACL_DNS_FLAG_CHECK_DNS_IP	(1 << 1)	/* Check whether DNS address matches */
#define	ACL_DNS_FLAG_CHECK_DNS_NET	(1 << 2)	/* Check whether DNS network matches */

	/* This function pointer is used to dynamically allocate memory for accessing addresses */
	ACL_EVENT_NOTIFY_TIME lookup_timeout;
} ACL_DNS;

typedef struct ACL_DNS_REQ ACL_DNS_REQ;

/**
 * Initialize DNS async query object structure.
 * @param dns {ACL_DNS*} DNS async query object
 * @param aio {ACL_AIO*} Async object
 * @param timeout {int} Timeout value for each DNS query
 * @return {int} Whether initialization succeeded, return 0 indicates success,
 * 	-1 indicates failure
 */
ACL_API int acl_dns_init(ACL_DNS *dns, ACL_AIO *aio, int timeout);

/**
 * Create a DNS async query object and initialize it.
 * @param aio {ACL_AIO*} Async object
 * @param timeout {int} Timeout value for each DNS query
 * @return {ACL_DNS*} DNS async query object, if NULL, indicates DNS query
 * 	object creation failed
 */
ACL_API ACL_DNS *acl_dns_create(ACL_AIO *aio, int timeout);

/**
 * Open DNS cache.
 * @param dns {ACL_DNS*} DNS async query object
 * @param limit {int} Maximum number of DNS query results to cache
 */
ACL_API void acl_dns_open_cache(ACL_DNS *dns, int limit);

/**
 * Add a DNS server address.
 * @param dns {ACL_DNS*} DNS async query object
 * @param dns_ip {const char*} DNS server IP address
 * @param dns_port {unsigned short} DNS server port
 * @param mask_length {int} DNS server's network mask length (0 < && < 32)
 */
ACL_API void acl_dns_add_dns(ACL_DNS *dns, const char *dns_ip,
	unsigned short dns_port, int mask_length);

/**
 * Clear DNS server address list.
 * @param dns {ACL_DNS*} DNS async query object
 */
ACL_API void acl_dns_clear_dns(ACL_DNS *dns);

/**
 * Get DNS server address list.
 * @param dns {ACL_DNS*} DNS async query object
 * @return {ACL_ARRAY*} Returns ACL_DNS_ADDR array, return value is never NULL
 */
ACL_API ACL_ARRAY *acl_dns_list(ACL_DNS *dns);

/**
 * Get DNS server address list size.
 * @param dns {ACL_DNS*} DNS async query object
 * @return {size_t}
 */
ACL_API size_t acl_dns_size(ACL_DNS *dns);

/**
 * Check whether DNS server address list is empty.
 * @param dns {ACL_DNS*} DNS async query object
 * @retrn {int} Return value, 0 indicates empty
 */
ACL_API int acl_dns_empty(ACL_DNS *dns);

/**
 * Delete a DNS server address.
 * @param dns {ACL_DNS*} DNS async query object
 * @param ip {const char*} DNS server IP address
 * @param port {unsigned short} DNS server port
 */
ACL_API void acl_dns_del_dns(ACL_DNS *dns, const char *ip, unsigned short port);

/**
 * Close async query object and free related resources.
 * @param dns {ACL_DNS*} DNS async query object
 */
ACL_API void acl_dns_close(ACL_DNS *dns);

/**
 * Set flag bit to check whether DNS source IP address is in the
 * same network segment as target address. If in the same network
 * segment, UDP packets are required. This is to prevent UDP
 * spoofing during DNS queries.
 * @param dns {ACL_DNS*} DNS async query object
 */
ACL_API void acl_dns_check_dns_ip(ACL_DNS *dns);

/**
 * Set flag bit to check whether DNS source IP network segment is
 * in the same network segment as target network segment. If in
 * the same network segment, UDP packets are required. This is to
 * prevent UDP spoofing during DNS queries.
 * @param dns {ACL_DNS*} DNS async query object
 */
ACL_API void acl_dns_check_dns_net(ACL_DNS *dns);

/**
 * Set DNS query timeout retry count.
 * @param dns {ACL_DNS*} DNS async query object
 * @param retry_limit {int} Retry count
 */
ACL_API void acl_dns_set_retry_limit(ACL_DNS *dns, int retry_limit);

/**
 * Async query a domain name's corresponding A record IP address list.
 * @param dns {ACL_DNS*} DNS async query object
 * @param domain {const char*} Domain name
 * @param callback {void (*)(ACL_DNS_DB*, void*, int, const
 *  ACL_RFC1035_MESSAGE*)} Callback function for query success or
 *  failure, if callback's ACL_DNS_DB is NULL, indicates query
 *  failed, second parameter is user-provided parameter, third
 *  parameter is error code when query fails
 * @param ctx {void*} One of callback's parameters
 */
ACL_API void acl_dns_lookup(ACL_DNS *dns, const char *domain,
	void (*callback)(ACL_DNS_DB*, void*, int, const ACL_RFC1035_MESSAGE*),
	void *ctx);

/**
 * Async query a domain name's corresponding record type IP address list.
 * @param dns {ACL_DNS*} DNS async query object
 * @param domain {const char*} Domain name
 * @param type {unsigned short} Query type, see ACL_RFC1035_TYPE_XXX in acl_rfc1035.h
 * @param callback {void (*)(ACL_DNS_DB*, void*, int, const
 *  ACL_RFC1035_MESSAGE*)} Callback function for query success or
 *  failure, if callback's ACL_DNS_DB is NULL, indicates query
 *  failed, second parameter is user-provided parameter, third
 *  parameter is error code when query fails
 * @param ctx {void*} One of callback's parameters
 */
ACL_API void acl_dns_lookup2(ACL_DNS *dns, const char *domain, unsigned short type,
	void (*callback)(ACL_DNS_DB*, void*, int, const ACL_RFC1035_MESSAGE*),
	void *ctx);

/**
 * Add static mapping information to DNS query result.
 * @param dns {ACL_DNS*} DNS async query object
 * @param domain {const char*} Domain name
 * @param ip_list {const char*} IP address list, separated by
 *  ';', e.g.: 192.168.0.1;192.168.0.2
 */
ACL_API void acl_dns_add_host(ACL_DNS *dns, const char *domain, const char *ip_list);

/**
 * Add query group information to DNS query result.
 * @param dns {ACL_DNS*} DNS async query object
 * @param group {const char*} Group domain name, e.g.: .test.com,
 *  then a.test.com, b.test.com all belong to .test.com group
 * @param ip_list {const char*} If not NULL, use static format
 *  to specify IP address list
 * @param refer {const char*} Default DNS server address, if this
 *  parameter is used, queries for this group will go to DNS query
 * @param excepts {ACL_ARGV*} Even though these domain names
 *  belong to group, they are excluded from group members
 */
ACL_API void acl_dns_add_group(ACL_DNS *dns, const char *group, const char *refer,
		const char *ip_list, const char *excepts);
/**
 * Cancel a query request.
 * @param handle {ACL_DNS_REQ*} A certain query request
 */
ACL_API void acl_dns_cancel(ACL_DNS_REQ *handle);

/**
 * Get error message string based on error number.
 * @param errnum {int} Error code returned during DNS query
 * @return {const char*} Error message string
 */
ACL_API const char *acl_dns_serror(int errnum);

#ifdef	__cplusplus
}
#endif

#endif
