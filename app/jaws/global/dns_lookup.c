#include "lib_acl.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "dns.h"
#include "dns_lookup.h"

#define	STRNCPY	ACL_SAFE_STRNCPY

/*----------------------------------------------------------------------------*/

static void inner_nslookup_error(CLIENT_ENTRY *entry)
{
	if (entry->client == NULL)
		acl_msg_fatal("%s(%d): client null",
			__FILE__, __LINE__);

	if (entry->dns_errmsg) {
		acl_aio_refer(entry->client);
		acl_aio_writen(entry->client, entry->dns_errmsg,
			(int) strlen(entry->dns_errmsg));
		/* 恢复refer值 */
		acl_aio_unrefer(entry->client);
	}

	entry->nslookup_notify_fn(entry, NSLOOKUP_ERR);
}

static void inner_nslookup_ok(CLIENT_ENTRY *entry, ACL_DNS_DB *dns_db)
{
	ACL_ITER iter;
	int   i = 0;

	entry->ip_idx = 0;
	entry->dns_ctx.ip_cnt = 0;
	acl_foreach(iter, dns_db) { 
		const ACL_HOST_INFO *info;

		info = (const ACL_HOST_INFO*) iter.data;
		ACL_SAFE_STRNCPY(entry->dns_ctx.ip[i], info->ip,
			sizeof(entry->dns_ctx.ip[i]));
		entry->dns_ctx.port[i++] = info->hport;
		entry->dns_ctx.ip_cnt++;
		if (entry->dns_ctx.ip_cnt >= DNS_IP_MAX)
			break;
	}
	entry->nslookup_notify_fn(entry, NSLOOKUP_OK);
}

static void inner_nslookup_complete(ACL_DNS_DB *dns_db, void *ctx,
	int errnum acl_unused)
{
	CLIENT_ENTRY *entry = (CLIENT_ENTRY*) ctx;

	if (dns_db != NULL)
		inner_nslookup_ok(entry, dns_db);
	else
		inner_nslookup_error(entry);
}

/* 查询DNS信息，采用协议发送的方式 */
static void inner_nslookup(SERVICE *service, CLIENT_ENTRY *entry,
	const char *domain, int port)
{
	char *ptr;

	STRNCPY(entry->dns_ctx.domain_key, domain,
		sizeof(entry->dns_ctx.domain_key));
	acl_lowercase(entry->dns_ctx.domain_key);
	ptr = strchr(entry->dns_ctx.domain_key, ':');
	if (ptr)
		*ptr = 0;
	entry->server_port = port;
	if (entry->server_port <= 0)
		entry->server_port = 80;

	STRNCPY(entry->domain_key, entry->dns_ctx.domain_key,
		sizeof(entry->domain_key));
	acl_dns_lookup(service->dns_handle, entry->domain_key,
		inner_nslookup_complete, entry);
}

/*----------------------------------------------------------------------------*/

/* DNS查询结果之后的回调函数 */
static void thrpool_nslookup_complete(const DNS_CTX *dns_ctx)
{
	const char *myname = "thrpool_nslookup_complte";
	SERVICE *service = (SERVICE *) dns_ctx->context;
	DNS_RING *list;
	ACL_RING *ring_ptr;
	CLIENT_ENTRY *entry;
	time_t inter, now;

	list = (DNS_RING *) acl_htable_find(service->dns_table,
				dns_ctx->domain_key);
	if (list == NULL) {
		acl_msg_warn(NULL, "%s: domain(%s) not found maybe handled",
			myname, dns_ctx->domain_key);
		return;
	}

	time(&now);
	inter = now - dns_ctx->begin;

	/* 如果查询时间过长，则给出警告信息 */
	if (inter >= 5)
		acl_msg_warn("%s(%d): dns search time=%d, domain(%s)",
			myname, __LINE__, time(NULL) - dns_ctx->begin,
			dns_ctx->domain_key);

	while (1) {
		ring_ptr = acl_ring_pop_head(&list->ring);
		if (ring_ptr == NULL)
			break;

		list->nrefer--;

		entry = ACL_RING_TO_APPL(ring_ptr, CLIENT_ENTRY, dns_entry);

		entry->tm.dns_lookup = now - entry->tm.stamp;
		entry->tm.stamp = now;

		if (dns_ctx->ip_cnt <= 0) {
			acl_msg_error("%s(%d): dns not found domain(%s)",
				myname, __LINE__, dns_ctx->domain_key);
			if (entry->client == NULL)
				acl_msg_fatal("%s(%d): client null",
					__FILE__, __LINE__);

			if (entry->dns_errmsg) {
				/* XXX: 因为此处可能会有两处关闭 client 流的地方:
				 * acl_aio_writen 及 forward_complete，为防止重复
				 * 关闭造成的内存访问非法，需要 * 在第一个可能关闭
				 * 的函数(acl_aio_writen)调用前提升 client 流的
				 * 引用值，并且在该函数返回后再恢复引用值
				 */

				acl_aio_refer(entry->client);
				acl_aio_writen(entry->client, entry->dns_errmsg,
					(int) strlen(entry->dns_errmsg));
				/* 恢复refer值 */
				acl_aio_unrefer(entry->client);
			}

			entry->nslookup_notify_fn(entry, NSLOOKUP_ERR);

			continue;
		}

		if (acl_do_debug(20, 2)) {
			int   i;

			for (i = 0; i < dns_ctx->ip_cnt; i++)
				acl_msg_info("%s(%d): domain(%s), ip%d(%s)",
					myname, __LINE__, dns_ctx->domain_key,
					i, dns_ctx->ip[i]);
		}
		
		/* 将查得的所有DNS结果都拷贝至请求对象中 */
		memcpy(&entry->dns_ctx, dns_ctx, sizeof(entry->dns_ctx));

		/* 下面注释部分被打开，便可以测试连接重试功能:)-- zsx, 2008.2.28
		 * strcpy(proxy_entry->dns_ctx.ip[1], proxy_entry->dns_ctx.ip[0]);
		 * strcpy(proxy_entry->dns_ctx.ip[0], "127.0.0.1");
		 * if (proxy_entry->dns_ctx.ip_cnt < 2)
		 *	proxy_entry->dns_ctx.ip_cnt = 2;
		 */
		entry->ip_idx = 0;
		entry->nslookup_notify_fn(entry, NSLOOKUP_OK);
	}

	acl_htable_delete(service->dns_table, dns_ctx->domain_key, NULL);
	if (list->nrefer <= 0)
		acl_myfree(list);
	else
		acl_msg_fatal("%s(%d): list's nrefer=%d",
			myname, __LINE__, list->nrefer);
}

/* 查询DNS信息，采用外挂模块的方式 */
static void thrpool_nslookup(SERVICE *service, CLIENT_ENTRY *entry,
	const char *domain, int port)
{
	const char *myname = "thrpool_nslookup";
	DNS_CTX dns_ctx;
	DNS_RING *list;
	char *ptr;

	entry->tm.stamp = time(NULL);

	memset(&dns_ctx, 0, sizeof(dns_ctx));
	dns_ctx.begin = entry->tm.stamp;
	STRNCPY(dns_ctx.domain_key, domain, sizeof(dns_ctx.domain_key));
	ptr = strchr(dns_ctx.domain_key, ':');
	/* 仅留下域名部分 */
	if (ptr)
		*ptr = 0;

	entry->server_port = port;
	if (entry->server_port <= 0)
		entry->server_port = 80;
	
	/* 将域名字符串都转换成小写，以便于进行哈希查询 */
	acl_lowercase(dns_ctx.domain_key);

	dns_ctx.context = service;
	dns_ctx.callback = thrpool_nslookup_complete;

	STRNCPY(entry->domain_key, dns_ctx.domain_key, sizeof(entry->domain_key));

	/* 先查询DNS查询表中是否已经包含本次需要被查询的域名 */
	list = (DNS_RING *) acl_htable_find(service->dns_table, dns_ctx.domain_key);
	if (list) {
		/* 将本次对同一域名的查询添加进同一个查询链中 */
		acl_ring_prepend(&list->ring, &entry->dns_entry);
		/* 将查询链对象的引用计数加1 */
		list->nrefer++;
		/* 如果该查询链已经存在，说明有查询任务等待返回，其返回后会一同将
		 * 本次任务进行触发，如果此处触发新任务，则会造成内存访问冲突，因为
		 * 查询DNS的过程是由一组线程池进行查询的。
		 * (void) dns_server_lookup(proxy_entry->aio_proxy->dns_server, &dns_ctx);
		 */
		return;
	}

	/* 创建一个新的查询链对象，并将本次查询任务加入该查询链中及将该查询链加入查询表中 */

	list = (DNS_RING *) acl_mycalloc(1, sizeof(DNS_RING));
	acl_ring_init(&list->ring);
	STRNCPY(list->domain_key, dns_ctx.domain_key, sizeof(list->domain_key));

	/* 将本次查询任务加入新的查询链中且将查询链的引用计数加1 */
	acl_ring_prepend(&list->ring, &entry->dns_entry);
	list->nrefer++;

	/* 将新的查询链加入查询表中 */
	if (acl_htable_enter(service->dns_table, list->domain_key, (char *) list) == NULL)
		acl_msg_fatal("%s: add domain(%s) to table error", myname, list->domain_key);

	/* 开始启动DNS查询过程 */
	(void) dns_server_lookup(service->dns_server, &dns_ctx);
}

/*----------------------------------------------------------------------------*/

void dns_lookup(CLIENT_ENTRY *entry, const char *domain, int port)
{
	const char *myname = "dns_lookup";
	SERVICE *service = entry->service;

	if (acl_is_ip(domain)) {
		entry->ip_idx = 0;
		ACL_SAFE_STRNCPY(entry->dns_ctx.ip[0], domain,
			sizeof(entry->dns_ctx.ip[0]));
		entry->dns_ctx.port[0] = port;
		entry->dns_ctx.ip_cnt = 1;
		entry->nslookup_notify_fn(entry, NSLOOKUP_OK);
		return;
	} else if (service->dns_handle) {
		inner_nslookup(service, entry, domain, port);
	} else if (service->dns_server) {
		thrpool_nslookup(service, entry, domain, port);
	} else {
		acl_msg_fatal("%s(%d): no dns lookup set", myname, __LINE__);
	}
}
