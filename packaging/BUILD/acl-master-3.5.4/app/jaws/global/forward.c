#include "lib_acl.h"
#include "conn_cache.h"
#include "sys_patch.h"
#include "service.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

static int connect_close_callback(ACL_ASTREAM *astream, void *context);
static int connect_timeout_callback(ACL_ASTREAM *astream, void *context);
static int connect_callback(ACL_ASTREAM *server, void *context);

void forward_complete(CLIENT_ENTRY *entry)
{
	if (entry->server) {
		acl_aio_iocp_close(entry->server);
	}
	if (entry->client) {
		acl_aio_iocp_close(entry->client);
	}
}

/* 尝试从连接池中获得一个连接流 */

static ACL_ASTREAM *peek_server_conn(CLIENT_ENTRY *entry)
{
	ACL_ASTREAM *stream;
	char  addr[64];
	int   i;

	if (entry->service->conn_cache == NULL)
		return (NULL);
	for (i = entry->ip_idx; i < entry->dns_ctx.ip_cnt; i++) {
		snprintf(addr, sizeof(addr), "%s:%d", entry->dns_ctx.ip[i],
			entry->dns_ctx.port[i] > 0
				? entry->dns_ctx.port[i] : entry->server_port);
		stream = conn_cache_get_stream(entry->service->conn_cache, addr, NULL);
		if (stream != NULL)
			return (stream);
	}

	return (NULL);
}

/* 取得某域名的下一个服务器 IP:PORT 地址 */

static const char *next_server_addr(CLIENT_ENTRY *entry, char *buf, size_t size)
{
	const char *myname = "next_server_addr";
	SERVICE *service = entry->service;
	int   i;

	/* 试着多连一次 */
	if (entry->ip_ntry++ > entry->dns_ctx.ip_cnt) {
		acl_msg_error("%s(%d): domain(%s), ip_ntry(%d) >= ip_cnt(%d)",
			myname, __LINE__, entry->domain_key,
			entry->ip_ntry, entry->dns_ctx.ip_cnt);
		return (NULL);
	}
	if (entry->ip_idx == entry->dns_ctx.ip_cnt)
		entry->ip_idx = 0;
	for (i = entry->ip_idx; i < entry->dns_ctx.ip_cnt; i++) {
		if (service->bind_ip_list) {
			snprintf(buf, size, "%s@%s:%d",
				service->bind_ip_list[service->bind_ip_index++],
				entry->dns_ctx.ip[i],
				entry->dns_ctx.port[i] > 0
					? entry->dns_ctx.port[i] : entry->server_port);
			if (service->bind_ip_list[service->bind_ip_index] == NULL)
				service->bind_ip_index = 0;
		} else {
			snprintf(buf, size, "%s:%d", entry->dns_ctx.ip[i],
				entry->dns_ctx.port[i] > 0
					? entry->dns_ctx.port[i] : entry->server_port);
		}
		entry->ip_idx++;
		acl_debug(23, 2) ("%s(%d): domain(%s), addr(%s)",
			myname, __LINE__, entry->domain_key, buf);
		return (buf);
	}

	return (NULL);
}

static ACL_ASTREAM *forward_connect_next(CLIENT_ENTRY *entry)
{
	const char* myname = "forward_connect_next";
	SERVICE *service = entry->service;
	ACL_ASTREAM* server;
	int   i = 0;

	while (1) {
		if (next_server_addr(entry, entry->domain_addr,
				sizeof(entry->domain_addr)) == NULL)
			break;
		server = acl_aio_connect(entry->client->aio,
				entry->domain_addr, service->conn_timeout);
		if (server != NULL)
			return (server);
		acl_msg_error("%s: connect server addr(%s), domain(%s), i=%d",
			myname, entry->domain_addr, entry->domain_key, i);
		i++;
	}

	return (NULL);
}

static int connect_close_callback(ACL_ASTREAM *astream, void *context)
{
	const char* myname = "connect_close_callback";
	CLIENT_ENTRY *entry = (CLIENT_ENTRY *) context;

	/* 卸载回调函数，防止被重复调用 */
	acl_aio_ctl(astream,
		ACL_AIO_CTL_TIMEO_HOOK_DEL, connect_timeout_callback, entry,
		ACL_AIO_CTL_END);

	if (entry->flag_has_replied == 1)
		return (-1);

	if (entry->ip_idx < entry->dns_ctx.ip_cnt) {
		ACL_ASTREAM* server;

		acl_debug(23, 1) ("%s(%d): begin to connect next ip(%s:%d)",
			myname, __LINE__, entry->dns_ctx.ip[entry->ip_idx],
			entry->server_port);

		/* 断开与服务端的连接，但保持与浏览器端的连接
		 * XXX: 因为该函数将清除一些关闭回调函数，不知是否会造成某些内存泄漏？
		 * 注，此处并不关闭服务端连接，需要调用者自己来关闭
		 */
		if (entry->server && client_entry_detach(entry, acl_aio_vstream(entry->server))) {
			acl_debug(23, 1) ("%s(%d): entry's freed", myname, __LINE__);
			return (-1);
		}

		server = forward_connect_next(entry);
		if (server == NULL)
			goto CONNECT_ERROR;

		client_entry_set_server(entry, server);
		acl_aio_ctl(server,
			ACL_AIO_CTL_CONNECT_HOOK_ADD, connect_callback,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, connect_close_callback, entry,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, connect_timeout_callback, entry,
			ACL_AIO_CTL_CTX, entry,
			ACL_AIO_CTL_END);

		/* 通过返回-1，使异步流框架关闭服务端连接 */
		return (-1);
	}

CONNECT_ERROR:

	entry->tm.connect = time(NULL) - entry->tm.stamp;
	if (entry->ip_idx <= 0)
		acl_debug(23, 0) ("%s(%d): internal error, ip_idx=%d, domain(%s:%d)",
			myname, __LINE__, entry->ip_idx,
			entry->domain_key, entry->server_port);
	else
		acl_debug(23, 0) ("%s(%d): connect error, addr(%s:%d), domain(%s)",
			myname, __LINE__,
			entry->dns_ctx.ip[entry->ip_idx - 1],
			entry->server_port,
			entry->domain_key);

	entry->flag_has_replied = 1;

	if (entry->connect_error_fn)
		entry->connect_error_fn(entry);

	return (-1);
}

static int connect_timeout_callback(ACL_ASTREAM *astream, void *context)
{
	const char* myname = "connect_timeout_callback";
	CLIENT_ENTRY *entry = (CLIENT_ENTRY *) context;

	/* 卸载回调函数，防止被重复调用 */
	acl_aio_ctl(astream,
		ACL_AIO_CTL_TIMEO_HOOK_DEL, connect_timeout_callback, entry,
		ACL_AIO_CTL_END);

	if (entry->flag_has_replied == 1)
		return (-1);

	if (entry->ip_idx < entry->dns_ctx.ip_cnt) {
		ACL_ASTREAM *server;

		acl_debug(23, 1) ("%s(%d): begin to connect next ip(%s:%d)",
			myname, __LINE__, entry->dns_ctx.ip[entry->ip_idx],
			entry->server_port);

		/* 断开与服务端的连接，但保持与浏览器端的连接
		 * XXX: 因为该函数将清除一些关闭回调函数，不知是否会造成某些内存泄漏？
		 * 注，此处并不关闭服务端连接，需要调用者自己来关闭
		 */
		if (client_entry_detach(entry, acl_aio_vstream(entry->server)) == 1) {
			acl_debug(3, 1) ("%s(%d): entry is freed", myname, __LINE__);
			return (-1);
		}

		server = forward_connect_next(entry);
		if (server == NULL)
			goto CONNECT_ERROR;

		client_entry_set_server(entry, server);
		acl_aio_ctl(server,
			ACL_AIO_CTL_CONNECT_HOOK_ADD, connect_callback,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, connect_close_callback, entry,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, connect_timeout_callback, entry,
			ACL_AIO_CTL_CTX, entry,
			ACL_AIO_CTL_END);

		/* 通过返回-1，使异步流框架关闭服务端连接 */
		return (-1);
	}

CONNECT_ERROR:

	entry->tm.connect = time(NULL) - entry->tm.stamp;
	if (entry->ip_idx <= 0)
		acl_debug(23, 0) ("%s(%d): internal error, ip_idx=0, domain(%s:%d)",
			myname, __LINE__, entry->domain_key, entry->server_port);
	else
		acl_debug(23, 0) ("%s(%d): connect timeout, addr(%s:%d)",
			myname, __LINE__,
			entry->dns_ctx.ip[entry->ip_idx - 1],
			entry->server_port);

	entry->flag_has_replied = 1;

	if (entry->connect_timeout_fn)
		entry->connect_timeout_fn(entry);

	return (-1);
}

static int connect_callback(ACL_ASTREAM *server, void *context)
{
	CLIENT_ENTRY *entry = (CLIENT_ENTRY*) context;

	entry->tm.connect = time(NULL) - entry->tm.stamp;
	return (entry->connect_notify_fn(entry));
}

void forward_start(CLIENT_ENTRY *entry)
{
	const char *myname = "forward_start";
	ACL_ASTREAM *server;

	/* 先从连接池中尝试一个连接流 */
	server = peek_server_conn(entry);
	if (server == NULL) {
		/* 如果连接池中没有可利用的连接流，则开始连接服务端 */
		server = forward_connect_next(entry);
		if (server == NULL) {
			acl_msg_error("%s: connect server_addr(%s:%d) error(%s)",
				myname, entry->domain_key, entry->server_port,
				acl_last_serror());
#if 1
			if (entry->connect_error_fn)
				entry->connect_error_fn(entry);
#else
			forward_complete(entry);
#endif
			return;
		}
		entry->flag_conn_reuse = 0;

		client_entry_set_server(entry, server);
		acl_aio_ctl(server,
			ACL_AIO_CTL_CONNECT_HOOK_ADD, connect_callback,
			ACL_AIO_CTL_CTX, entry,
			ACL_AIO_CTL_CLOSE_HOOK_ADD, connect_close_callback, entry,
			ACL_AIO_CTL_TIMEO_HOOK_ADD, connect_timeout_callback, entry,
			ACL_AIO_CTL_END);

	} else {
		/* 复用连接池中的连接 */

		entry->flag_conn_reuse = 1;
		client_entry_set_server(entry, server);
		acl_aio_ctl(server, ACL_AIO_CTL_CTX, entry, ACL_AIO_CTL_END);
		connect_callback(server, entry);
	}
}
