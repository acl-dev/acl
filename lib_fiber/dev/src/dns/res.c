#include "stdafx.h"
#include "common.h"

#include "sane_inet.h"
#include "netdb.h"
#include "rfc1035.h"
#include "res.h"

static int __conn_timeout = 10;
static int __rw_timeout   = 10;

void res_set_timeout(int conn_timeout, int rw_timeout)
{
	__conn_timeout = conn_timeout;
	__rw_timeout   = rw_timeout;
}

RES *res_new(const char *dns_ip, unsigned short dns_port)
{
	const char *name = "res_new";
	RES *res;

	if (dns_ip == NULL || *dns_ip == 0) {
		msg_fatal("%s: dns_ip invalid", name);
	}
	if (dns_port <= 0) {
		dns_port = 53;
	}

	res = calloc(1, sizeof(RES));
	res->cur_qid = (unsigned short) time(NULL);

	SAFE_STRNCPY(res->dns_ip, dns_ip, sizeof(res->dns_ip));
	res->dns_port = dns_port;

	res->conn_timeout = __conn_timeout;
	res->rw_timeout   = __rw_timeout;
	res->transfer     = RES_USE_UDP;

	return res;
}

void res_free(RES *res)
{
	if (res) {
		free(res);
	}
}

static int udp_lookup(RES *res, const char *data, int dlen, char *buf, int size)
{
	const char *name = "udp_lookup";
	ssize_t ret;
	int fd;
	struct sockaddr_in addr;

	fd = socket(PF_INET, SOCK_DGRAM, 0);
	if (fd == -1) {
		msg_fatal("%s: socket create error", name);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(res->dns_port);
	addr.sin_addr.s_addr = inet_addr(res->dns_ip);

	ret = sendto(fd, data, dlen, 0, (struct sockaddr *) &addr,
			(socklen_t) sizeof(addr));
	if (ret < 0) {
		close(fd);
		res->errnum = RES_ERR_SEND;
		return (-1);
	}

	ret = read_wait(fd, res->rw_timeout);
	if (ret < 0) {
		close(fd);
		res->errnum = RES_ERR_RTMO;
		return (-1);
	}

	ret = recv(fd, buf, size, 0);
	close(fd);

	if (ret <= 0) {
		res->errnum = RES_ERR_SEND;
		return (-1);
	}

	return (int) ret;
}

static int res_lookup2(RES *res, const char *data,
	int dlen, char *buf, int size)
{
	return udp_lookup(res, data, dlen, buf, size);
}

DNS_DB *res_lookup(RES *res, const char *domain)
{
	const char *name = "res_lookup";
	DNS_DB *dns_db;
	char  buf[1024];
	ssize_t ret, i;
	rfc1035_message *answers;
	HOSTNAME *phost;
	time_t  begin;

	if (res == NULL) {
		msg_fatal("%s: res NULL", name);
	}

	if (domain == NULL || *domain == 0) {
		msg_error("%s: domain %s", name, domain ? "empty" : "null");
		return NULL;
	}

	memset(buf, 0, sizeof(buf));
	ret = rfc1035BuildAQuery(domain, buf, sizeof(buf), res->cur_qid++, NULL);

	(void) time(&begin);
	ret = res_lookup2(res, buf, (int) ret, buf, sizeof(buf));
	res->tm_spent = time(NULL) - begin;

	if (ret <= 0) {
		return NULL;
	}

	ret = rfc1035MessageUnpack(buf, ret, &answers);
	if (ret < 0) {
		res->errnum = (int) ret;
		return NULL;
	} else if (ret == 0) {
		rfc1035MessageDestroy(answers);
		res->errnum = RES_ERR_NULL;
		return NULL;
	}

	dns_db = netdb_new(domain);
	if (dns_db == NULL) {
		rfc1035MessageDestroy(answers);
		msg_error("%s: netdb_new error", name);
		return NULL;
	}

	for (i = 0; i < ret; i++) {
		if (answers->answer[i].type == RFC1035_TYPE_A) {
			phost = calloc(1, sizeof(HOSTNAME));
			if (phost == NULL) {
				msg_error("%s: calloc error(%s)",
					name, last_strerror(buf, sizeof(buf)));
				netdb_free(dns_db);
				rfc1035MessageDestroy(answers);
				return NULL;
			}

			memcpy(&phost->saddr.sin_addr, answers->answer[i].rdata, 4);
			sane_inet_ntoa(phost->saddr.sin_addr, phost->ip, sizeof(phost->ip));
			phost->ttl = answers->answer[i].ttl;

			if (array_append(dns_db->h_db, phost) < 0) {
				msg_error("%s, %s(%d): array append error(%s)",
					__FILE__, name, __LINE__,
					last_strerror(buf, sizeof(buf)));
				netdb_free(dns_db);
				rfc1035MessageDestroy(answers);
				return (NULL);
			}       

			dns_db->size++;
		}
	}

	rfc1035MessageDestroy(answers);
	return dns_db;
}

const char *res_strerror(int errnum)
{
	int   i;
	struct __ERRMSG {
		int  errnum;
		const char *msg;
	};
	static const struct __ERRMSG errmsg[] = {
		{ RES_ERR_SEND, "send msg error" },
		{ RES_ERR_READ, "read msg error" },
		{ RES_ERR_RTMO, "read timeout" },
		{ RES_ERR_NULL, "result emplty" },
		{ RES_ERR_CONN, "connect error" },
		{ 0, NULL },
	};

	for (i = 0; errmsg[i].errnum != 0; i++) {
		if (errmsg[i].errnum == errnum) {
			return errmsg[i].msg;
		}
	}

	return rfc1035Strerror(errnum);
}

const char *res_errmsg(const RES *res)
{
	const char *name = "res_errmsg";

	if (res == NULL) {
		msg_fatal("%s: res null", name);
	}

	return res_strerror(res->errnum);
}
