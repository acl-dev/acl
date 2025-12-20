#ifndef	ACL_NETDB_INCLUDE_H
#define	ACL_NETDB_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <netinet/in.h>
#endif

#include "../stdlib/acl_array.h"
#include "acl_sane_inet.h"

/**
 * Host address structure.
 */
typedef struct ACL_HOSTNAME ACL_HOST_INFO;
typedef struct ACL_HOSTNAME {
	char  ip[256];                  /**< hold ip or cname of the HOST */
	ACL_SOCKADDR saddr;		/**< ip addr in ACL_SOCKADDR */
	unsigned int ttl;               /**< the HOST's ip timeout(second) */
	int   priority;			/**< the priority of mx record */
	int   hport;
	unsigned int nrefer;            /**< refer number to this HOST */
	unsigned int type;		/**< the content type in ip buf */
#define	ACL_HOSTNAME_TYPE_IPV4		0
#define	ACL_HOSTNAME_TYPE_IPV6		1
#define	ACL_HOSTNAME_TYPE_CNAME		2
#define ACL_HOSTNAME_TYPE_MX		3
#define ACL_HOSTNAME_TYPE_SOA		4
#define ACL_HOSTNAME_TYPE_NS		5
#define ACL_HOSTNAME_TYPE_TXT		6
} ACL_HOSTNAME;

/**
 * DNS query result structure.
 */
typedef struct ACL_DNS_DB {
	ACL_ARRAY *h_db;
	int   size;
	char  name[256];
	const ACL_HOSTNAME *refer;	/**< refer to the cname node in h_db */
	ACL_SOCKADDR ns_addr;

	/* for acl_iterator */

	/* Get iterator head pointer */
	const ACL_HOSTNAME *(*iter_head)(ACL_ITER*, struct ACL_DNS_DB*);
	/* Get next iterator pointer */
	const ACL_HOSTNAME *(*iter_next)(ACL_ITER*, struct ACL_DNS_DB*);
	/* Get iterator tail pointer */
	const ACL_HOSTNAME *(*iter_tail)(ACL_ITER*, struct ACL_DNS_DB*);
	/* Get previous iterator pointer */
	const ACL_HOSTNAME *(*iter_prev)(ACL_ITER*, struct ACL_DNS_DB*);
	/* Get the current iterator's member structure object */
	const ACL_HOSTNAME *(*iter_info)(ACL_ITER*, struct ACL_DNS_DB*);
} ACL_DNS_DB;

/* in acl_netdb.c */

/**
 * Get host address structure at a certain index position from result structure.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @param i {int} Index position
 * @return {const ACL_HOSTNAME*} Host address structure
 *  corresponding to the index
 */
ACL_API const ACL_HOSTNAME *acl_netdb_index(const ACL_DNS_DB *h_dns_db, int i);

/**
 * Get IP address at a certain index position from result structure.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @param i {int} Index position
 * @return {const ACL_SOCKADDR*} IP address structure, NULL indicates failure
 */
ACL_API const ACL_SOCKADDR *acl_netdb_index_saddr(ACL_DNS_DB *h_dns_db, int i);

/**
 * Operate on reference count of host address structure
 * corresponding to a certain index in result structure.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @param i {int} Index position
 * @param n {int} Reference value to add
 */
ACL_API void acl_netdb_refer_oper(ACL_DNS_DB *h_dns_db, int i, int n);

/**
 * Increment reference count of host address structure
 * corresponding to a certain index in result structure by 1.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @param i {int} Index position
 */
ACL_API void acl_netdb_refer(ACL_DNS_DB *h_dns_db, int i);

/**
 * Decrement reference count of host address structure
 * corresponding to a certain index in result structure by 1.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @param i {int} Index position
 */
ACL_API void acl_netdb_unrefer(ACL_DNS_DB *h_dns_db, int i);

/**
 * Get IP address string corresponding to a certain index in result structure.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @param i {int} Index position
 * @return {const char*} Obtained result, NULL indicates failure
 */
ACL_API const char *acl_netdb_index_ip(const ACL_DNS_DB *h_dns_db, int i);

/**
 * Get the number of host addresses in result structure.
 * @param h_dns_db {const ACL_DNS_DB*} DNS result structure
 * @return {int} Number of host addresses > 0, -1 indicates invalid result structure
 */
ACL_API int acl_netdb_size(const ACL_DNS_DB *h_dns_db);

/**
 *  Free result structure memory resources.
 * @param h_dns_db {ACL_DNS_DB*} DNS result structure
 */
ACL_API void acl_netdb_free(ACL_DNS_DB *h_dns_db);

/**
 * Create a new query result structure, used for DNS queries.
 * @param domain {const char*} Domain name to query
 * @return {ACL_DNS_DB*} Created result structure
 */
ACL_API ACL_DNS_DB *acl_netdb_new(const char *domain);

/**
 * Set DNS query result structure's bound DNS server address.
 * @param db {ACL_DNS_DB*} From acl_netdb_new or acl_netdb_clone
 * @param sa {ACL_SOCKADDR*} DNS server address
 */
ACL_API void acl_netdb_set_ns(ACL_DNS_DB *db, ACL_SOCKADDR *sa);

/**
 * Add an IP address to result structure.
 * @param h_dns_db {ACL_DNS_DB*} Query result structure
 * @param ip {const char*} IP address to add
 */
ACL_API void acl_netdb_addip(ACL_DNS_DB *h_dns_db, const char *ip);

/**
 * Add an IP address and port to result structure.
 * @param h_dns_db {ACL_DNS_DB*} Query result structure
 * @param ip {const char*} IP address to add
 * @param port {int} Port to add
 */
ACL_API void acl_netdb_add_addr(ACL_DNS_DB *h_dns_db, const char *ip, int port);

/**
 * Clone a query result structure.
 * @param h_dns_db {const ACL_DNS_DB*} Source result structure
 * @return {ACL_DNS_DB*} Newly cloned result structure
 */
ACL_API ACL_DNS_DB *acl_netdb_clone(const ACL_DNS_DB *h_dns_db);

/**
 * Query IP address list for a certain domain name.
 * @param name {const char*} Domain name
 * @param h_error {int*} If query fails, stores error reason
 * @return {ACL_DNS_DB*} Query result, if NULL, query failed.
 *  Additionally, even if return is not empty, you still need to
 *  get the result array length via acl_netdb_size()
 */
ACL_API ACL_DNS_DB *acl_gethostbyname(const char *name, int *h_error);

/**
 * Query IP address list for a certain domain name.
 * @param name {const char*} Domain name
 * @param socktype {int} Socket type used for query connection:
 *  SOCK_DGRAM -- UDP mode, SOCK_STREAM -- TCP mode
 * @param family {int} IP address family type: PF_INET -- IPV4,
 *  PF_INET6 -- IPV6, PF_UNSPEC -- let system automatically select
 * @param h_error {int*} If query fails, stores error reason
 * @return {ACL_DNS_DB*} Query result, if NULL, query failed.
 *  Additionally, even if return is not empty, you still need to
 *  get the result array length via acl_netdb_size()
 */
ACL_API ACL_DNS_DB *acl_gethostbyname2(const char *name, int socktype,
		int family, int *h_error);

/**
 * Get error message string based on error number.
 * @param errnum {int} Error number
 * @return {const char*} Error message
 */ 
ACL_API const char *acl_netdb_strerror(int errnum);

/* in acl_netdb_cache.c */
/**
 * Push DNS query result structure to cache.
 * @param h_dns_db {const ACL_DNS_DB*} DNS query result structure
 * @param timeout {int} Timeout for this result structure in
 *  cache, if <= 0, uses default value. Default value is the
 *  timeout value in acl_netdb_cache_init(), unit is seconds
 */
ACL_API void acl_netdb_cache_push(const ACL_DNS_DB *h_dns_db, int timeout);

/**
 * Get DNS query result structure from DNS cache.
 * @param name {const char*} Domain name
 * @return {ACL_DNS_DB*} DNS query result structure
 */
ACL_API ACL_DNS_DB *acl_netdb_cache_lookup(const char *name);

/**
 * Delete a DNS query result structure from DNS cache.
 * @param name {const char*} Domain name
 */
ACL_API void acl_netdb_cache_del_host(const char *name);

/**
 * Initialize DNS cache.
 * @param timeout {int} Default cache timeout for DNS cache (seconds)
 * @param thread_safe {int} Whether DNS cache needs to be
 *  thread-safe, 0: indicates not needed, 1: indicates
 *  thread-safe needed
 */
ACL_API void acl_netdb_cache_init(int timeout, int thread_safe);

#endif
