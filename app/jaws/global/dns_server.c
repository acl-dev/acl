#include "lib_acl.h"
#include "sys_patch.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "dns.h"

#define MSG_SEND	acl_msgio_send

/* DNS查询线程向主线程发送IO消息，因为多个线程都要用一个消息句柄发送消息，所以需要加锁 */

static void reply_lookup_msg(DNS_CTX *dns_ctx, DNS_SERVER *dns)
{
	const char *myname = "reply_lookup_msg";

	/* 加锁 */
	acl_pthread_mutex_lock(&dns->lock);

	/* 向主线程发送DNS查询结果消息 */
	if (MSG_SEND(dns_ctx->mio, DNS_MSG_LOOKUP_RESULT, dns_ctx, sizeof(DNS_CTX)) < 0) {
		acl_msg_error("%s: send msg error, domain(%s)", myname, dns_ctx->domain_key);
	}

	/* 解锁 */
	acl_pthread_mutex_unlock(&dns->lock);

	/* 释放由 msg_lookup 函数分配的内存 */
	acl_myfree(dns_ctx);
}

/* DNS查询线程，查到DNS结果会通过IO消息将数据传递给非阻塞式主线程的消息队列 */

static void lookup_thread(void *arg)
{
	const char *myname = "lookup_thread";
#ifdef USE_THREAD_POOL
	DNS_CTX *dns_ctx = (DNS_CTX *) arg;
	DNS_SERVER *dns = dns_ctx->dns;
#else
	DNS_CTX *dns_ctx = (DNS_CTX *) arg;
	DNS_SERVER *dns = dns_ctx->dns;
#endif
	int  error = 0, n, i;

	dns_ctx->dns_db = acl_gethostbyname(dns_ctx->domain_key, &error);
	if (dns_ctx->dns_db == NULL) {
		acl_msg_error("%s: gethostbyname error(%s), domain(%s)",
			myname, acl_netdb_strerror(error), dns_ctx->domain_key);
		dns_ctx->ip_cnt = 0;
		reply_lookup_msg(dns_ctx, dns);
		return;
	}

	n = acl_netdb_size(dns_ctx->dns_db);
	dns_ctx->ip_cnt = n > DNS_IP_MAX ? DNS_IP_MAX : n;

	for (i = 0; i < dns_ctx->ip_cnt; i++) {
		snprintf(dns_ctx->ip[i], sizeof(dns_ctx->ip[i]), "%s",
			acl_netdb_index_ip(dns_ctx->dns_db, i));
		dns_ctx->port[i] = 0; /* 对于DNS查询到的结果，采用默认的端口号 */
	}
	reply_lookup_msg(dns_ctx, dns);
}

/* 主线程消息处理函数: 主线程通过此函数创建DNS线程进行DNS查询过程 */

static int msg_lookup(int msg_type acl_unused, ACL_MSGIO *mio,
		  const ACL_MSGIO_INFO *info, void *arg)
{
	DNS_CTX *dns_ctx;

	dns_ctx = (DNS_CTX *) acl_mycalloc(1, sizeof(DNS_CTX));
	memcpy(dns_ctx, acl_vstring_str(info->body.buf),
		ACL_VSTRING_LEN(info->body.buf));

	/* 设置DNS句柄 */
	dns_ctx->dns = (DNS_SERVER *) arg;
	/* 设置消息句柄，线程池中的查询线程通过向此句柄发送消息以通知
	 * 主线程有关DNS的查询结果 
	 */
	dns_ctx->mio = mio;

	/* 创建单独的线程进行阻塞式DNS查询过程 */
#ifdef USE_THREAD_POOL
	acl_pthread_pool_add(dns_ctx->dns->wq, lookup_thread, dns_ctx);
#else
	lookup_thread(dns_ctx);
#endif

	return (1);
}

/* 主线程消息处理函数: 主线程通过此函数接收DNS查询线程发送的DNS查询结果消息 */

static int msg_lookup_result(int msg_type acl_unused, ACL_MSGIO *mio acl_unused,
		  const ACL_MSGIO_INFO *info, void *arg)
{
	DNS_CTX *dns_ctx;
	DNS_SERVER *dns = (DNS_SERVER*) arg;

	/* 获得DNS查询线程的结果消息 */
	dns_ctx = (DNS_CTX *) acl_vstring_str(info->body.buf);

	/* 将查询结果放入DNS缓存中 */
	if (dns_ctx->ip_cnt > 0)
		dns_cache_push(dns->dns_cache, dns_ctx->dns_db);

	/* 释放由 lookup_thread 分配的 dns_db 对象 */
	if (dns_ctx->dns_db)
		acl_netdb_free(dns_ctx->dns_db);
	dns_ctx->dns_db = NULL;

	/* 回调请求任务函数 */
	dns_ctx->callback(dns_ctx);

	return (1);
}

/* 创建DNS查询句柄 */

DNS_SERVER *dns_server_create(ACL_AIO *aio, int timeout)
{
	const char *myname = "dns_server_create";
	DNS_SERVER *dns;
	int   max_threads = 200;
	int   idle_timeout = 60 /* 此值目前不起作用，需要修改一下ACL库才可 */;

	if (aio == NULL)
		acl_msg_fatal("%s(%d): aio null", myname, __LINE__);

	/* acl 库的DNS缓存模块需要加锁，因为查询线程池都要访问该同一资源
	 * 仅让 acl 的DNS缓存模块的缓存时间为60秒，因为本程序模块也有缓存控制
	 * 因为本代码自己已经实现了DNS缓存，所以不需要ACL库的DNS缓存处理
	 * acl_netdb_cache_init(60 , 1);
	 */

	dns = (DNS_SERVER *) acl_mycalloc(1, sizeof(DNS_SERVER));
	dns->aio = aio;
	/* 本地的DNS缓存模块需要加锁，因为只有一个线程访问其资源 */
	dns->dns_cache = dns_cache_create(timeout, 0);


	/* 监听 IO 事件消息 */
	dns->listener = acl_msgio_listen(aio, NULL);
	if (dns->listener == NULL)
		acl_msg_fatal("%s: listen error", myname);

	/* 注册 IO 事件消息及处理函数：用于监听新的查询请求 */
	acl_msgio_listen_reg(dns->listener, DNS_MSG_LOOKUP, msg_lookup, dns, 1);

	acl_msgio_addr(dns->listener, dns->addr, sizeof(dns->addr));
	dns->mio = acl_msgio_connect(aio, dns->addr, 0);
	if (dns->mio == NULL)
		acl_msg_fatal("%s: connect server(%s) error", myname, dns->addr);

	/* 注册 IO 事件消息及处理函数：用于处理查询线程返回查询结果 */
	acl_msgio_reg(dns->mio, DNS_MSG_LOOKUP_RESULT, msg_lookup_result, dns);

	/* 初始化DNS共享线程锁 */
	acl_pthread_mutex_init(&dns->lock, NULL);

	/* 需要创建独立的线程程查询DNS(因为查询DNS是阻塞式查询) */
#ifdef USE_THREAD_POOL
	dns->wq = acl_thread_pool_create(max_threads, idle_timeout);
#endif
	
	return (dns);
}

/* 关闭DNS查询句柄 */

void dns_server_close(DNS_SERVER *dns)
{
	acl_msgio_close(dns->listener);
	acl_pthread_pool_destroy(dns->wq);
	acl_myfree(dns);
}

void dns_server_static_add(DNS_SERVER *dns, const char *map, const char *delim, int def_port)
{
	ACL_DNS_DB *dns_db = NULL;
	ACL_ARGV *argv = NULL;
	char *ptr;
	int   i, port;

#undef	RETURN
#define	RETURN do {  \
	if (argv != NULL)  \
		acl_argv_free(argv);  \
	if (dns_db != NULL)  \
		acl_netdb_free(dns_db);  \
	return;  \
} while (0)

	argv = acl_argv_split(map, delim);
	if (argv == NULL)
		RETURN;
	if (argv->argc < 2)
		RETURN;

	dns_db = acl_netdb_new(argv->argv[0]);
	for (i = 1; i < argv->argc; i++) {
		ptr = strchr(argv->argv[i], ':');
		if (ptr != NULL) {
			*ptr++ = 0;
			port = atoi(ptr);	
			if (port <= 0)
				port = def_port;
		} else {
			port = def_port;
		}
		acl_netdb_add_addr(dns_db, argv->argv[i], port);
	}

	/* 设置超时时间为0从而使其永不超时 */
	dns_cache_push_one(dns->dns_cache, dns_db, 0);
	RETURN;
}

/* 开始查询某个域名 */

int dns_server_lookup(DNS_SERVER *dns, const DNS_CTX *ctx)
{
	const char *myname = "dns_server_lookup";
	ACL_MSGIO *mio = dns->mio;
	DNS_CTX dns_ctx;
	ACL_DNS_DB *dns_db;

	/* 只所以采用此方式，是为了保证 dns_server_lookup 参数 ctx为 const 类型 */
	memcpy(&dns_ctx, ctx, sizeof(dns_ctx));

	/* 先查询DNS缓存表中是否存在本次所查域名 */
	dns_db = dns_cache_lookup(dns->dns_cache, ctx->domain_key);
	if (dns_db) {
		ACL_ITER iter;
		int   i = 0;

		acl_foreach(iter, dns_db) {
			const ACL_HOST_INFO *info;

			info = (const ACL_HOST_INFO*) iter.data;
			ACL_SAFE_STRNCPY(dns_ctx.ip[i], info->ip,
				sizeof(dns_ctx.ip[i]));
			dns_ctx.port[i++] = info->hport;
			dns_ctx.ip_cnt++;
			if (dns_ctx.ip_cnt >= DNS_IP_MAX)
				break;
		}

		/* 对已经DNS缓存表中已经存在的域名直接触发任务回调过程 */
		dns_ctx.callback(&dns_ctx);

		/* 此处之所以需要释放该对象，是因为 dns_cache_lookup 返回的对象为动态
		 * 分配的(acl_netdb_clone)
		 */
		acl_netdb_free(dns_db);
		return (0);
	}

	/* 向查询线程池发送查询指令 */
	
	if (MSG_SEND(mio, DNS_MSG_LOOKUP, &dns_ctx, sizeof(DNS_CTX)) < 0) {
		acl_msg_error("%s: send msg error, domain(%s)",
			myname, dns_ctx.domain_key);
		return (-1);
	}

	return (0);
}
