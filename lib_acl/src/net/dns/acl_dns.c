#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#ifdef  ACL_UNIX
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_vstream.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_array.h"
#include "net/acl_sane_socket.h"
#include "net/acl_sane_inet.h"
#include "net/acl_mask_addr.h"
#include "net/acl_vstream_net.h"
#include "net/acl_dns.h"
#include "net/acl_rfc1035.h"

#endif

#include "../../aio/aio.h"

typedef struct ACL_DOMAIN_GROUP {
	char  group[ACL_RFC1035_MAXHOSTNAMESZ];
	int   group_len;
	char  domain[ACL_RFC1035_MAXHOSTNAMESZ];
	ACL_ARGV *excepts;
} ACL_DOMAIN_GROUP;

struct ACL_DNS_REQ{
	char  key[ACL_RFC1035_MAXHOSTNAMESZ + 16];
	void (*callback)(ACL_DNS_DB*, void*, int);
	void *ctx;
	int   nretry;
	ACL_DNS *dns;
	unsigned short qid;
};

#define SAFE_COPY ACL_SAFE_STRNCPY

static int dns_stream_open(ACL_DNS *dns);

/* ACL_VSTREAM: 从数据流读取数据的回调函数 */

static int dns_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream acl_unused, void *arg)
{
	ACL_DNS *dns = (ACL_DNS*) arg;
	int      ret;

	/* xxx: 必须先将系统可读标志位置0，以免引起事件引擎的重复触发 */
	stream->read_ready = 0;

	dns->addr_from.addr_len = sizeof(dns->addr_from.addr);
#ifdef ACL_UNIX
	ret = (int) recvfrom(fd, buf, size, 0,
			(struct sockaddr*) &dns->addr_from.addr,
			(socklen_t*) &dns->addr_from.addr_len);
#elif defined(ACL_WINDOWS)
	ret = recvfrom(fd, (char*) buf, (int) size, 0,
			(struct sockaddr*) &dns->addr_from.addr,
			&dns->addr_from.addr_len);
#else
#error "unknown OS"
#endif
	if (ret < 0) {
		acl_msg_error("%s, %s(%d): recvfrom error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
	}
	return ret;
}

/* ACL_VSTREAM: 向数据流写取数据的回调函数 */

static int dns_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream acl_unused, void *arg)
{
	ACL_DNS *dns = (ACL_DNS*) arg;
	int      ret;
	unsigned short i;
	ACL_DNS_ADDR  *addr;

	if (dns->dns_list->count <= 0) {
		acl_msg_fatal("%s(%d): dns_list's size(%d) invalid",
			__FUNCTION__, __LINE__, dns->dns_list->count);
	}

	/* 根据当前ID号取模获得目标DNS地址 */
	i = dns->dns_idx++ % dns->dns_list->count;
	if (dns->dns_idx == (unsigned) dns->dns_list->count) {
		dns->dns_idx = 0;
	}

	addr = acl_array_index(dns->dns_list, i);
	if (addr == NULL) {
		acl_msg_fatal("%s(%d): addr null for %d",
			__FUNCTION__, __LINE__, i);
	}

#ifdef ACL_UNIX 
	ret = (int) sendto(fd, buf, size, 0,
			(struct sockaddr*) &addr->addr, addr->addr_len);
#elif defined(ACL_WINDOWS)
	ret = sendto(fd, (const char*) buf, (int) size, 0,
			(struct sockaddr*) &addr->addr, addr->addr_len);
#else
#error  "unknown OS"
#endif
	if (ret < 0) {
		acl_msg_error("%s, %s(%d): sendto error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
	}
	return ret;
}

/* 根据DNS查询结果生成 ACL_DNS_DB 对象 */

static ACL_DNS_DB *build_dns_db(const ACL_RFC1035_MESSAGE *res,
	unsigned int *ttl_min)
{
	ACL_DNS_DB *dns_db = acl_netdb_new(res->query->name);
	ACL_HOSTNAME *phost;
	ACL_SOCKADDR *saddr;
	int   i;

	if (ttl_min) {
		*ttl_min = 100000000;
	}

	for (i = 0; i < res->ancount; i++) {
		if (res->answer[i].type == ACL_RFC1035_TYPE_A) {
			phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
			phost->type = ACL_HOSTNAME_TYPE_IPV4;

			saddr = &phost->saddr;

#if defined(ACL_UNIX)
			/* 这样直接赋值要比用 memcpy 快些 */
# ifdef MINGW
			saddr->in.sin_addr.s_addr =
				*((unsigned int*) res->answer[i].rdata);
# else
			saddr->in.sin_addr.s_addr =
				*((in_addr_t*) res->answer[i].rdata);
# endif
#elif defined(ACL_WINDOWS)
			saddr->in.sin_addr.s_addr =
				*((unsigned int*) res->answer[i].rdata);
#endif
			/* 目前该模块仅支持 IPV4 */
			saddr->sa.sa_family = AF_INET;

			if (!inet_ntop(AF_INET, &saddr->in.sin_addr,
				phost->ip, sizeof(phost->ip))) {

				continue;
			}

			phost->ttl = res->answer[i].ttl;
			if (ttl_min && *ttl_min > phost->ttl) {
				*ttl_min = phost->ttl;
			}

			(void) acl_array_append(dns_db->h_db, phost);
			dns_db->size++;
#ifdef AF_INET6
		} else if (res->answer[i].type == ACL_RFC1035_TYPE_AAAA) {
			phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
			phost->type = ACL_HOSTNAME_TYPE_IPV6;

			memcpy(&phost->saddr.in6.sin6_addr,
				res->answer[i].rdata, 16);
			if (!inet_ntop(AF_INET6, &phost->saddr.in6.sin6_addr,
				phost->ip, sizeof(phost->ip))) {

				acl_myfree(phost);
				continue;
			}

			/* 目前该模块仅支持 IPV4 */
            phost->saddr.sa.sa_family = AF_INET6;

			phost->ttl = res->answer[i].ttl;
			if (ttl_min && *ttl_min > phost->ttl) {
				*ttl_min = phost->ttl;
			}

			(void) acl_array_append(dns_db->h_db, phost);
			dns_db->size++;
#endif
		} else if (res->answer[i].type == ACL_RFC1035_TYPE_CNAME) {
			phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));
			phost->type = ACL_HOSTNAME_TYPE_CNAME;

			acl_snprintf(phost->ip, sizeof(phost->ip), "%s",
				res->answer[i].rdata);

			phost->ttl = res->answer[i].ttl;
			if (ttl_min && *ttl_min > phost->ttl) {
				*ttl_min = phost->ttl;
			}

			(void) acl_array_append(dns_db->h_db, phost);
			dns_db->size++;
		}
	}

	return dns_db;
}

static int dns_safe_addr_check(ACL_DNS *dns)
{
	char  from[64];
	ACL_ITER iter;

	/* 检查响应包的 DNS 地址与本地配置的地址是否匹配 */
	acl_foreach(iter, dns->dns_list) {
		ACL_DNS_ADDR *addr = (ACL_DNS_ADDR*) iter.data;

		if (dns->addr_from.addr.in.sin_addr.s_addr
			== addr->addr.in.sin_addr.s_addr) {
			return 0;
		}
	}

	inet_ntop(AF_INET, &dns->addr_from.addr.in.sin_addr, from, sizeof(from));
	acl_msg_warn("%s(%d): invalid from=%s", __FUNCTION__, __LINE__, from);

	return -1;
}

static int dns_safe_net_check(ACL_DNS *dns)
{
	char  from[64];
	ACL_ITER iter;

	acl_foreach(iter, dns->dns_list) {
		ACL_DNS_ADDR *addr = (ACL_DNS_ADDR*) iter.data;
		struct in_addr in;

		in.s_addr = dns->addr_from.addr.in.sin_addr.s_addr;
		acl_mask_addr((unsigned char*) &in.s_addr,
			sizeof(in.s_addr), addr->mask_length);

		if (in.s_addr == addr->in.s_addr) {
			return 0;
		}
	}

	inet_ntop(AF_INET, &dns->addr_from.addr.in.sin_addr, from, sizeof(from));
	acl_msg_warn("%s(%d): invalid from=%s", __FUNCTION__, __LINE__, from);
	return -1;
}

static void dns_lookup_error(ACL_DNS *dns, ACL_RFC1035_MESSAGE *res)
{
	char  key[ACL_RFC1035_MAXHOSTNAMESZ + 16];
	ACL_DNS_REQ *req;

	if (dns->aio == NULL) {
		acl_msg_info("%s(%d): the dns is closed", __FUNCTION__, __LINE__);
		return;
	}

	snprintf(key, sizeof(key), "%s:%d", res->query->name, res->id);
	acl_lowercase(key);
	req = acl_htable_find(dns->lookup_table, key);

	if (req) {
		/* 取消定时器 */
		acl_aio_cancel_timer(dns->aio, dns->lookup_timeout, req);

		/* 从查询列表删除该查询对象 */
		acl_htable_delete(dns->lookup_table, req->key, NULL);

		/* 通知应用查询失败 */
		req->callback(NULL, req->ctx, res->rcode);

		/* 释放该查询对象 */
		acl_myfree(req);
	}
}

static void dns_lookup_ok(ACL_DNS *dns, ACL_RFC1035_MESSAGE *res)
{
	char  key[ACL_RFC1035_MAXHOSTNAMESZ + 16];
	int   ttl_min;
	ACL_DNS_REQ *req;
	ACL_DNS_DB  *dns_db;

	acl_lowercase(res->query->name);
	snprintf(key, sizeof(key), "%s:%d", res->query->name, res->id);

	req = acl_htable_find(dns->lookup_table, key);
	if (req == NULL) {
		return;
	}

	/* 取消定时器 */
	if (dns->aio == NULL) {
		acl_aio_cancel_timer(dns->aio, dns->lookup_timeout, req);
	} else {
		acl_msg_warn("%s(%d): the dns is closed", __FUNCTION__, __LINE__);
	}

	/* 从查询列表删除该查询对象 */
	acl_htable_delete(dns->lookup_table, req->key, NULL);

	/* 创建 DNS 查询结果缓存对象，并加入本地缓存中 */
	dns_db = build_dns_db(res, (unsigned int*) &ttl_min);

	/* 设置返回本次查询结果所使用的 DNS 服务器地址*/
	acl_netdb_set_ns(dns_db, &dns->addr_from.addr);

	/* 回调函数用户的回调函数 */
	req->callback(dns_db, req->ctx, res->rcode);

	/* 释放该查询对象 */
	acl_myfree(req);

	/* 如果缓存机制允许则缓存该查询结果 */

	if (dns->dns_cache == NULL || ttl_min <= 0
		|| acl_cache2_enter(dns->dns_cache, res->query->name,
			dns_db, ttl_min) == NULL) {

		acl_netdb_free(dns_db);
	}
}

/* 有DNS服务器数据可读时的回调函数 */

static int dns_lookup_callback(ACL_ASTREAM *astream acl_unused, void *ctx,
	char *data, int dlen)
{
	ACL_DNS *dns = (ACL_DNS*) ctx;
	ACL_RFC1035_MESSAGE *res;

	/* 解析DNS响应数据包 */
	res = acl_rfc1035_response_unpack(data, dlen);
	if (res == NULL) {
		return 0;
	} else if (res->ancount == 0) {
		dns_lookup_error(dns, res);
		acl_rfc1035_message_destroy(res);
		return 0;
	}

	/* 是否检查 DNS 源/目的地址, 以保证安全性 */

	if ((dns->flag & ACL_DNS_FLAG_CHECK_DNS_IP)) {
		if (dns_safe_addr_check(dns) < 0) {
			acl_rfc1035_message_destroy(res);
			return 0;
		}
	}

	/* 是否检查 DNS 源/目的网络, 以保证安全性 */

	if ((dns->flag & ACL_DNS_FLAG_CHECK_DNS_NET)) {
		if (dns_safe_net_check(dns) < 0) {
			acl_rfc1035_message_destroy(res);
			return 0;
		}
	}

	dns_lookup_ok(dns, res);
	acl_rfc1035_message_destroy(res);
	return 0;
}

static void dns_stream_reopen_timer(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *ctx)
{
	ACL_DNS *dns = (ACL_DNS*) ctx;

	/* 创建一个新的套接口 */
	if (dns_stream_open(dns) == 0) {
		/* 异步读DNS服务器响应数据 */
		acl_aio_read(dns->astream);
	} else if (dns->aio) {
		/* 设置定时器重新打开 UDP 套接字 */
		acl_aio_request_timer(dns->aio, dns_stream_reopen_timer, dns,
			2 * 1000000, 0);
	} else {
		acl_msg_warn("%s(%d): the dns is closed", __FUNCTION__, __LINE__);
	}
}

/* 数据流出错时的回调函数 */

static int dns_lookup_close(ACL_ASTREAM *server acl_unused, void *ctx acl_unused)
{
	const char *myname = "dns_lookup_close";
	ACL_DNS *dns = (ACL_DNS*) ctx;


	/* 设置 UDP 句柄为 NULL，以防止在重新打开前被使用，因为本函数返回后该
	 * 异步流对象将会被关闭
	 */
	dns->astream = NULL;

	/* 设置定时器重新打开 UDP 套接字，不应立即打开 socket，以防止频繁关闭 */
	if (dns->aio) {
		acl_msg_warn("%s(%d): dns socket closed %s, re-open it in timer",
			myname, __LINE__, acl_last_serror());
		acl_aio_request_timer(dns->aio, dns_stream_reopen_timer, dns,
			2* 1000000, 0);
	}
	return -1;
}

/* 创建DNS查询的异步流 */

static int dns_stream_open(ACL_DNS *dns)
{
	/* ndk9 居然要求 acl_vstream_bind 前加返回类型？*/
	ACL_VSTREAM *stream = acl_vstream_bind("0.0.0.0:0", 0, 0);

	if (stream == NULL) {
		acl_msg_error("%s(%d), %s: acl_vstream_bind error=%s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		dns->astream = NULL;
		return -1;
	} else if (dns->aio == NULL) {
		acl_msg_error("%s(%d): dns->aio = NULL", __FUNCTION__, __LINE__);
		return -1;
	}

	/* 创建异步流 */
	dns->astream = acl_aio_open(dns->aio, stream);
	acl_vstream_ctl(stream,
		ACL_VSTREAM_CTL_READ_FN, dns_read,
		ACL_VSTREAM_CTL_WRITE_FN, dns_write,
		ACL_VSTREAM_CTL_CONTEXT, dns,
		ACL_VSTREAM_CTL_END);

	/* 设置查询套接口可读、关闭时的回调函数 */
	acl_aio_add_read_hook(dns->astream, dns_lookup_callback, dns);
	acl_aio_add_close_hook(dns->astream, dns_lookup_close, dns);

	/* 设置该异步流为持续读状态 */
	dns->astream->keep_read = 1;
	return 0;
}

static void dns_lookup_send(ACL_DNS *dns, const char *domain, unsigned short qid)
{
	char   buf[1024];
	size_t ret;

	memset(buf, 0, sizeof(buf));

	/* 创建DNS查询数据包 */
	ret = acl_rfc1035_build_query4a(domain, buf, sizeof(buf), qid, NULL);

	/* 发送请求DNS包 */
	if (dns->astream != NULL) {
		acl_aio_writen(dns->astream, buf, (int) ret);
	} else {
		acl_msg_warn("%s(%d), %s: astream null, wait for a while",
			__FILE__, __LINE__, __FUNCTION__);
	}
}

/* 查询超时的回调函数 */

static void dns_lookup_timeout(int event_type, ACL_EVENT *event acl_unused,
	void *context)
{
	ACL_DNS_REQ *req = (ACL_DNS_REQ*) context;
	ACL_DNS *dns = req->dns;

	if (event_type != ACL_EVENT_TIME) {
		acl_msg_warn("%s(%d): invalid event_type(%d)",
			__FUNCTION__, __LINE__, event_type);
	}

	if (++req->nretry <= dns->retry_limit) {
		char  domain[ACL_RFC1035_MAXHOSTNAMESZ + 16], *ptr;
		int   i;

		SAFE_COPY(domain, req->key, sizeof(domain));
		ptr = strchr(domain, ':');
		if (ptr) {
			*ptr = 0;
		}

		for (i = 0; i < dns->dns_list->count; i++) {
			dns_lookup_send(dns, domain, req->qid);
		}

		/* 设置定时器 */
		if (dns->aio) {
			acl_aio_request_timer(dns->aio, dns->lookup_timeout,
				req, dns->timeout * 1000000, 0);
		} else {
			acl_msg_error("%s(%d): dns->aio NULL", __FUNCTION__, __LINE__);
		}
		return;
	}

	/* 从查询列表删除该查询对象 */
	acl_htable_delete(req->dns->lookup_table, req->key, NULL);

	/* 回调函数用户的回调函数 */
	req->callback(NULL, req->ctx, ACL_DNS_ERR_TIMEOUT);

	/* 释放该查询对象 */
	acl_myfree(req);
}

int acl_dns_init(ACL_DNS *dns, ACL_AIO *aio, int timeout)
{
	dns->flag       &= ~ACL_DNS_FLAG_ALLOC;  /* 默认为栈空间 */
	dns->aio         = aio;
	dns->timeout     = timeout > 0 ? timeout : 5;
	dns->qid         = 0;
	dns->dns_idx     = 0;
	dns->retry_limit = 0;

	/* 创建DNS服务器地址数组 */
	dns->dns_list       = acl_array_create(10);

	/* 创建查询对象表 */
	dns->lookup_table   = acl_htable_create(1024, 0);

	/* 设置 DNS 查询超时的回调函数*/
	dns->lookup_timeout = dns_lookup_timeout;

	/* 打开异步读取DNS服务器响应的数据流 */
	if (dns_stream_open(dns) == -1) {
		acl_msg_error("%s(%d), %s: dns_stream_open error=%s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}

	/* 开始异步读查询结果 */
	acl_aio_read(dns->astream);
	return 0;
}

ACL_DNS *acl_dns_create(ACL_AIO *aio, int timeout)
{
	ACL_DNS *dns = (ACL_DNS*) acl_mycalloc(1, sizeof(ACL_DNS));

	if (acl_dns_init(dns, aio, timeout) == -1) {
		acl_myfree(dns);
		acl_msg_error("%s(%d), %s: acl_dns_init error",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}
	dns->flag |= ACL_DNS_FLAG_ALLOC;  /* 设置为堆分配的变量 */
	return dns;
}

void acl_dns_close(ACL_DNS *dns)
{
	ACL_ITER iter;

	acl_foreach(iter, dns->lookup_table) {
		ACL_DNS_REQ *req = (ACL_DNS_REQ*) iter.data;
		acl_myfree(req);
	}

	if (dns->aio) {
		dns->aio->dns = NULL;

		/* 置空后，为后续过程提示当前对象正在关闭过程中 */
		dns->aio = NULL;
	}

	acl_htable_free(dns->lookup_table, NULL);
	dns->lookup_table = NULL;
	if (dns->dns_cache) {
		acl_cache2_free(dns->dns_cache);
		dns->dns_cache = NULL;
	}
	if (dns->astream != NULL) {
		acl_aio_iocp_close(dns->astream);
		dns->astream = NULL;
	}
	acl_array_destroy(dns->dns_list, acl_myfree_fn);

	if (dns->groups) {
		acl_foreach(iter, dns->groups) {
			ACL_DOMAIN_GROUP *tmp = (ACL_DOMAIN_GROUP*) iter.data;
			if (tmp->excepts) {
				acl_argv_free(tmp->excepts);
			}
			acl_myfree(tmp);
		}
		acl_array_destroy(dns->groups, NULL);
	}

	if ((dns->flag & ACL_DNS_FLAG_ALLOC)) {
		acl_myfree(dns);
	} else {
		dns->flag = 0;
	}
}

void acl_dns_check_dns_ip(ACL_DNS *dns)
{
	dns->flag |= ACL_DNS_FLAG_CHECK_DNS_IP;
}

void acl_dns_check_dns_net(ACL_DNS *dns)
{
	dns->flag |= ACL_DNS_FLAG_CHECK_DNS_NET;
}

void acl_dns_set_retry_limit(ACL_DNS *dns, int retry_limit)
{
	dns->retry_limit = retry_limit;
}

static void cache_free_fn(const ACL_CACHE2_INFO *info acl_unused, void *arg)
{
	ACL_DNS_DB *dns_db = (ACL_DNS_DB*) arg;

	/* 释放缓存对象 */
	acl_netdb_free(dns_db);
}

void acl_dns_open_cache(ACL_DNS *dns, int limit)
{
	if (dns->dns_cache == NULL && limit > 0) {
		dns->dns_cache = acl_cache2_create(limit, cache_free_fn);
	}
}

void acl_dns_add_dns(ACL_DNS *dns, const char *dns_ip,
	unsigned short dns_port, int mask_length)
{
	ACL_DNS_ADDR *addr;

	if (mask_length >= 32 || mask_length <= 0) {
		acl_msg_error("%s(%d): mask_length(%d) invalid",
			__FUNCTION__, __LINE__, mask_length);
		return;
	}

	addr = (ACL_DNS_ADDR*) acl_mycalloc(1, sizeof(ACL_DNS_ADDR));
	addr->mask_length = mask_length;

	/* 设置DNS服务器地址 */

	SAFE_COPY(addr->ip, dns_ip, sizeof(addr->ip));
	addr->port = dns_port;

	memset(&addr->addr, 0, sizeof(addr->addr));
	addr->addr.in.sin_port        = htons(dns_port);
	addr->addr.in.sin_addr.s_addr = inet_addr(dns_ip);
	addr->addr.sa.sa_family       = AF_INET;
	addr->addr_len                = sizeof(struct sockaddr_in);

	addr->in.s_addr               = addr->addr.in.sin_addr.s_addr;
	acl_mask_addr((unsigned char*) &addr->in.s_addr,
		sizeof(addr->in.s_addr), mask_length);

	/* 将该DNS地址添加进数组中 */
	(void) acl_array_append(dns->dns_list, addr);
}

void acl_dns_del_dns(ACL_DNS *dns, const char *ip, unsigned short port)
{
	int size = acl_array_size(dns->dns_list), i;

	for (i = 0; i < size; i++) {
		ACL_DNS_ADDR *addr = (ACL_DNS_ADDR*)
			acl_array_index(dns->dns_list, i);
		if (strcmp(addr->ip, ip) && addr->port == port) {
			acl_array_delete(dns->dns_list, i, NULL);
			acl_myfree(addr);
			break;
		}
	}
}

void acl_dns_clear_dns(ACL_DNS *dns)
{
	acl_array_clean(dns->dns_list, acl_myfree_fn);
}

ACL_ARRAY *acl_dns_list(ACL_DNS *dns)
{
	return dns->dns_list;
}

size_t acl_dns_size(ACL_DNS *dns)
{
	return (size_t) acl_array_size(dns->dns_list);
}

int acl_dns_empty(ACL_DNS *dns)
{
	return acl_dns_size(dns) == 0;
}

void acl_dns_add_host(ACL_DNS *dns, const char *domain, const char *ip_list)
{
	ACL_DNS_DB *dns_db;
	ACL_ARGV *argv;
	ACL_ITER iter;

	if (dns->dns_cache == NULL) {
		acl_msg_error("%s(%d): please call acl_dns_open_cache first!",
			__FUNCTION__, __LINE__);
		return;
	}

	dns_db = acl_netdb_new(domain);
	argv = acl_argv_split(ip_list, ",; \t");
	acl_foreach(iter, argv) {
		char *ip = (char*) iter.data;
		ACL_HOSTNAME *phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));

		SAFE_COPY(phost->ip, ip, sizeof(phost->ip));
		phost->saddr.sa.sa_family       = AF_INET;
		phost->saddr.in.sin_addr.s_addr = inet_addr(ip);
		(void) acl_array_append(dns_db->h_db, phost);
	}

	if (acl_cache2_enter(dns->dns_cache, dns_db->name, dns_db, 0) == NULL) {
		acl_msg_fatal("%s(%d): add domain(%s) error(%s)",
			__FUNCTION__, __LINE__, domain, acl_last_serror());
		acl_netdb_free(dns_db);
	}
	acl_argv_free(argv);
}

void acl_dns_add_group(ACL_DNS *dns, const char *group, const char *refer,
	const char *ip_list, const char *excepts)
{
	ACL_DOMAIN_GROUP *dmgrp;
	ACL_ITER iter;

	if (dns->groups == NULL) {
		dns->groups = acl_array_create(10);
	}

	acl_foreach(iter, dns->groups) {
		dmgrp = (ACL_DOMAIN_GROUP*) iter.data;
		if (strcasecmp(dmgrp->group, group) == 0) {
			acl_msg_warn("%s(%d): group(%s) already exist",
				__FUNCTION__, __LINE__, group);
			return;
		}
	}

	dmgrp = (ACL_DOMAIN_GROUP*) acl_mycalloc(1, sizeof(ACL_DOMAIN_GROUP));

	SAFE_COPY(dmgrp->group, group, sizeof(dmgrp->group));
	acl_lowercase(dmgrp->group);
	dmgrp->group_len = (int) strlen(dmgrp->group);

	if (refer == NULL || *refer == 0) {
		SAFE_COPY(dmgrp->domain, dmgrp->group, sizeof(dmgrp->domain));
	} else {
		SAFE_COPY(dmgrp->domain, refer, sizeof(dmgrp->domain));
		acl_lowercase(dmgrp->domain);
	}
	if (excepts) {
		dmgrp->excepts = acl_argv_split(excepts, ",; \t");
	} else {
		dmgrp->excepts = NULL;
	}

	acl_array_append(dns->groups, dmgrp);

	if (ip_list && *ip_list) {
		acl_dns_add_host(dns, dmgrp->domain, ip_list);
	}
}

void acl_dns_lookup(ACL_DNS *dns, const char *domain_in,
	void (*callback)(ACL_DNS_DB*, void*, int), void *ctx)
{
	char  key[ACL_RFC1035_MAXHOSTNAMESZ + 16], domain[ACL_RFC1035_MAXHOSTNAMESZ];
	ACL_DNS_REQ *req;
	int i;

	/* 先检查是否匹配域名组 */
	if (dns->groups) {
		ACL_DOMAIN_GROUP *dmgrp = NULL;
		ACL_ITER iter;
		acl_foreach(iter, dns->groups) {
			ACL_ITER iter2;
			ACL_DOMAIN_GROUP *tmp = (ACL_DOMAIN_GROUP*) iter.data;

#define NEQ acl_strrncasecmp

			/* 先找到域名组对象 */
			if (NEQ(tmp->group, domain_in, tmp->group_len)) {
				continue;
			}
			/* 检查该域名是否是域名组的例外域名 */
			if (!tmp->excepts) {
				dmgrp = tmp;
				break;
			}
			acl_foreach(iter2, tmp->excepts) {
				char *except = (char*) iter2.data;
				if (strcasecmp(except, domain_in) == 0) {
					goto END_FOREACH_TAG;
				}
			}
		}

END_FOREACH_TAG:
		if (dmgrp) {
			SAFE_COPY(domain, dmgrp->domain, sizeof(domain));
		} else {
			SAFE_COPY(domain, domain_in, sizeof(domain));
		}
	} else {
		SAFE_COPY(domain, domain_in, sizeof(domain));
	}

	acl_lowercase(domain);

	/* 如果打开DNS缓存功能，则优先查询缓存 */
	if (dns->dns_cache) {
		ACL_DNS_DB *dns_db;
		dns_db = acl_cache2_find(dns->dns_cache, domain);
		if (dns_db) {
			callback(dns_db, ctx, ACL_DNS_OK_CACHE);
			return;
		}
	}

	snprintf(key, sizeof(key), "%s:%d", domain, dns->qid);
	acl_lowercase(key);
	req = (ACL_DNS_REQ*) acl_htable_find(dns->lookup_table, key);

	/* XXX: 不应存在相同的键存在, 因为该键是由域名及自动ID组成 */
	if (req != NULL) {
		acl_msg_warn("%s(%d): key(%s) exist",
			__FUNCTION__, __LINE__, key);
		callback(NULL, ctx, ACL_DNS_ERR_EXIST);
		return;
	}

	/* 分配新的查询对象 */
	req           = (ACL_DNS_REQ*) acl_mycalloc(1, sizeof(ACL_DNS_REQ));
	req->dns      = dns;
	req->callback = callback;
	req->ctx      = ctx;
	req->qid      = dns->qid++;
	SAFE_COPY(req->key, key, sizeof(req->key));

	/* 添加进查询对象表中 */
	if (acl_htable_enter(dns->lookup_table, key, req) == NULL) {
		acl_msg_fatal("%s(%d): enter htable error(%s)",
			__FUNCTION__, __LINE__, acl_last_serror());
	}

	for (i = 0; i < dns->dns_list->count; i++) {
		dns_lookup_send(dns, domain, req->qid);
	}

	/* 设置定时器 */
	acl_aio_request_timer(dns->aio, dns->lookup_timeout,
		req, dns->timeout * 1000000, 0);
}

void acl_dns_cancel(ACL_DNS_REQ *req)
{
	if (req == NULL || req->dns == NULL) {
		acl_msg_error("%s(%d): input error", __FUNCTION__, __LINE__);
		return;
	}
	acl_htable_delete(req->dns->lookup_table, req->key, NULL);
	acl_myfree(req);
}

const char *acl_dns_serror(int errnum)
{
	struct __ERRMSG{
		int   errnum;
		const char *msg;
	};
	static const struct __ERRMSG errmsg[] = {
		{ ACL_DNS_OK, "OK, No error condition" },
		{ ACL_DNS_OK_CACHE, "OK, in cache" },
		{ ACL_DNS_ERR_FMT, "Format Error: The name server was unable to "
			"interpret the query." },
		{ ACL_DNS_ERR_SVR, "Server Failure: The name server was "
			"unable to process this query." },
		{ ACL_DNS_ERR_NO_EXIST, "Name Error: The domain name does not exist." },
		{ ACL_DNS_ERR_NO_SUPPORT, "Not Implemented: The name server does "
			"not support the requested kind of query." },
		{ ACL_DNS_ERR_DENY, "Refused: The name server refuses to "
			"perform the specified operation." },
		{ ACL_DNS_ERR_YX, "The domain should not exist" },
		{ ACL_DNS_ERR_YXRR, "The domain's RR should not exist" },
		{ ACL_DNS_ERR_NXRR, "The domain's RR should exist" },
		{ ACL_DNS_ERR_NO_AUTH, "The dns is not authority" },
		{ ACL_DNS_ERR_NOT_ZONE, "The domain name is not in the zone" },
		{ ACL_DNS_ERR_NOT_ZONE + 1, "Unknown Error" },
		{ ACL_DNS_ERR_NOT_ZONE + 2, "Unknown Error" },
		{ ACL_DNS_ERR_NOT_ZONE + 3, "Unknown Error" },
		{ ACL_DNS_ERR_NOT_ZONE + 4, "Unknown Error" },
		{ ACL_DNS_ERR_UNPACK, "The DNS reply message is corrupt or could "
			"not be safely parsed." },
		{ ACL_DNS_ERR_TIMEOUT, "The DNS reply timeout" },
		{ ACL_DNS_ERR_EXIST, "The same DNS search exist" },
		{ ACL_DNS_ERR_BUILD_REQ, "Can't build query packet" },
		{ 0, 0 }
	};
	static const char *unknown = "Unknown Error";
	size_t i;

	for (i = 0; errmsg[i].msg != NULL; ++i) {
		if (errnum == errmsg[i].errnum)
			return errmsg[i].msg;
	}
	return unknown;
}
