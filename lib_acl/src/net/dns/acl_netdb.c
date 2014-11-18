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
#include "net/acl_netdb.h"

#endif

const ACL_HOSTNAME *acl_netdb_index(const ACL_DNS_DB *h_dns_db, int n)
{
	const char *myname = "acl_netdb_index";
	ACL_HOSTNAME *h_hostname;

	if (h_dns_db == NULL || n < 0) {
		acl_msg_error("%s, %s(%d): input error",
			__FILE__, myname, __LINE__);
		return (NULL);
	}

	if (h_dns_db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
			__FILE__, myname, __LINE__);
		return (NULL);
	}
	if (n >= h_dns_db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
			__FILE__, myname, __LINE__, n, h_dns_db->size);
		return (NULL);
	}

	h_hostname = (ACL_HOSTNAME *) acl_array_index(h_dns_db->h_db, n);
	return (h_hostname);
}

const struct sockaddr_in *acl_netdb_index_saddr(ACL_DNS_DB *h_dns_db, int n)
{
	const char *myname= "acl_netdb_index_saddr";
	ACL_HOSTNAME *h_hostname;

	if (h_dns_db == NULL || n < 0) {
		acl_msg_error("%s, %s(%d): input error",
				__FILE__, myname, __LINE__);
		return (NULL);
	}
	if (h_dns_db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
				__FILE__, myname, __LINE__);
		return (NULL);
	}
	if (n >= h_dns_db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
				__FILE__, myname, __LINE__, n, h_dns_db->size);
		return (NULL);
	}

	h_hostname = (ACL_HOSTNAME *) acl_array_index(h_dns_db->h_db, n);
	return (&h_hostname->saddr);
}

const char *acl_netdb_index_ip(const ACL_DNS_DB *h_dns_db, int n)
{
	const char *myname = "acl_netdb_index_ip";
	ACL_HOSTNAME *h_hostname;

	if (h_dns_db == NULL || n < 0) {
		acl_msg_error("%s, %s(%d): input error",
				__FILE__, myname, __LINE__);
		return (NULL);
	}
	if (h_dns_db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
				__FILE__, myname, __LINE__);
		return (NULL);
	}
	if (n >= h_dns_db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
				__FILE__, myname, __LINE__, n, h_dns_db->size);
		return (NULL);
	}
	h_hostname = (ACL_HOSTNAME *) acl_array_index(h_dns_db->h_db, n);
	return (h_hostname->ip);
}

void acl_netdb_refer_oper(ACL_DNS_DB *h_dns_db, int idx, int value)
{
	const char *myname = "acl_netdb_refer_oper";
	ACL_HOSTNAME *h_hostname;

	if (h_dns_db == NULL || idx < 0) {
		acl_msg_error("%s, %s(%d): input error",
			__FILE__, myname, __LINE__);
		return;
	}

	if (h_dns_db->size == 0) {
		acl_msg_error("%s, %s(%d): dns db size is 0",
			__FILE__, myname, __LINE__);
		return;
	}
	if (idx >= h_dns_db->size) {
		acl_msg_error("%s, %s(%d): index(%d) > size(%d)",
			__FILE__, myname, __LINE__, idx, h_dns_db->size);
		return;
	}
	h_hostname = (ACL_HOSTNAME *) acl_array_index(h_dns_db->h_db, idx);

	h_hostname->nrefer += value;
}

void acl_netdb_refer(ACL_DNS_DB *h_dns_db, int idx)
{
	acl_netdb_refer_oper(h_dns_db, idx, 1);
}

void acl_netdb_unrefer(ACL_DNS_DB *h_dns_db, int idx)
{
	acl_netdb_refer_oper(h_dns_db, idx, -1);
}

int acl_netdb_size(const ACL_DNS_DB *h_dns_db)
{
	const char *myname = "acl_netdb_size";

	if (h_dns_db == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
				__FILE__, myname, __LINE__);
		return (-1);
	}

	return (h_dns_db->size);
}

static void free_dns_db(void *ctx)
{
	acl_myfree(ctx);
}

void acl_netdb_free(ACL_DNS_DB *h_dns_db)
{
	if (h_dns_db == NULL)
		return;
	if (h_dns_db->h_db)
		acl_array_destroy(h_dns_db->h_db, free_dns_db);
	acl_myfree(h_dns_db);
}

static const ACL_HOST_INFO *netdb_iter_head(ACL_ITER *iter, struct ACL_DNS_DB *dns_db)
{
	return ((const ACL_HOST_INFO*) dns_db->h_db->iter_head(iter, dns_db->h_db));
}

static const ACL_HOST_INFO *netdb_iter_next(ACL_ITER *iter, struct ACL_DNS_DB *dns_db)
{
	return ((const ACL_HOST_INFO*) dns_db->h_db->iter_next(iter, dns_db->h_db));
}

static const ACL_HOST_INFO *netdb_iter_tail(ACL_ITER *iter, struct ACL_DNS_DB *dns_db)
{
	return ((const ACL_HOST_INFO*) dns_db->h_db->iter_tail(iter, dns_db->h_db));
}

static const ACL_HOST_INFO *netdb_iter_prev(ACL_ITER *iter, struct ACL_DNS_DB *dns_db)
{
	return ((const ACL_HOST_INFO*) dns_db->h_db->iter_prev(iter, dns_db->h_db));
}

static const ACL_HOST_INFO *netdb_iter_info(ACL_ITER *iter, struct ACL_DNS_DB *dns_db acl_unused)
{
	return (iter->ptr ? (ACL_HOST_INFO*) iter->ptr : NULL);
}

ACL_DNS_DB *acl_netdb_new(const char *domain)
{
	const char *myname = "acl_netdb_new";
	ACL_DNS_DB *dns_db;
	char  buf[256];

	dns_db = acl_mycalloc(1, sizeof(ACL_DNS_DB));
	if (dns_db == NULL) {
		acl_msg_error("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__,
			acl_last_strerror(buf, sizeof(buf)));
		return (NULL);
	}

	dns_db->h_db = acl_array_create(5);
	if (dns_db->h_db == NULL) {
		acl_msg_error("%s, %s(%d): create array error(%s)",
			__FILE__, myname, __LINE__,
			acl_last_strerror(buf, sizeof(buf)));
		acl_myfree(dns_db);
		return (NULL);
	}

	snprintf(dns_db->name, sizeof(dns_db->name), "%s", domain);
	acl_lowercase(dns_db->name);

	dns_db->iter_head = netdb_iter_head;
	dns_db->iter_next = netdb_iter_next;
	dns_db->iter_tail = netdb_iter_tail;
	dns_db->iter_prev = netdb_iter_prev;
	dns_db->iter_info = netdb_iter_info;

	return (dns_db);
}

void acl_netdb_addip(ACL_DNS_DB *dns_db, const char *ip)
{
	acl_netdb_add_addr(dns_db, ip, 0);
}

void acl_netdb_add_addr(ACL_DNS_DB *dns_db, const char *ip, int hport)
{
	const char *myname = "acl_netdb_add_addr";
	ACL_HOSTNAME *phost;
	char  buf[256];

	if (dns_db == NULL || dns_db->h_db == NULL || ip == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return;
	}

	phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
	if (phost == NULL) {
		acl_msg_error("%s(%d): calloc error(%s)",
			myname, __LINE__, acl_last_strerror(buf, sizeof(buf)));
		return;
	}

	memset(&phost->saddr, 0, sizeof(phost->saddr));
	ACL_SAFE_STRNCPY(phost->ip, ip, sizeof(phost->ip));
	phost->saddr.sin_addr.s_addr = (unsigned long) inet_addr(ip);
	phost->hport = hport;

	if (acl_array_append(dns_db->h_db, phost) < 0) {
		acl_msg_error("%s(%d): array append error(%s)",
			myname, __LINE__, acl_last_strerror(buf, sizeof(buf)));
		return;
	}

	dns_db->size++;
}

ACL_DNS_DB *acl_netdb_clone(const ACL_DNS_DB *h_dns_db)
{
	const char *myname = "acl_netdb_clone";
	char  buf[256];
	ACL_DNS_DB *dns_db;
	ACL_HOSTNAME *phost, *h_host;
	int  i, n;

	if (h_dns_db == NULL || h_dns_db->h_db == NULL)
		return (NULL);

	n = acl_array_size(h_dns_db->h_db);
	if (n <= 0) {
		acl_msg_error("%s, %s(%d): h_db's size(%d) <= 0",
				__FILE__, myname, __LINE__, n);
		return (NULL);
	}

	dns_db = acl_netdb_new(h_dns_db->name);
	if (dns_db == NULL) {
		acl_msg_error("%s, %s(%d): acl_netdb_new error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
		return (NULL);
	}

	for (i = 0; i < n; i++) {
		phost = (ACL_HOSTNAME *) acl_array_index(h_dns_db->h_db, i);
		acl_assert(phost);

		h_host = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
		if (h_host == NULL) {
			acl_msg_error("%s, %s(%d): calloc error(%s)",
					__FILE__, myname, __LINE__,
					acl_last_strerror(buf, sizeof(buf)));
			acl_netdb_free(dns_db);
			return (NULL);
		}

		h_host->saddr.sin_addr.s_addr = phost->saddr.sin_addr.s_addr;
		ACL_SAFE_STRNCPY(h_host->ip, phost->ip, sizeof(h_host->ip));
		h_host->hport = phost->hport;

		if (acl_array_append(dns_db->h_db, h_host) < 0) {
			acl_msg_error("%s, %s(%d): array append error(%s)",
					__FILE__, myname, __LINE__,
					acl_last_strerror(buf, sizeof(buf)));
			acl_netdb_free(dns_db);
			return (NULL);
		}       

		dns_db->size++;
	}

	return (dns_db);
}

ACL_DNS_DB *acl_gethostbyname(const char *name, int *h_error)
{
	const char *myname = "acl_gethostbyname";
	ACL_DNS_DB *h_dns_db = NULL;
	ACL_HOSTNAME *h_host;
/* #ifndef	SUNOS5 */
	struct hostent *h_addrp = NULL;
/* #endif */

#ifdef	ACL_UNIX
# ifndef ACL_MACOSX
	struct hostent  h_buf;
	int   errnum = 0;
# endif
#endif
	char **pptr, buf[4096];
	int   n;

#undef	ERETURN
#define	ERETURN(_x_) do {  \
	if (h_dns_db)  \
		acl_netdb_free(h_dns_db);  \
	return (_x_);  \
} while (0)

	if (name == NULL) {
		acl_msg_error("%s, %s(%d): input error",
				__FILE__, myname, __LINE__);
		ERETURN (NULL);
	}

	if (h_error)
		*h_error = 0;
	
	/* lookup the local dns cache first */
	h_dns_db = acl_netdb_cache_lookup(name);
	if (h_dns_db)
		return (h_dns_db);

	h_dns_db = acl_netdb_new(name);
	if (h_dns_db == NULL) {
		acl_msg_error("%s, %s(%d): acl_netdb_new error(%s)",
			__FILE__, myname, __LINE__,
			acl_last_strerror(buf, sizeof(buf)));
		return (NULL);
	}

	if (acl_is_ip(name) == 0) {
		h_host = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
		if (h_host == NULL) {
			acl_msg_error("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
			ERETURN (NULL);
		}
		h_host->saddr.sin_addr.s_addr = inet_addr(name);
		ACL_SAFE_STRNCPY(h_host->ip, name, sizeof(h_host->ip));

		if (acl_array_append(h_dns_db->h_db, h_host) < 0) {
			acl_msg_error("%s, %s(%d): array append error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
			ERETURN (NULL);
		}

		h_dns_db->size++;

		return (h_dns_db);
	}

	memset(buf, 0, sizeof(buf));

	h_addrp = NULL;

#if	defined(WIN32) || defined(ACL_MACOSX)
	h_addrp = gethostbyname(name);
	if (h_addrp == NULL) {
		acl_msg_error("%s, %s(%d): gethostbyname error(%s), addr=%s",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)), name);
		ERETURN (NULL);
	}

#elif	defined(ACL_UNIX)
	memset(&h_buf, 0, sizeof(h_buf));
# if	defined(LINUX2) || defined(ACL_FREEBSD)
	n = gethostbyname_r(name, &h_buf, buf, sizeof(buf), &h_addrp, &errnum);
	if (n) {
		if (h_error)
			*h_error = errnum;
		ERETURN (NULL);
	}
# elif	defined(SUNOS5)
	h_addrp = gethostbyname_r(name, &h_buf, buf, sizeof(buf), &errnum);
	if (h_addrp == NULL) {
		if (h_error)
			*h_error = errnum;
		ERETURN (NULL);
	}
# else
#  error "unknown OS type"
# endif
#else
# error "unknown OS type"
#endif

	if (h_addrp == NULL || h_addrp->h_addr_list == NULL) {
		acl_msg_error("%s, %s(%d): null result return(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
		ERETURN (NULL);
	}

	for (pptr = h_addrp->h_addr_list; *pptr != NULL; pptr++) {
		h_host = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
		if (h_host == NULL) {
			acl_msg_error("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
			ERETURN (NULL);
		}

		memset(&h_host->saddr, 0, sizeof(h_host->saddr));
		n = (int) sizeof(h_host->saddr.sin_addr) > h_addrp->h_length
			? h_addrp->h_length : (int) sizeof(h_host->saddr.sin_addr);
		memcpy(&h_host->saddr.sin_addr, *pptr, n);
		/* bugifx: 2009.12.8
		 * this is not thread safe
		 * ACL_SAFE_STRNCPY(h_host->ip, inet_ntoa(h_host->saddr.sin_addr), sizeof(h_host->ip));
		 */
		acl_inet_ntoa(h_host->saddr.sin_addr, h_host->ip, sizeof(h_host->ip));

		if (acl_array_append(h_dns_db->h_db, h_host) < 0) {
			acl_msg_error("%s, %s(%d): array append error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
			ERETURN (NULL);
		}

		h_dns_db->size++;
	}

	acl_netdb_cache_push(h_dns_db, 0);

	return (h_dns_db);
}

typedef struct ACL_DNS_ERROR {
	int   h_error;
	const char *info;
} ACL_DNS_ERROR;

static ACL_DNS_ERROR __dns_errlist[] = {
#ifdef	LINUX2
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
			return (__dns_errlist[i].info);

	return ("unknown error number");
}

