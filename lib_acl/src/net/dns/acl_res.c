#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "net/acl_vstream_net.h"
#include "net/acl_sane_inet.h"
#include "net/acl_rfc1035.h"
#include "net/acl_res.h"

#endif

static int __conn_timeout = 10;
static int __rw_timeout   = 10;

void acl_res_set_timeout(int conn_timeout, int rw_timeout)
{
	__conn_timeout = conn_timeout;
	__rw_timeout   = rw_timeout;
}

ACL_RES *acl_res_new(const char *dns_ip, unsigned short dns_port)
{
	ACL_RES *res;

	if (dns_ip == NULL || *dns_ip == 0) {
		acl_msg_error("%s(%d), %s: dns_ip invalid",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	if (dns_port <= 0) {
		dns_port = 53;
	}

	res = acl_mycalloc(1, sizeof(ACL_RES));
	res->cur_qid = (unsigned short) time(NULL);

	ACL_SAFE_STRNCPY(res->dns_ip, dns_ip, sizeof(res->dns_ip));
	res->dns_port = dns_port;

	res->conn_timeout = __conn_timeout;
	res->rw_timeout   = __rw_timeout;
	res->transfer     = ACL_RES_USE_UDP;

	return res;
}

void acl_res_free(ACL_RES *res)
{
	if (res) {
		acl_myfree(res);
	}
}

static int udp_res_lookup(ACL_RES *res, const char *data, int dlen,
	char *buf, int size)
{
	ssize_t    ret;
	ACL_SOCKET fd;
	struct sockaddr_in addr;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: socket create error=%s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_port        = htons(res->dns_port);
	addr.sin_addr.s_addr = inet_addr(res->dns_ip);

	ret = sendto(fd, data, dlen, 0, (struct sockaddr *) &addr,
			(socklen_t) sizeof(addr));
	if (ret < 0) {
		acl_socket_close(fd);
		res->errnum = ACL_RES_ERR_SEND;
		return -1;
	}

	ret = acl_read_wait(fd, res->rw_timeout);
	if (ret < 0) {
		acl_socket_close(fd);
		res->errnum = ACL_RES_ERR_RTMO;
		return -1;
	}

	ret = recv(fd, buf, size, 0);
	acl_socket_close(fd);

	if (ret <= 0) {
		res->errnum = ACL_RES_ERR_SEND;
		return -1;
	}

	return (int) ret;
}

static int tcp_res_lookup(ACL_RES *res, const char *data,
	int dlen, char *buf, int size)
{
	ACL_VSTREAM *stream = NULL;
	char  addr[256], *pbuf = NULL, *ptr, tbuf[3];
	unsigned short nsz, n;
	int   ret;

#undef RETURN
#define	RETURN(_x_) do { \
	if (pbuf) \
		acl_myfree(pbuf); \
	if (stream) \
		acl_vstream_close(stream); \
	return (_x_); \
} while (0)

	snprintf(addr, sizeof(addr), "%s:%d", res->dns_ip, res->dns_port);
	stream = acl_vstream_connect(addr, ACL_BLOCKING, res->conn_timeout,
			res->rw_timeout, 1024);
	if (stream == NULL) {
		res->errnum = ACL_RES_ERR_CONN;
		RETURN (-1);
	}

	pbuf = acl_mycalloc(1, dlen + 2);
	nsz = htons((short) dlen);
	ptr = pbuf;
	memcpy(ptr, &nsz, 2);
	ptr += 2;
	memcpy(ptr, data, dlen);

	ret = acl_vstream_writen(stream, pbuf, dlen + 2);
	if (ret == ACL_VSTREAM_EOF) {
		res->errnum = ACL_RES_ERR_SEND;
		RETURN (-1);
	}

	memset(tbuf, 0, sizeof(tbuf));

	ret = acl_vstream_readn(stream, &n, 2);
	if (ret == ACL_VSTREAM_EOF) {
		res->errnum = ACL_RES_ERR_READ;
		RETURN (-1);
	}

	nsz = ntohs(n);

	size = size > nsz ? nsz : size;
	ret = acl_vstream_readn(stream, buf, size);
	if (ret == ACL_VSTREAM_EOF) {
		res->errnum = ACL_RES_ERR_READ;
		RETURN (-1);
	}

	RETURN (ret);
#ifdef	ACL_BCB_COMPILER
	return -1;
#endif
}

static int res_lookup(ACL_RES *res, const char *data, int dlen,
	char *buf, int size)
{
	if (res->transfer == ACL_RES_USE_TCP) {
		return tcp_res_lookup(res, data, dlen, buf, size);
	} else {
		return udp_res_lookup(res, data, dlen, buf, size);
	}
}

static ACL_DNS_DB *acl_res_lookup_with_type(ACL_RES *res,
	const char *domain, int type)
{
	ACL_DNS_DB *dns_db;
	char  buf[1024];
	ssize_t ret, i;
	ACL_RFC1035_MESSAGE *answers;
	ACL_HOSTNAME *phost;
	time_t  begin;

	if (res == NULL) {
		acl_msg_error("%s(%d), %s: res NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}
	if (domain == NULL || *domain == 0) {
		acl_msg_error("%s(%d), %s: domain %s", __FILE__, __LINE__,
			__FUNCTION__, domain ? "empty" : "null");
		return NULL;
	}

	memset(buf, 0, sizeof(buf));

#ifdef	AF_INET6
	if (type == AF_INET6) {
		ret = (ssize_t) acl_rfc1035_build_query4aaaa(domain, buf,
			sizeof(buf), res->cur_qid++, NULL);
	} else {
		ret = (ssize_t) acl_rfc1035_build_query4a(domain, buf,
			sizeof(buf), res->cur_qid++, NULL);
	}
#else
	(void) type;
	ret = (ssize_t) acl_rfc1035_build_query4a(domain, buf, sizeof(buf),
		res->cur_qid++, NULL);
#endif

	if (ret == 0) {
		acl_msg_error("%s(%d), %s: build a query error",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	(void) time(&begin);
	ret = res_lookup(res, buf, (int) ret, buf, sizeof(buf));
	res->tm_spent = time(NULL) - begin;

	if (ret <= 0) {
		return NULL;
	}

	answers = acl_rfc1035_response_unpack(buf, ret);
	if (answers == NULL) {
		res->errnum = ACL_RES_ERR_NULL;
		return NULL;
	} else if (answers->ancount == 0) {
		acl_rfc1035_message_destroy(answers);
		res->errnum = ACL_RES_ERR_NULL;
		return NULL;
	}

	dns_db = acl_netdb_new(domain);

	for (i = 0; i < answers->ancount; i++) {
		if (answers->answer[i].type == ACL_RFC1035_TYPE_A) {
			phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));

			memcpy(&phost->saddr.in.sin_addr,
				answers->answer[i].rdata, 4);
			if (!inet_ntop(AF_INET, &phost->saddr.in.sin_addr,
				phost->ip, sizeof(phost->ip))) {

				acl_myfree(phost);
				continue;
			}

			phost->saddr.sa.sa_family = AF_INET;
			phost->ttl = answers->answer[i].ttl;
			phost->type = ACL_HOSTNAME_TYPE_IPV4;
			(void) acl_array_append(dns_db->h_db, phost);
			dns_db->size++;
#ifdef	AF_INET6
		} else if (answers->answer[i].type == ACL_RFC1035_TYPE_AAAA) {
			phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));

			memcpy(&phost->saddr.in6.sin6_addr,
				answers->answer[i].rdata, 16);
			if (!inet_ntop(AF_INET6, &phost->saddr.in6.sin6_addr,
				phost->ip, sizeof(phost->ip))) {

				acl_myfree(phost);
				continue;
			}

			phost->saddr.sa.sa_family = AF_INET6;
			phost->ttl = answers->answer[i].ttl;
			phost->type = ACL_HOSTNAME_TYPE_IPV6;
			(void) acl_array_append(dns_db->h_db, phost);
			dns_db->size++;
#endif
		} else if (acl_msg_verbose) {
			acl_msg_error("%s(%d), %s: answer type %d, domain %s",
				__FILE__, __LINE__, __FUNCTION__,
				(int) answers->answer[i].type, domain);
		}
	}

	acl_rfc1035_message_destroy(answers);
	return dns_db;
}

ACL_DNS_DB *acl_res_lookup(ACL_RES *res, const char *domain)
{
	return acl_res_lookup_with_type(res, domain, AF_INET);
}

#ifdef	AF_INET6
ACL_DNS_DB *acl_res_lookup6(ACL_RES *res, const char *domain)
{
	return acl_res_lookup_with_type(res, domain, AF_INET6);
}
#endif

const char *acl_res_strerror(int errnum)
{
	int   i;
	struct __ERRMSG {
		int  errnum;
		const char *msg;
	};
	static const struct __ERRMSG errmsg[] = {
		{ ACL_RES_ERR_SEND, "send msg error" },
		{ ACL_RES_ERR_READ, "read msg error" },
		{ ACL_RES_ERR_RTMO, "read timeout" },
		{ ACL_RES_ERR_NULL, "result emplty" },
		{ ACL_RES_ERR_CONN, "connect error" },
		{ 0, NULL },
	};

	for (i = 0; errmsg[i].errnum != 0; i++) {
		if (errmsg[i].errnum == errnum) {
			return errmsg[i].msg;
		}
	}

	return acl_rfc1035_strerror(errnum);
}

const char *acl_res_errmsg(const ACL_RES *res)
{
	if (res == NULL) {
		acl_msg_error("%s: res null", __FUNCTION__);
		return "res NULL";
	}

	return acl_res_strerror(res->errnum);
}
