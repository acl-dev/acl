#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <ctype.h>
#include <stdio.h>
#ifdef	ACL_UNIX
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* extern int h_errno; */
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_array.h"
#include "net/acl_sane_inet.h"
#include "net/acl_sane_socket.h"
#include "net/acl_valid_hostname.h"
#include "net/acl_host_port.h"
#include "net/acl_netdb.h"

#endif

const ACL_HOSTNAME *acl_netdb_index(const ACL_DNS_DB *db, int n)
{
	ACL_HOSTNAME *h_hostname;

	if (db == NULL || n < 0) {
		acl_msg_error("%s, %s(%d): input error",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}

	if (db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}
	if (n >= db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
			__FILE__, __FUNCTION__, __LINE__, n, db->size);
		return NULL;
	}

	h_hostname = (ACL_HOSTNAME *) acl_array_index(db->h_db, n);
	return h_hostname;
}

const ACL_SOCKADDR *acl_netdb_index_saddr(ACL_DNS_DB *db, int n)
{
	ACL_HOSTNAME *h_hostname;

	if (db == NULL || n < 0) {
		acl_msg_error("%s, %s(%d): input error",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}
	if (db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}
	if (n >= db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
			__FILE__, __FUNCTION__, __LINE__, n, db->size);
		return NULL;
	}

	h_hostname = (ACL_HOSTNAME *) acl_array_index(db->h_db, n);
	return &h_hostname->saddr;
}

const char *acl_netdb_index_ip(const ACL_DNS_DB *db, int n)
{
	ACL_HOSTNAME *h_hostname;

	if (db == NULL || n < 0) {
		acl_msg_error("%s, %s(%d): input error",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}
	if (db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}
	if (n >= db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
			__FILE__, __FUNCTION__, __LINE__, n, db->size);
		return NULL;
	}
	h_hostname = (ACL_HOSTNAME *) acl_array_index(db->h_db, n);
	return h_hostname->ip;
}

void acl_netdb_refer_oper(ACL_DNS_DB *db, int idx, int value)
{
	ACL_HOSTNAME *h_hostname;

	if (db == NULL || idx < 0) {
		acl_msg_error("%s, %s(%d): input error",
			__FILE__, __FUNCTION__, __LINE__);
		return;
	}

	if (db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
			__FILE__, __FUNCTION__, __LINE__);
		return;
	}
	if (idx >= db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
			__FILE__, __FUNCTION__, __LINE__, idx, db->size);
		return;
	}
	h_hostname = (ACL_HOSTNAME *) acl_array_index(db->h_db, idx);
	h_hostname->nrefer += value;
}

void acl_netdb_refer(ACL_DNS_DB *db, int idx)
{
	acl_netdb_refer_oper(db, idx, 1);
}

void acl_netdb_unrefer(ACL_DNS_DB *db, int idx)
{
	acl_netdb_refer_oper(db, idx, -1);
}

int acl_netdb_size(const ACL_DNS_DB *db)
{
	if (db == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			__FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	return db->size;
}

static void free_dns_db(void *ctx)
{
	acl_myfree(ctx);
}

void acl_netdb_free(ACL_DNS_DB *db)
{
	if (db == NULL)
		return;
	if (db->h_db)
		acl_array_destroy(db->h_db, free_dns_db);
	acl_myfree(db);
}

static const ACL_HOST_INFO *netdb_iter_head(ACL_ITER *iter, struct ACL_DNS_DB *db)
{
	return (const ACL_HOST_INFO*) db->h_db->iter_head(iter, db->h_db);
}

static const ACL_HOST_INFO *netdb_iter_next(ACL_ITER *iter, struct ACL_DNS_DB *db)
{
	return (const ACL_HOST_INFO*) db->h_db->iter_next(iter, db->h_db);
}

static const ACL_HOST_INFO *netdb_iter_tail(ACL_ITER *iter, struct ACL_DNS_DB *db)
{
	return (const ACL_HOST_INFO*) db->h_db->iter_tail(iter, db->h_db);
}

static const ACL_HOST_INFO *netdb_iter_prev(ACL_ITER *iter, struct ACL_DNS_DB *db)
{
	return (const ACL_HOST_INFO*) db->h_db->iter_prev(iter, db->h_db);
}

static const ACL_HOST_INFO *netdb_iter_info(ACL_ITER *iter,
	struct ACL_DNS_DB *db acl_unused)
{
	return iter->ptr ? (ACL_HOST_INFO*) iter->ptr : NULL;
}

void acl_netdb_set_ns(ACL_DNS_DB *db, ACL_SOCKADDR *sa)
{
	memcpy(&db->ns_addr, sa, sizeof(db->ns_addr));
}

ACL_DNS_DB *acl_netdb_new(const char *domain)
{
	ACL_DNS_DB *db;

	db = acl_mycalloc(1, sizeof(ACL_DNS_DB));
	db->h_db = acl_array_create(5);

	snprintf(db->name, sizeof(db->name), "%s", domain);
	acl_lowercase(db->name);

	db->iter_head = netdb_iter_head;
	db->iter_next = netdb_iter_next;
	db->iter_tail = netdb_iter_tail;
	db->iter_prev = netdb_iter_prev;
	db->iter_info = netdb_iter_info;

	return db;
}

void acl_netdb_addip(ACL_DNS_DB *db, const char *ip)
{
	acl_netdb_add_addr(db, ip, 0);
}

static int ip2addr(const char *ip, unsigned short port, ACL_SOCKADDR *saddr)
{
	memset(saddr, 0, sizeof(ACL_SOCKADDR));

	if (acl_valid_ipv4_hostaddr(ip, 0)) {
		saddr->sa.sa_family = AF_INET;
		saddr->in.sin_port  = htons(port);

		if (inet_pton(AF_INET, ip, &saddr->in.sin_addr) == 1) {
			return 1;
		}
		acl_msg_error("%s(%d): invalid ip=%s",
			__FUNCTION__, __LINE__, ip);
	}
#ifdef AF_INET6
	else if (acl_valid_ipv6_hostaddr(ip, 0)) {
		saddr->sa.sa_family  = AF_INET6;
		saddr->in6.sin6_port = htons(port);
		if (inet_pton(AF_INET6, ip, &saddr->in6.sin6_addr) == 1) {
			return 1;
		}
		acl_msg_error("%s(%d): invalid ip=%s",
			__FUNCTION__, __LINE__, ip);
	}
#endif

	return 0;
}

void acl_netdb_add_addr(ACL_DNS_DB *db, const char *ip, int hport)
{
	ACL_HOSTNAME *phost;
	ACL_SOCKADDR  saddr;

	if (db == NULL || db->h_db == NULL || ip == NULL) {
		acl_msg_error("%s(%d): input invalid", __FUNCTION__, __LINE__);
		return;
	}

	memset(&saddr, 0, sizeof(saddr));

	if (ip2addr(ip, 0, &saddr) == 0) {
		acl_msg_error("%s(%d): invalid ip=%s",
			__FUNCTION__, __LINE__, ip);
		return;
	}

	phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
	memcpy(&phost->saddr, &saddr, sizeof(phost->saddr));
	ACL_SAFE_STRNCPY(phost->ip, ip, sizeof(phost->ip));
	phost->hport = hport;

	(void) acl_array_append(db->h_db, phost);
	db->size++;
}

ACL_DNS_DB *acl_netdb_clone(const ACL_DNS_DB *db)
{
	ACL_DNS_DB *dbp;
	ACL_HOSTNAME *phost, *h_host;
	int  i, n;

	if (db == NULL || db->h_db == NULL)
		return NULL;

	n = acl_array_size(db->h_db);
	if (n <= 0) {
		acl_msg_error("%s, %s(%d): h_db's size(%d) <= 0",
			__FILE__, __FUNCTION__, __LINE__, n);
		return NULL;
	}

	dbp = acl_netdb_new(db->name);
	memcpy(&dbp->ns_addr, &db->ns_addr, sizeof(db->ns_addr));

	for (i = 0; i < n; i++) {
		phost = (ACL_HOSTNAME *) acl_array_index(db->h_db, i);
		acl_assert(phost);

		h_host = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
		memcpy(&h_host->saddr, &phost->saddr, sizeof(h_host->saddr));
		ACL_SAFE_STRNCPY(h_host->ip, phost->ip, sizeof(h_host->ip));
		h_host->hport = phost->hport;

		(void) acl_array_append(dbp->h_db, h_host);
		dbp->size++;
	}

	return dbp;
}

ACL_DNS_DB *acl_gethostbyname(const char *name, int *h_error)
{
	ACL_DNS_DB *db;
	ACL_SOCKADDR saddr;
	struct addrinfo *res0, *res;

	if (h_error)
		*h_error = 0;
	
	/* lookup the local dns cache first */
	db = acl_netdb_cache_lookup(name);
	if (db) {
		return db;
	}

	db = acl_netdb_new(name);

	if (ip2addr(name, 0, &saddr)) {
		ACL_HOSTNAME *h_host = acl_mycalloc(1, sizeof(ACL_HOSTNAME));

		memcpy(&h_host->saddr, &saddr, sizeof(h_host->saddr));
		ACL_SAFE_STRNCPY(h_host->ip, name, sizeof(h_host->ip));
		(void) acl_array_append(db->h_db, h_host);
		db->size++;
		return db;
	}

	res0 = acl_host_addrinfo(name, SOCK_DGRAM);
	if (res0 == NULL) {
		acl_netdb_free(db);
		return NULL;
	}

	for (res = res0; res != NULL; res = res->ai_next) {
		ACL_SOCKADDR *sa = (ACL_SOCKADDR *) res->ai_addr;
		ACL_HOSTNAME *h_host;
		char ip[64];

		memset(&saddr, 0, sizeof(saddr));
		saddr.sa.sa_family = res->ai_family;
		if (res->ai_family == AF_INET) {
			if (inet_ntop(res->ai_family, &sa->in.sin_addr,
				ip, sizeof(ip)) == NULL) {

				continue;
			}
			memcpy(&saddr.in.sin_addr, &sa->in.sin_addr,
				sizeof(saddr.in.sin_addr));
#ifdef AF_INET6
		} else if (res->ai_family == AF_INET6) {
			if (inet_ntop(res->ai_family, &sa->in6.sin6_addr,
				ip, sizeof(ip)) == NULL) {

				continue;
			}
			memcpy(&saddr.in6.sin6_addr, &sa->in6.sin6_addr,
				sizeof(saddr.in6.sin6_addr));
#endif
		} else {
			continue;
		}

		h_host = (ACL_HOSTNAME*) acl_mycalloc(1, sizeof(ACL_HOSTNAME));
		memcpy(&h_host->saddr, &saddr, sizeof(h_host->saddr));
		ACL_SAFE_STRNCPY(h_host->ip, ip, sizeof(h_host->ip));
		h_host->hport = 0;

		(void) acl_array_append(db->h_db, h_host);
		db->size++;
	}

	freeaddrinfo(res0);

	if (acl_netdb_size(db) > 0) {
		return db;
	}

	acl_netdb_free(db);
	return NULL;
}

typedef struct ACL_DNS_ERROR {
	int   h_error;
	const char *info;
} ACL_DNS_ERROR;

static ACL_DNS_ERROR __dns_errlist[] = {
#ifdef	ACL_LINUX
	{ HOST_NOT_FOUND, "The specified host is unknown" },
	{ TRY_AGAIN, "A temporary error occurred on an authoritative name server.  Try again later." },
	{ NO_RECOVERY, "A non-recoverable name server error occurred" },
	{ NO_DATA, "The requested name is valid but does not have an IP address" },
#endif
	{ -1, "unknown error" },
};

const char *acl_netdb_strerror(int errnum)
{
	int  i, n;

	n = sizeof(__dns_errlist) / sizeof(ACL_DNS_ERROR);

	for (i = 0; i < n; i++)
		if (__dns_errlist[i].h_error == errnum)
			return __dns_errlist[i].info;

	return "unknown error number";
}
