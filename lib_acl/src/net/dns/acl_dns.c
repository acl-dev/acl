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
#include "net/acl_sane_inet.h"
#include "net/acl_mask_addr.h"
#include "net/acl_dns.h"

#endif

#include "rfc1035.h"

typedef struct ACL_DOMAIN_GROUP {
	char  group[RFC1035_MAXHOSTNAMESZ];
	int   group_len;
	char  domain[RFC1035_MAXHOSTNAMESZ];
	ACL_ARGV *excepts;
} ACL_DOMAIN_GROUP;

struct ACL_DNS_REQ{
	char  key[RFC1035_MAXHOSTNAMESZ + 16];
	void (*callback)(ACL_DNS_DB *, void *, int);
	void *ctx;
	unsigned short qid;
	int   nretry;
	ACL_DNS *dns;
};

static void dns_stream_open(ACL_DNS *dns);

/* ACL_VSTREAM: 从数据流读取数据的回调函数 */

static int dns_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream acl_unused, void *arg)
{       
	const char *myname = "dns_read";
	ACL_DNS *dns = (ACL_DNS*) arg;
	int   ret;

	dns->addr_from.addr_len = sizeof(dns->addr_from.addr);
#ifdef ACL_UNIX
	ret = recvfrom(fd, buf, size, 0,
			(struct sockaddr*) &dns->addr_from.addr,
			(socklen_t*) &dns->addr_from.addr_len);
#elif defined(WIN32)
	ret = recvfrom(fd, (char*) buf, (int) size, 0,
			(struct sockaddr*) &dns->addr_from.addr,
			&dns->addr_from.addr_len);
#else   
#error "unknown OS"     
#endif                  
	if (ret < 0)
		acl_msg_error("%s, %s(%d): recvfrom error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	return (ret);
}

/* ACL_VSTREAM: 向数据流写取数据的回调函数 */

static int dns_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream acl_unused, void *arg)
{       
	const char *myname = "dns_write";
	ACL_DNS *dns = (ACL_DNS*) arg;
	int   ret;
	unsigned short i;
	ACL_DNS_ADDR *addr;

	if (dns->dns_list->count <= 0)
		acl_msg_fatal("%s(%d): dns_list's size(%d) invalid",
			myname, __LINE__, dns->dns_list->count);

	/* 根据当前ID号取模获得目标DNS地址 */
	i = dns->qid % dns->dns_list->count;
	addr = acl_array_index(dns->dns_list, i);
	if (addr == NULL)
		acl_msg_fatal("%s(%d): addr null for %d",
			myname, __LINE__, i);

#ifdef ACL_UNIX 
	ret = sendto(fd, buf, size, 0,
			(struct sockaddr*) &addr->addr, addr->addr_len);
#elif defined(WIN32)
	ret = sendto(fd, (const char*) buf, (int) size,
			0, (struct sockaddr*) &addr->addr, addr->addr_len);
#else
#error  "unknown OS"
#endif
	return (ret);
}

/* 根据DNS查询结果生成 ACL_DNS_DB 对象 */

static ACL_DNS_DB *build_dns_db(const rfc1035_message *res, int count,
	unsigned int *ttl_min)
{
	const char *myname = "build_dns_db";
	ACL_DNS_DB *dns_db = acl_netdb_new(res->query->name);
	int   i;

	if (ttl_min)
		*ttl_min = 100000000;

	for (i = 0; i < count; i++) {
		if (res->answer[i].type == RFC1035_TYPE_A) {
			ACL_HOSTNAME *phost =
				acl_mycalloc(1, sizeof(ACL_HOSTNAME));
			if (phost == NULL) {
				acl_msg_fatal("%s: calloc error(%s)",
					myname, acl_last_serror());
			}

#if 0
			memcpy(&phost->saddr.sin_addr, res->answer[i].rdata, 4);
#elif defined(ACL_UNIX)
			/* 这样直接赋值要比用 memcpy 快些 */
			phost->saddr.sin_addr.s_addr =
				*((in_addr_t*) res->answer[i].rdata);
#elif defined(WIN32)
			phost->saddr.sin_addr.s_addr =
				*((unsigned int*) res->answer[i].rdata);
#endif
			acl_inet_ntoa(phost->saddr.sin_addr,
				phost->ip, sizeof(phost->ip));
			phost->ttl = res->answer[i].ttl;
			if (ttl_min && *ttl_min > phost->ttl)
				*ttl_min = phost->ttl;

			if (acl_array_append(dns_db->h_db, phost) < 0) {
				acl_msg_fatal("%s(%d): array append error(%s)",
					myname, __LINE__, acl_last_serror());
			}
			dns_db->size++;
		} else if (0) {
			ACL_HOSTNAME *phost =
				acl_mycalloc(1, sizeof(ACL_HOSTNAME));
			if (phost == NULL) {
				acl_msg_fatal("%s: calloc error(%s)",
					myname, acl_last_serror());
			}

			memcpy(&phost->saddr.sin_addr, res->answer[i].rdata, 4);
			acl_inet_ntoa(phost->saddr.sin_addr,
				phost->ip, sizeof(phost->ip));
			phost->ttl = res->answer[i].ttl;

			if (acl_array_append(dns_db->h_db, phost) < 0) {
				acl_msg_fatal("%s(%d): array append error(%s)",
					myname, __LINE__, acl_last_serror());
			}
			dns_db->size++;

			acl_msg_warn("%s: can't print answer type %d, domain %s, ip %s",
				myname, res->answer[i].type, res->query->name, phost->ip);
		}
	}

	return (dns_db);
}

/* 有DNS服务器数据可读时的回调函数 */

static int dns_lookup_callback(ACL_ASTREAM *astream acl_unused, void *ctx,
	char *data, int dlen)
{
	const char *myname = "dns_lookup_callback";
	ACL_DNS *dns = (ACL_DNS*) ctx;
	int   ret;
	ACL_DNS_REQ *handle;
	rfc1035_message *res;
	char  key[RFC1035_MAXHOSTNAMESZ + 16];

	/* 解析DNS响应数据包 */
	ret = rfc1035MessageUnpack(data, dlen, &res);
	if (ret < 0) {
		if (res == NULL)
			return (0);

		snprintf(key, sizeof(key), "%s:%d", res->query->name, res->id);
		acl_lowercase(key);
		handle = acl_htable_find(dns->lookup_table, key);

		if (handle) {
			void (*callback)(ACL_DNS_DB*, void*, int) = handle->callback;
			void *arg = handle->ctx;

			/* 取消定时器 */
			acl_aio_cancel_timer(dns->aio, dns->lookup_timeout, handle);
			/* 释放该查询对象 */
			acl_htable_delete(dns->lookup_table, handle->key, NULL);
			acl_myfree(handle);
			/* 通知应用查询失败 */
			callback(NULL, arg, res->rcode);
		}

		rfc1035MessageDestroy(res);
		return (0);
	} else if (ret == 0) {
		rfc1035MessageDestroy(res);
		return (0);
	}

	/* 是否检查 DNS 源/目的地址, 以保证安全性 */

	if ((dns->flag & ACL_DNS_FLAG_CHECK_DNS_IP)) {
		ACL_DNS_ADDR *addr;
		unsigned short i;

		/* 获得本数据包对应的 DNS 地址索引 */
		i = (res->id + 1) % dns->dns_list->count;
		addr = acl_array_index(dns->dns_list, i);
		if (addr == NULL)
			acl_msg_fatal("%s(%d): addr null for %d",
				myname, __LINE__, i);

		if (dns->addr_from.addr.sin_addr.s_addr != addr->addr.sin_addr.s_addr) {
			char  from[64], to[64];
			acl_inet_ntoa(dns->addr_from.addr.sin_addr, from, sizeof(from));
			acl_inet_ntoa(addr->addr.sin_addr, to, sizeof(to));
			acl_msg_warn("%s(%d): from(%s) != to(%s)",
				myname, __LINE__, from, to);
			rfc1035MessageDestroy(res);
			return (0);
		}
	}

	/* 是否检查 DNS 源/目的网络, 以保证安全性 */

	if ((dns->flag & ACL_DNS_FLAG_CHECK_DNS_NET)) {
		struct in_addr in;
		ACL_DNS_ADDR *addr;
		unsigned short i;

		/* 获得本数据包对应的 DNS 地址索引 */
		i = (res->id + 1) % dns->dns_list->count;
		addr = acl_array_index(dns->dns_list, i);
		if (addr == NULL)
			acl_msg_fatal("%s(%d): addr null for %d",
				myname, __LINE__, i);

		in.s_addr = dns->addr_from.addr.sin_addr.s_addr;
		acl_mask_addr((unsigned char*) &in.s_addr,
			sizeof(in.s_addr), addr->mask_length);
		if (in.s_addr != addr->in.s_addr) {
			char  from[64], to[64];
			acl_inet_ntoa(in, from, sizeof(from));
			acl_inet_ntoa(addr->in, to, sizeof(to));
			acl_msg_warn("%s(%d): from(%s) != to(%s)",
				myname, __LINE__, from, to);
			rfc1035MessageDestroy(res);
			return (0);
		}
	}

	acl_lowercase(res->query->name);
	snprintf(key, sizeof(key), "%s:%d", res->query->name, res->id);
	handle = acl_htable_find(dns->lookup_table, key);
	if (handle != NULL) {
		int ttl_min;
		void (*callback)(ACL_DNS_DB*, void*, int) = handle->callback;
		void *arg = handle->ctx;
		ACL_DNS_DB *dns_db =
			build_dns_db(res, ret, (unsigned int*) &ttl_min);

		/* 取消定时器 */
		acl_aio_cancel_timer(dns->aio, dns->lookup_timeout, handle);
		/* 释放该查询对象 */
		acl_htable_delete(dns->lookup_table, handle->key, NULL);
		acl_myfree(handle);

		/* 回调函数用户的回调函数 */
		callback(dns_db, arg, res->rcode);

		/* 如果缓存机制允许则缓存该查询结果 */
		if (dns->dns_cache == NULL) {
			/* 释放结果对象 */
			acl_netdb_free(dns_db);
		} else {
			if (ttl_min <= 0 || acl_cache2_enter(dns->dns_cache,
				res->query->name, dns_db, ttl_min) == NULL)
			{
				acl_netdb_free(dns_db);
			}
		}
	}

	rfc1035MessageDestroy(res);
	return (0);
}

/* 数据流出错时的回调函数 */

static int dns_lookup_error(ACL_ASTREAM *server acl_unused, void *ctx acl_unused)
{
	const char *myname = "dns_lookup_error";
#if 0
	ACL_DNS *dns = (ACL_DNS*) ctx;

	acl_msg_warn("%s(%d): error(%s), re-open a new socket",
		myname, __LINE__, acl_last_serror());

	/* 创建一个新的套接口 */
	dns_stream_open(dns);

	/* 异步读DNS服务器响应数据 */
	acl_aio_read(dns->astream);
#else
	acl_msg_warn("%s(%d): dns_lookup error %s",
		myname, __LINE__, acl_last_serror());
#endif
	return (-1);
}

/* 创建DNS查询的异步流 */

static void dns_stream_open(ACL_DNS *dns)
{
	const char *myname = "dns_stream_open";
	ACL_SOCKET fd = socket(PF_INET, SOCK_DGRAM, 0);
	ACL_VSTREAM *vstream;

	if (fd == ACL_SOCKET_INVALID)
		acl_msg_fatal("%s: socket create error", myname);

	/* 创建 ACL_VSTREAM 流并设置读写接口 */

	vstream = acl_vstream_fdopen(fd, O_RDWR, 1024, 0, ACL_VSTREAM_TYPE_SOCK);
	acl_vstream_ctl(vstream,
		ACL_VSTREAM_CTL_READ_FN, dns_read,
		ACL_VSTREAM_CTL_WRITE_FN, dns_write,
		ACL_VSTREAM_CTL_CONTEXT, dns,
		ACL_VSTREAM_CTL_END);

	/* 创建异步流 */
	dns->astream = acl_aio_open(dns->aio, vstream);

	/* 设置查询套接口可读时的回调函数 */
	acl_aio_add_read_hook(dns->astream, dns_lookup_callback, dns);
	acl_aio_add_close_hook(dns->astream, dns_lookup_error, dns);
	/* 设置该异步流为持续读状态 */
	dns->astream->keep_read = 1;
}

static int dns_lookup_send(ACL_DNS *dns, ACL_DNS_REQ *handle, const char *domain)
{
	const char *myname = "dns_lookup_send";
	char  buf[1024];
	int   ret;

	memset(buf, 0, sizeof(buf));
	/* 创建DNS查询数据包 */
	ret = rfc1035BuildAQuery(domain, buf, sizeof(buf), dns->qid, NULL);
	if (ret < 0) {
		acl_msg_error("%s(%d): rfc1035BuildAQuery error for(%s)",
			myname, __LINE__, domain);
		return (-1);
	}

	/* 增加ID号 */
	dns->qid++;

	/* 发送请求DNS包 */
	acl_aio_writen(dns->astream, buf, ret);

	/* 设置定时器 */
	acl_aio_request_timer(dns->aio, dns->lookup_timeout,
		handle, dns->timeout * 1000000, 0);
	return (0);
}

/* 查询超时的回调函数 */

static void dns_lookup_timeout(int event_type, ACL_EVENT *event acl_unused,
	void *context)
{
	const char *myname = "dns_lookup_timeout";
	ACL_DNS_REQ *handle = (ACL_DNS_REQ*) context;
	ACL_DNS *dns = handle->dns;
	void (*callback)(ACL_DNS_DB*, void*, int) = handle->callback;
	void *arg = handle->ctx;

	if (event_type != ACL_EVENT_TIME) {
		acl_msg_warn("%s(%d): invalid event_type(%d)",
			myname, __LINE__, event_type);
	}

	if (++handle->nretry <= dns->retry_limit) {
		char  domain[RFC1035_MAXHOSTNAMESZ + 16], *ptr;

		ACL_SAFE_STRNCPY(domain, handle->key, sizeof(domain));
		ptr = strchr(domain, ':');
		if (ptr)
			*ptr = 0;
		if (dns_lookup_send(dns, handle, domain) == 0)
			return;
	}

	/* 释放该查询对象 */
	acl_htable_delete(handle->dns->lookup_table, handle->key, NULL);
	acl_myfree(handle);

	/* 回调函数用户的回调函数 */
	callback(NULL, arg, ACL_DNS_ERR_TIMEOUT);
}

void acl_dns_init(ACL_DNS *dns, ACL_AIO *aio, int timeout)
{
	dns->flag &= ~ACL_DNS_FLAG_ALLOC;  /* 默认为栈空间 */
	dns->aio = aio;
	dns->timeout = timeout > 0 ? timeout : 5;
	dns->qid = 0;
	dns->dns_idx = 0;
	dns->retry_limit = 0;

	/* 创建DNS服务器地址数组 */
	dns->dns_list = acl_array_create(10);
	/* 创建查询对象表 */
	dns->lookup_table = acl_htable_create(1024, 0);

	dns->lookup_timeout = dns_lookup_timeout;

	/* 打开异步读取DNS服务器响应的数据流 */
	dns_stream_open(dns);

	/* 开始异步读查询结果 */
	acl_aio_read(dns->astream);
}

ACL_DNS *acl_dns_create(ACL_AIO *aio, int timeout)
{
	ACL_DNS *dns = (ACL_DNS*) acl_mycalloc(1, sizeof(ACL_DNS));

	acl_dns_init(dns, aio, timeout);
	dns->flag |= ACL_DNS_FLAG_ALLOC;  /* 设置为堆分配的变量 */
	return (dns);
}

void acl_dns_close(ACL_DNS *dns)
{
	ACL_ITER iter;

	acl_foreach(iter, dns->lookup_table) {
		ACL_DNS_REQ *handle = (ACL_DNS_REQ*) iter.data;
		acl_myfree(handle);
	}

	acl_htable_free(dns->lookup_table, NULL);
	dns->lookup_table = NULL;
	if (dns->dns_cache) {
		acl_cache2_free(dns->dns_cache);
		dns->dns_cache = NULL;
	}
	acl_aio_iocp_close(dns->astream);
	dns->aio = NULL;
	dns->astream = NULL;
	acl_array_destroy(dns->dns_list, acl_myfree_fn);

	if (dns->groups) {
		acl_foreach(iter, dns->groups) {
			ACL_DOMAIN_GROUP *tmp = (ACL_DOMAIN_GROUP*) iter.data;
			if (tmp->excepts)
				acl_argv_free(tmp->excepts);
			acl_myfree(tmp);
		}
		acl_array_destroy(dns->groups, NULL);
	}

	if ((dns->flag & ACL_DNS_FLAG_ALLOC))
		acl_myfree(dns);
	else
		dns->flag = 0;
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
	if (dns->dns_cache)
		return;
	if (limit <= 0)
		return;
	dns->dns_cache = acl_cache2_create(limit, cache_free_fn);
}

void acl_dns_add_dns(ACL_DNS *dns, const char *dns_ip,
	unsigned short dns_port, int mask_length)
{
	const char *myname = "acl_dns_add_dns";
	ACL_DNS_ADDR *addr;

	if (mask_length >= 32 || mask_length <= 0) {
		acl_msg_error("%s(%d): mask_length(%d) invalid",
			myname, __LINE__, mask_length);
		return;
	}

	addr = (ACL_DNS_ADDR*) acl_mycalloc(1, sizeof(ACL_DNS_ADDR));
	addr->mask_length = mask_length;

	/* 设置DNS服务器地址 */

	ACL_SAFE_STRNCPY(addr->ip, dns_ip, sizeof(addr->ip));
	addr->port = dns_port;

	memset(&addr->addr, 0, sizeof(addr->addr));
	addr->addr.sin_family = AF_INET;
	addr->addr.sin_port = htons(dns_port);
	addr->addr.sin_addr.s_addr = inet_addr(dns_ip);
	addr->addr_len = sizeof(struct sockaddr_in);

	addr->in.s_addr = addr->addr.sin_addr.s_addr;
	acl_mask_addr((unsigned char*) &addr->in.s_addr,
		sizeof(addr->in.s_addr), mask_length);

	/* 将该DNS地址添加进数组中 */

	if (acl_array_append(dns->dns_list, addr) < 0)
		acl_msg_fatal("%s(%d): add dns error(%s)",
			myname, __LINE__, acl_last_serror());
}

void acl_dns_add_host(ACL_DNS *dns, const char *domain, const char *ip_list)
{
	const char *myname = "acl_dns_add_host";
	ACL_DNS_DB *dns_db;
	ACL_ARGV *argv;
	ACL_ITER iter;

	if (dns->dns_cache == NULL) {
		acl_msg_error("%s(%d): please call acl_dns_open_cache first!",
			myname, __LINE__);
		return;
	}

	dns_db = acl_netdb_new(domain);
	argv = acl_argv_split(ip_list, ",; \t");
	acl_foreach(iter, argv) {
		char *ip = (char*) iter.data;
		ACL_HOSTNAME *phost = acl_mycalloc(1, sizeof(ACL_HOSTNAME));

		ACL_SAFE_STRNCPY(phost->ip, ip, sizeof(phost->ip));
		phost->saddr.sin_family = AF_INET;
		phost->saddr.sin_addr.s_addr = inet_addr(ip);
		if (acl_array_append(dns_db->h_db, phost) < 0) {
			acl_msg_fatal("%s(%d): array append error(%s)",
				myname, __LINE__, acl_last_serror());
		}
	}

	if (acl_cache2_enter(dns->dns_cache, dns_db->name, dns_db, 0) == NULL) {
		acl_msg_fatal("%s(%d): add domain(%s) error(%s)",
			myname, __LINE__, domain, acl_last_serror());
		acl_netdb_free(dns_db);
	}
	acl_argv_free(argv);
}

void acl_dns_add_group(ACL_DNS *dns, const char *group, const char *refer,
	const char *ip_list, const char *excepts)
{
	const char *myname = "acl_dns_add_group";
	ACL_DOMAIN_GROUP *dmgrp;
	ACL_ITER iter;

	if (dns->groups == NULL)
		dns->groups = acl_array_create(10);

	acl_foreach(iter, dns->groups) {
		dmgrp = (ACL_DOMAIN_GROUP*) iter.data;
		if (strcasecmp(dmgrp->group, group) == 0) {
			acl_msg_warn("%s(%d): group(%s) already exist",
				myname, __LINE__, group);
			return;
		}
	}

	dmgrp = (ACL_DOMAIN_GROUP*) acl_mycalloc(1, sizeof(ACL_DOMAIN_GROUP));

	ACL_SAFE_STRNCPY(dmgrp->group, group, sizeof(dmgrp->group));
	acl_lowercase(dmgrp->group);
	dmgrp->group_len = strlen(dmgrp->group);

	if (refer == NULL || *refer == 0)
		ACL_SAFE_STRNCPY(dmgrp->domain, dmgrp->group, sizeof(dmgrp->domain));
	else {
		ACL_SAFE_STRNCPY(dmgrp->domain, refer, sizeof(dmgrp->domain));
		acl_lowercase(dmgrp->domain);
	}
	if (excepts)
		dmgrp->excepts = acl_argv_split(excepts, ",; \t");
	else
		dmgrp->excepts = NULL;

	acl_array_append(dns->groups, dmgrp);

	if (ip_list && *ip_list)
		acl_dns_add_host(dns, dmgrp->domain, ip_list);
}

ACL_DNS_REQ *acl_dns_lookup(ACL_DNS *dns, const char *domain_in,
	void (*callback)(ACL_DNS_DB *, void *, int), void *ctx)
{
	const char *myname = "acl_dns_lookup";
	char  key[RFC1035_MAXHOSTNAMESZ + 16], domain[RFC1035_MAXHOSTNAMESZ];
	ACL_DNS_REQ *handle;

	/* 先检查是否匹配域名组 */
	if (dns->groups) {
		ACL_DOMAIN_GROUP *dmgrp = NULL;
		ACL_ITER  iter;
		acl_foreach(iter, dns->groups) {
			ACL_ITER  iter2;
			ACL_DOMAIN_GROUP *tmp = (ACL_DOMAIN_GROUP*) iter.data;
			/* 先找到域名组对象 */
			if (acl_strrncasecmp(tmp->group, domain_in, tmp->group_len))
				continue;
			/* 检查该域名是否是域名组的例外域名 */
			if (tmp->excepts) {
				acl_foreach(iter2, tmp->excepts) {
					char *except = (char*) iter2.data;
					if (strcasecmp(except, domain_in) == 0)
						goto END_FOREACH_TAG;
				}
			}
			dmgrp = tmp;
			break;
		}

END_FOREACH_TAG:
		if (dmgrp)
			ACL_SAFE_STRNCPY(domain, dmgrp->domain, sizeof(domain));
		else
			ACL_SAFE_STRNCPY(domain, domain_in, sizeof(domain));
	} else
		ACL_SAFE_STRNCPY(domain, domain_in, sizeof(domain));

	acl_lowercase(domain);

	/* 如果打开DNS缓存功能，则优先查询缓存 */
	if (dns->dns_cache) {
		ACL_DNS_DB *dns_db;
		dns_db = acl_cache2_find(dns->dns_cache, domain);
		if (dns_db) {
			callback(dns_db, ctx, ACL_DNS_OK_CACHE);
			return (NULL);
		}
	}

	snprintf(key, sizeof(key), "%s:%d", domain, dns->qid);
	acl_lowercase(key);
	handle = (ACL_DNS_REQ*) acl_htable_find(dns->lookup_table, key);
	/* XXX: 不应存在相同的键存在, 因为该键是由域名及自动ID组成 */
	if (handle != NULL) {
		acl_msg_warn("%s(%d): key(%s) exist", myname, __LINE__, key);
		callback(NULL, ctx, ACL_DNS_ERR_EXIST);
		return (NULL);
	}

	/* 分配新的查询对象 */
	handle = (ACL_DNS_REQ*) acl_mycalloc(1, sizeof(ACL_DNS_REQ));
	handle->dns = dns;
	handle->callback = callback;
	handle->ctx = ctx;
	handle->qid = dns->qid;
	ACL_SAFE_STRNCPY(handle->key, key, sizeof(handle->key));

	/* 添加进查询对象表中 */
	if (acl_htable_enter(dns->lookup_table, key, handle) == NULL)
		acl_msg_fatal("%s(%d): enter htable error(%s)",
			myname, __LINE__, acl_last_serror());

	if (dns_lookup_send(dns, handle, domain) < 0) {
		acl_htable_delete(dns->lookup_table, key, NULL);
		acl_myfree(handle);
		callback(NULL, ctx, ACL_DNS_ERR_BUILD_REQ);
		return (NULL);
	}
	return (handle);
}

void acl_dns_cancel(ACL_DNS_REQ *handle)
{
	const char *myname = "acl_dns_cancel";

	if (handle == NULL || handle->dns == NULL) {
		acl_msg_error("%s(%d): input error", myname, __LINE__);
		return;
	}
	acl_htable_delete(handle->dns->lookup_table, handle->key, NULL);
	acl_myfree(handle);
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
	const char *unknown = "Unknown Error";
	size_t i;

	for (i = 0; errmsg[i].msg != NULL; ++i)
	{
		if (errnum == errmsg[i].errnum)
			return errmsg[i].msg;
	}
	return unknown;
}
