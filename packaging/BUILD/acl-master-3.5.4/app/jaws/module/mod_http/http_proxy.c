#include "lib_acl.h"
#include "lib_protocol.h"
#include "assert.h"
#include "dns_lookup.h"
#include "conn_cache.h"
#include "service.h"
#include "http_module.h"
#include "http_service.h"

#ifdef ACL_MS_WINDOWS
#include <process.h>
#define getpid _getpid
#endif

static int http_proxy_next(HTTP_CLIENT *http_client);
static void http_proxy_req_get(HTTP_CLIENT *http_client);

/*---------------------------------------------------------------------------*/

/* 当前请求处理完毕, 是否继续下一个客户端请求? */

static void http_proxy_server_complete(HTTP_CLIENT *http_client, int keep_alive)
{
	HTTP_SERVICE *service = (HTTP_SERVICE*) http_client->entry.service;
	ACL_ASTREAM *server;
	ACL_VSTREAM *sstream;

	if (http_client->res) {
		http_res_free(http_client->res);
		http_client->res = NULL;
		http_client->hdr_res = NULL;
	} else if (http_client->hdr_res) {
		http_hdr_res_free(http_client->hdr_res);
		http_client->hdr_res = NULL;
	}

	server = http_client->entry.server;
	/* 是否应与服务端保持长连接? */
	if (server == NULL) {
		return;
	}

	/* 先禁止对该异步流继续监听 */
	acl_aio_disable_readwrite(server);

	/* 清除异步流的所有勾子回调函数，防止在流结束后回调被调用 */
	acl_aio_clean_hooks(server);

	sstream = acl_aio_vstream(server);

	if (keep_alive) {
		int   timeout = 60;
		/* 与服务端流分离 */
		client_entry_detach(&http_client->entry, sstream);
		ACL_VSTRING_RESET(&server->strbuf);

		/* 将与服务端的连接流置入连接池中 */
		conn_cache_push_stream(service->service.conn_cache,
				server, timeout, NULL, NULL);
	} else {
		client_entry_detach(&http_client->entry, sstream);
		/* 关闭异步流 */
		acl_aio_iocp_close(server);
	}
}

static void http_proxy_client_complete(HTTP_CLIENT *http_client, int keep_alive)
{
	ACL_ASTREAM *client;

	/* 重置重试次数 */
	http_client->entry.nretry_on_error = 0;
	http_client->entry.ip_ntry = 0;

	/* 清除本次会话完成标志位 */
	http_client->flag &= ~HTTP_FLAG_FINISH;

	client = http_client->entry.client;
	if (client == NULL) {
		return;
	}

	/* 清除异步流的所有勾子回调函数，防止在流结束后回调被调用 */
	acl_aio_clean_hooks(client);

	/* 是否应与客户端保持长连接? */
	if (keep_alive) {
		if (http_client->req_curr) {
			http_client_req_free(http_client->req_curr);
			http_client->req_curr = NULL;
		}
		http_proxy_next(http_client);
	} else {
		ACL_VSTREAM *cstream = acl_aio_vstream(client);

		if (http_client->req_curr) {
			http_client_req_free(http_client->req_curr);
			http_client->req_curr = NULL;
		}
		acl_aio_disable_readwrite(client);
		/* 与客户端流分离 */
		client_entry_detach(&http_client->entry, cstream);
		/* 关闭老的异步流 */
		acl_aio_iocp_close(client);
	}
}

static void http_proxy_complete(HTTP_CLIENT *http_client, int error_happen)
{
	/* 需要提前知道服务端流和客户端流是否已经被分离，因为下面经过
	 * http_proxy_server_complete 或 http_proxy_client_complte 后
	 * http_client 所占内存可能已经被释放，这样提前知道服务端/客户
	 * 端流的状态可以避免内存非法访问
	 */
	int  server_null = http_client->entry.server == NULL;
	int  client_null = http_client->entry.client == NULL;

	/* 判定服务端流是否应保持长连接 */

	if (var_cfg_http_server_keepalive && !error_happen
		&& http_client->hdr_res
		&& http_client->hdr_res->hdr.keep_alive)
	{
		http_client->flag |= HTTP_FLAG_SERVER_KEEP_ALIVE;
	}

	/* 判定客户端流是否应保持长连接 */

	if (var_cfg_http_client_keepalive && !error_happen
		&& (http_client->flag & HTTP_FLAG_SERVER_KEEP_ALIVE)
		&& http_client->req_curr
		&& http_client->req_curr->hdr_req
		&& http_client->req_curr->hdr_req->hdr.keep_alive)
	{
		http_client->flag |= HTTP_FLAG_CLIENT_KEEP_ALIVE;
	} else {
		http_client->flag &= ~HTTP_FLAG_CLIENT_KEEP_ALIVE;
	}

	/* 如果服务端流处于锁定状态则不立即关闭服务端流 */

	if (!server_null && !(http_client->flag & HTTP_FLAG_SERVER_LOCKED)) {
		http_proxy_server_complete(http_client,
			(http_client->flag & HTTP_FLAG_SERVER_KEEP_ALIVE));
	}

	/* 如果客户端流处于锁定状态则不立即关闭客户端流 */

	if (!client_null && !(http_client->flag & HTTP_FLAG_CLIENT_LOCKED)) {
		http_proxy_client_complete(http_client,
			(http_client->flag & HTTP_FLAG_CLIENT_KEEP_ALIVE));
	}
}

/*---------------------------------------------------------------------------*/

/* 发送响应数据至客户端，为了减少IO次数，合并响应头同响应体数据一起发送 */

static void send_to_client(HTTP_CLIENT *http_client, char *data, int dlen)
{
	int   hdr_len = (int) LEN(http_client->buf);

	/* 是否连同HTTP响应头一起发送? */
	if (hdr_len > 0) {
		/* 将HTTP响应头和一部分数据体一起发送,
		 * 这样可以减少 IO 写次数
		 */

		struct iovec iov[2];

		iov[0].iov_base = STR(http_client->buf);
		iov[0].iov_len  = hdr_len;
		iov[1].iov_base = (char*) data;
		iov[1].iov_len  = dlen;

		/* 必须提交将缓冲区提前复位，但不影响内部数据 */
		ACL_VSTRING_RESET(http_client->buf);
		/* 将客户端流加锁，防止被提前关闭 */
		http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;
		acl_aio_writev(http_client->entry.client, iov, 2);
	} else {
		/* 响应头已经发送，此处仅发送响应体部分数据 */

		/* 将客户端流加锁，防止被提前关闭 */
		http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;
		acl_aio_writen(http_client->entry.client, data, dlen);
	}
}

/* 获得服务器HTTP数据体并发送至浏览器 */

static void forward_respond_body_data(HTTP_CLIENT *http_client,
	char *data, int dlen)
{
	const char *data_saved = data;
	char *data_ptr = data;
	char *ptr;
	ACL_ITER iter;
	plugin_dat_free_fn last_plugin_free = NULL;
	char *last_plugin_buf = NULL;
	void *plugin_res_ctx = http_client->plugin_res_ctx;
	HTTP_SERVICE *service = (HTTP_SERVICE*) http_client->entry.service;

	/* 遍历所有的数据体过滤器 */
	acl_foreach(iter, &service->respond_dat_plugins) {
		HTTP_PLUGIN *tmp = (HTTP_PLUGIN*) iter.data;
		int  stop = 0, ret;
		ptr = tmp->data_filter(data_ptr, dlen, &ret, &stop, plugin_res_ctx);

		/* 释放前一个过滤器分配的动态内存 */
		if (last_plugin_buf && last_plugin_buf != data_saved && last_plugin_free)
			last_plugin_free(last_plugin_buf, plugin_res_ctx);

		dlen = ret;
		data = data_ptr = ptr;
		last_plugin_buf = ptr;
		last_plugin_free = tmp->data_free;

		if (ret < 0 || ptr == NULL) {
			ret = -1;
			data = NULL;
			break;
		} else if (stop)
			break;
	}

	/* 向客户端写数据 */
	if (dlen > 0 && data)
		send_to_client(http_client, data, dlen);

	/* 释放前一个过滤器分配的动态内存 */
	if (last_plugin_buf && last_plugin_buf != data_saved && last_plugin_free)
		last_plugin_free(last_plugin_buf, plugin_res_ctx);
}

/* 成功从服务器读到响应体数据的回调函数 */

static int read_respond_body_ready(int status, char *data, int dlen, void *arg)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) arg;

acl_msg_info("%s(%d)", __FUNCTION__, __LINE__); /* only for test */
	if (data == NULL || dlen <= 0) {
		/* 取消服务端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;
		/* 设置会话完成标志位 */
		http_client->flag |= HTTP_FLAG_FINISH;
		http_proxy_complete(http_client, -1);
		return (-1);
	}

	http_client->total_size += dlen;

	/* client 流有可能被提前关闭了 */
	if (http_client->entry.client == NULL) {
		/* 取消服务端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;
		/* 设置会话完成标志位 */
		http_client->flag |= HTTP_FLAG_FINISH;
		http_proxy_complete(http_client, -1);
		return (-1);
	}

	if (status >= HTTP_CHAT_ERR_MIN) {
		/* 取消服务端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;
		/* 设置会话完成标志位 */
		http_client->flag |= HTTP_FLAG_FINISH;
		http_proxy_complete(http_client, -1);
		return (-1);
	} else if (status == HTTP_CHAT_OK) {
		/* 设置会话完成标志位 */
		http_client->flag |= HTTP_FLAG_FINISH;
#if 1
		/* 取消服务端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;
#endif
	}

	/* 如果 HTTP_FLAG_FINISH 标志设置，则会在 forward_respond_body_data 之后
	 * 回调函数 send_respond_body_complete 调用 http_proxy_complete
	 */
	forward_respond_body_data(http_client, data, dlen);
#if 0
	if (status == HTTP_CHAT_OK) {
		/* 取消服务端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;
		http_proxy_complete(http_client, 0);
	}
#endif

	return (0);
}

/* 发送响应体数据至客户端, 如果确定已经发送完最后一批数据则触发结束过程 */

static int send_respond_body_complete(ACL_ASTREAM *client acl_unused, void *ctx)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) ctx;

	/* 取消客户端流的锁定状态, 从而允许当服务流异常关闭时可以在
	 * on_close_server 等函数里调用 http_proxy_complete 时里关闭客户流!
	 */
	http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;

	/* 如果是最后的数据则完成本次会话过程 */
	if ((http_client->flag & HTTP_FLAG_FINISH)) {
		if ((http_client->flag & HTTP_FLAG_SERVER_CLOSED))
			http_proxy_complete(http_client, -1);
		else
			http_proxy_complete(http_client, 0);
	}
	return (0);
}

/* 传输服务器响应HTTP数据体至浏览器
 * 调用该过程的函数需要注意流关闭保护措施
 */

static void forward_respond_hdr_body(HTTP_CLIENT *http_client)
{
	/* 创建HTTP响应体对象 */
	http_client->res = http_res_new(http_client->hdr_res);

	/* 如果向客户流发送响应体失败会自动调用在 send_request_hdr_complete
	 * 里针对客户流设置的回调函数 on_close_clinet
	 */

	/* 设置向客户流发送数据成功的回调函数 */
	acl_aio_add_write_hook(http_client->entry.client,
		send_respond_body_complete, http_client);

	/* 将服务端流置于锁定状态, 从而防止被提前关闭 */
	http_client->flag |= HTTP_FLAG_SERVER_LOCKED;

acl_msg_info("%s(%d)", __FUNCTION__, __LINE__); /* only for test */
	/* 开始从服务器读取HTTP数据体数据 */
	http_res_body_get_async(http_client->res,
		http_client->entry.server,
		read_respond_body_ready,
		http_client,
		http_client->entry.service->rw_timeout);
}

/* 发送服务器响应头至浏览器 */

static int send_respond_hdr_complete(ACL_ASTREAM *client acl_unused, void *ctx)
{
	const char *myname = "send_respond_hdr_complete";
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) ctx;

	/* 取消客户端流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;

	if (http_client->hdr_res == NULL) {
		acl_msg_error("%s(%d): http_client->hdr_res null", myname, __LINE__);
		http_proxy_complete(http_client, -1);
		return (0);
	}

	/* 因为进入此函数后 client 的引用值已经被 acl_aio_xxx 自动加1了，
	 * 所以也许不必担心重复关闭流的现象发生
	 */
	http_proxy_complete(http_client, 0);
	return (0);
}

/* 仅 forward 响应头，因为没有响应体 */

static void forward_respond_hdr(HTTP_CLIENT *http_client)
{
	acl_aio_add_write_hook(http_client->entry.client,
		send_respond_hdr_complete, http_client);

	/* 设定客户端流为锁定状态 */
	http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;

	acl_aio_writen(http_client->entry.client,
		acl_vstring_str(http_client->buf),
		(int) ACL_VSTRING_LEN(http_client->buf));
}

static void start_forward_respond(HTTP_CLIENT *http_client)
{
	/* 是否与服务端保持长连接? */
	if (!var_cfg_http_client_keepalive) {
		http_hdr_entry_replace(&http_client->hdr_res->hdr,
			"Connection", "close", 1);
		http_hdr_entry_replace(&http_client->hdr_res->hdr,
			"Proxy-Connection", "close", 0);
	} else if (http_client->req_curr->hdr_req->hdr.keep_alive
		&& http_client->hdr_res->hdr.keep_alive)
	{
		http_hdr_entry_replace(&http_client->hdr_res->hdr,
			"Connection", "keep-alive", 1);
		http_hdr_entry_replace(&http_client->hdr_res->hdr,
			"Proxy-Connection", "keep-alive", 0);
	}

	/* 重新组成HTTP响应头 */
	http_hdr_build(&http_client->hdr_res->hdr, http_client->buf);

	/* 对于 3xx, 4xx 的服务器响应，不应有数据体部分 */

	if (http_client->hdr_res->hdr.content_length == 0
	    || (http_client->hdr_res->hdr.content_length == -1
		&& !http_client->hdr_res->hdr.chunked
		&& http_client->hdr_res->reply_status > 300
		&& http_client->hdr_res->reply_status < 400))
	{
		/* 如果没有数据体，则仅返回数据头 */
		forward_respond_hdr(http_client);
		return;
	}

	/* 将数据头连同一部分数据体一起发送给客户端，从而减少io次数 */

	/* xxx: 对于没有 content-length 或 content-length > 0
	 * 及服务器响应状态码不为 3xx, 4xx 的情况
	 */
	forward_respond_hdr_body(http_client);
}

/**
 * 针对HTTP响应头的过滤器处理过程，如果根据该响应头过滤器决定完全接管
 * 该响应则主程序不再处理该服务端流及客户端流
 * 返回 0 表示所有过滤器均不接管该响应, 否则表示接管
 */
static int reply_plugin_takeover(HTTP_CLIENT *http_client)
{
	const char *myname = "reply_plugin_takeover";
	HTTP_SERVICE *service = (HTTP_SERVICE*) http_client->entry.service;
	HTTP_PLUGIN *plugin = NULL;
	ACL_ITER iter;

	/* xxx: plugin_res_ctx 该参数在每次请求都有可能不一样, 外挂模块应该自行管理 */
	http_client->plugin_res_ctx = NULL;

	/* 遍历所有的插件回调处理函数 */

	acl_foreach(iter, &service->respond_plugins) {
		ACL_ASTREAM *client = http_client->entry.client;
		ACL_ASTREAM *server = http_client->entry.server;
		ACL_VSTREAM *client_stream = acl_aio_vstream(client);
		ACL_VSTREAM *server_stream = acl_aio_vstream(server);
		HTTP_PLUGIN *tmp = (HTTP_PLUGIN*) iter.data;

		if (tmp->filter.respond(client_stream, server_stream,
			http_client->req_curr->hdr_req,
			http_client->hdr_res,
			&http_client->plugin_res_ctx))
		{
			plugin = tmp;
			break;
		}
	}

	if (plugin && plugin->forward.respond) {
		ACL_ASTREAM *client = http_client->entry.client;
		ACL_ASTREAM *server = http_client->entry.server;
		ACL_VSTREAM *client_stream = acl_aio_vstream(client);
		ACL_VSTREAM *server_stream = acl_aio_vstream(server);
		HTTP_HDR_REQ *hdr_req;
		HTTP_HDR_RES *hdr_res;
		void *plugin_res_ctx = http_client->plugin_res_ctx;

		/* 将 http_client 中的 hdr_req/hdr_res 置空 */
		if (http_client->req_curr) {
			hdr_req = http_client->req_curr->hdr_req;
			http_client->req_curr->hdr_req = NULL;
			if (http_client->req_curr->req)
				http_client->req_curr->req->hdr_req = NULL;
		} else {
			acl_msg_fatal("%s(%d): req_curr null", myname, __LINE__);
			/* XXX: can't reach here just avoid compiling warning */
			hdr_req = NULL;
		}
		hdr_res = http_client->hdr_res;
		http_client->hdr_res = NULL;

		/* 将客户端数据流与该代理对象分离 */
		client_entry_detach(&http_client->entry, client_stream);

		/* 将服务端数据与该代理对象分离 */
		/* 因为 entry 代理对象的引用计数为0，所以其会在该分离函数
		 * 中自动被释放
		 */
		client_entry_detach(&http_client->entry, server_stream);

		/* 禁止异步流的读/写监控 */
		acl_aio_disable_readwrite(client);
		acl_aio_disable_readwrite(server);

		/* 清除回调函数 */
		acl_aio_clean_hooks(client);
		acl_aio_clean_hooks(server);

		/* 将客户端异步流的数据流置空 */
		acl_aio_ctl(client, ACL_AIO_CTL_STREAM, NULL, ACL_AIO_CTL_END);
		/* 将服务端异步流的数据流置空 */
		acl_aio_ctl(server, ACL_AIO_CTL_STREAM, NULL, ACL_AIO_CTL_END);

		/* xxx: 异步关闭 client/server 异步流 */
		acl_aio_iocp_close(client);
		acl_aio_iocp_close(server);

		/* 必须流由非阻塞模式转换为阻塞模式 */
		acl_non_blocking(ACL_VSTREAM_SOCK(client_stream), ACL_BLOCKING);
		acl_non_blocking(ACL_VSTREAM_SOCK(server_stream), ACL_BLOCKING);

		/* 调用在非阻塞通信时设置的关闭回调函数并清除之 */
		acl_vstream_call_close_handles(client_stream);
		acl_vstream_call_close_handles(server_stream);

		/* 至此，已经将客户端数据流由非阻塞模式转换为阻塞模式，同时关闭
		 * 了与服务端的连接流，将该连接请求转给相关代理模块处理，异步
		 * 代理不再代理该客户端的请求及服务端的响应
		 * 注意：client_stream, hdr_res, hdr_res 此处并未释放，
		 * 需要下载代理模块下载完毕后自己单独释放
		 */
		plugin->forward.respond(client_stream, server_stream,
				hdr_req, hdr_res, plugin_res_ctx);
		return (1);
	}

	return (0);
}

static int http_request_reforward(HTTP_CLIENT *http_client);
static int read_respond_hdr_timeout(ACL_ASTREAM *server, void *ctx);
static int read_respond_hdr_error(ACL_ASTREAM *server, void *ctx);

/* 获得服务器响应头 */
static void begin_read_respond(HTTP_CLIENT *http_client);

static int get_respond_hdr_ready(int status, void *arg)
{
	const char *myname = "get_respond_hdr_ready";
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) arg;
	ACL_ASTREAM *client = http_client->entry.client;
	ACL_ASTREAM *server = http_client->entry.server;
 
	/* 取消服务流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;

	/* xxx: sanity check */

	if (client == NULL) {
		acl_msg_warn("%s: client null(%s)", myname,
			http_client->flag & HTTP_FLAG_FINISH
			? "finished" : "not finished");
		http_client->flag |= HTTP_FLAG_FINISH;
		http_proxy_complete(http_client, -1);
		return (0);
	}

	/* 需要关闭两个回调函数，防止触发 read_respond_hdr_error
	 * 和 read_respond_hdr_timeout 过程(read_respond_hdr_timeout 会触发
	 * read_respond_hdr_error), 而在 read_respond_hdr_error 里会调用
	 * http_request_reforward
	 */
	acl_aio_ctl(server,
		ACL_AIO_CTL_CLOSE_HOOK_DEL, read_respond_hdr_error, http_client,
		ACL_AIO_CTL_TIMEO_HOOK_DEL, read_respond_hdr_timeout, http_client,
		ACL_AIO_CTL_END);

acl_msg_info("%s(%d)", __FUNCTION__, __LINE__); /* only for test */
	if (status != HTTP_CHAT_OK) {
		/* 如果读响应头出现错误则需要重试 */

		/* 进行重试 */
		if (http_request_reforward(http_client) == 0) {
			/* 如果已经开始重试过程，则直接返回 */
			return (0);
		}

		http_proxy_complete(http_client, -1);
		/* xxx: 应该返回 5xx 信息给客户端 */
		return (0);
	}

	/* 分析 HTTP 响应头 */

	if (http_hdr_res_parse(http_client->hdr_res) < 0) {
		/* 如果分析响应头失败则需要重试 */

		acl_msg_error("%s: parse hdr_res error", myname);
		/* 进行重试 */
		if (http_request_reforward(http_client) == 0) {
			/* 如果已经开始重试过程，则直接返回 */
			return (0);
		}

		http_proxy_complete(http_client, -1);
		/* xxx: 应该返回 5xx 信息给客户端 */
		return (0);
	}

	/* 忽略 100 continue 的回应 */
	if (http_client->hdr_res->reply_status == 100) {
		begin_read_respond(http_client);
		return (0);
	}

	/* 判断是否需要由其它代理模块接管 */
	if (reply_plugin_takeover(http_client)) {
		return (0);
	}

	/* 开始转发服务器返回的数据给客户端 */
	start_forward_respond(http_client);
	return (0);
}

static int read_respond_hdr_timeout(ACL_ASTREAM *server, void *ctx)
{
	const char *myname = "read_respond_hdr_timeout";
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) ctx;

	/* 取消服务流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;

	/* 取消 HTTP 响应头的读关闭回调函数 */
	acl_aio_clean_close_hooks(server);

	if (http_client->entry.client == NULL) {
		acl_msg_warn("%s(%d): client null", myname, __LINE__);
		http_proxy_complete(http_client, -1);
		/* 必须返回 -1, 因为不希望继续调用其它的超时回调函数 */
		return (-1);
	}

	/* 进行重试 */
	if (http_request_reforward(http_client) == 0) {
		/* 如果已经开始重试过程，则直接返回 */
		/* 必须返回 -1, 因为不希望继续调用其它的超时回调函数 */
		return (-1);
	}

	/* 锁定客户端流 */
	http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;

	/* 返回给客户端读服务端响应超时信息 */
	acl_aio_writen(http_client->entry.client,
		HTTP_REPLY_TIMEOUT, (int) strlen(HTTP_REPLY_TIMEOUT));

	/* 解锁客户端流 */
	http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;

	http_proxy_complete(http_client, -1);
	/* 必须返回 -1, 因为不希望继续调用其它的超时回调函数 */
	return (-1);
}

static int read_respond_hdr_error(ACL_ASTREAM *server acl_unused, void *ctx)
{
	const char *myname = "read_respond_hdr_error";
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) ctx;

	/* 取消服务流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;

	if (http_client->entry.client == NULL) {
		acl_msg_warn("%s(%d): client null", myname, __LINE__);
		/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
		http_proxy_complete(http_client, -1);
		return (-1);
	}

	/* 进行重试 */
	if (http_request_reforward(http_client) == 0) {
		/* 如果已经开始重试过程，则直接返回 */
		/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
		return (-1);
	}

	/* 锁定客户端流 */
	http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;

	/* 返回给客户端读服务端响应出错信息 */
	acl_aio_writen(http_client->entry.client,
		HTTP_REPLY_ERROR, (int) strlen(HTTP_REPLY_ERROR));

	/* 解锁客户端流 */
	http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;

	http_proxy_complete(http_client, -1);
	/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
	return (-1);
}

/* XXX: 该函数需要处于关闭保护状态，即调用此函数的函数需要对服务端流加保护措施 */

static void begin_read_respond(HTTP_CLIENT *http_client)
{
	/* 生成一个 HTTP 响应头 */
	http_client->hdr_res = http_hdr_res_new();

	/* 设定服务流的锁定状态 */
	http_client->flag |= HTTP_FLAG_SERVER_LOCKED;

	/* 设置从服务器的读错误及读超时的回调函数 */
	acl_aio_ctl(http_client->entry.server,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, read_respond_hdr_error, http_client,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, read_respond_hdr_timeout, http_client,
		ACL_AIO_CTL_END);

	/* 开始读服务端的 HTTP 响应头 */
	http_hdr_res_get_async(http_client->hdr_res,
		http_client->entry.server,
		get_respond_hdr_ready,
		http_client,
		http_client->entry.service->rw_timeout);
}

/*----------------------------------------------------------------------------*/

/* 发送最后请求数据至服务器的回调函数, 至此函数，通信方向发生
 * 改变，由原来的从客户流读数据、向服务流写数据变为从服务流读
 * 数据、向客户流写数据
 */

static int send_request_body_complete(ACL_ASTREAM *server, void *context)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) context;

	/* 取消服务流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;

	/* xxx: sanity check */
	if (http_client->entry.client == NULL) {
		http_proxy_complete(http_client, -1);
		return (0);
	}

	/* 如果请求体数据发送完毕则开始读取服务器响应 */
	if ((http_client->flag & HTTP_FLAG_REQEND)) {
		/* 取消之前设置的发送请求体成功的回调函数 */
		acl_aio_del_write_hook(server, send_request_body_complete,
				http_client);
		http_client->flag &= ~HTTP_FLAG_REQEND;
		/* 开始读取服务端的响应数据 */
		begin_read_respond(http_client);
	}
	return (0);
}

/* 读到一些HTTP请求体数据 */

static int read_request_body_ready(int status, char *data, int dlen, void *arg)
{
	const char *myname = "read_request_body_ready";
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) arg;

	if (data == NULL || dlen <= 0) {
		acl_msg_error("%s(%d): data: %s, dlen: %d",
			myname, __LINE__, data ? "not null" : "null", dlen);
		/* 取消客户流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;
		/* 设置请求过程完毕标志位 */
		http_client->flag |= HTTP_FLAG_REQEND;
		http_proxy_complete(http_client, -1);
		return (0);
	}

	if (http_client->entry.server == NULL) {
		/* 有可能在向服务端写数据时出错而触发了 on_close_server 过程,
		 * 从而导致 http_proxy_complete 过程被调用
		 */

		/* 取消客户流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;
		/* 设置请求过程完毕标志位 */
		http_client->flag |= HTTP_FLAG_REQEND;
		http_proxy_complete(http_client, -1);
		return (0);
	}

	if (status >= HTTP_CHAT_ERR_MIN) {
		/* 取消客户端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;
		/* 设置请求过程完毕标志位 */
		http_client->flag |= HTTP_FLAG_REQEND;
		http_proxy_complete(http_client, -1);
		return (0);
	} else if (status == HTTP_CHAT_OK) {
		/* 已经读完了浏览器本次会话的请求数据 */

		/* 因为已经从客户端读完了本次会话的请求数据，所以此处可以
		 * 取消客户端流锁定状态, 以允许当向服务端写数据出错时可以在
		 * on_close_server 中关闭客户端流
		 */
		http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;
		/* 设置请求过程完毕标志位 */
		http_client->flag |= HTTP_FLAG_REQEND;
	}

	/* 设定服务端流的锁定状态 */
	http_client->flag |= HTTP_FLAG_SERVER_LOCKED;
	/* 将来自于浏览器的数据体部分发送至服务器 */
	acl_aio_writen(http_client->entry.server, data, dlen);
	return (0);
}

/* 如果有请求体则转发请求体数据至服务器 */

static void forward_request_body(HTTP_CLIENT *http_client)
{
	/* 根据请求头对象生成请求体对象 */
 	http_client->req_curr->req = http_req_new(http_client->req_curr->hdr_req);

	/* 设置发送请求体成功的回调函数 */
	acl_aio_add_write_hook(http_client->entry.server,
		send_request_body_complete, http_client);

	/* 将客户端流置于锁定状态, 从而防止被提前关闭 */
	http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;

	 /* 开始读客户端请求体数据 */
	http_req_body_get_async(http_client->req_curr->req,
		http_client->entry.client,
		read_request_body_ready,
		http_client,
		http_client->entry.service->rw_timeout);
}

/* 发送请求头至服务器时出错的回调函数 */
static int send_request_hdr_complete(ACL_ASTREAM *server, void *ctx);

static int send_request_hdr_error(ACL_ASTREAM *server acl_unused, void *ctx)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) ctx;

	/* 取消服务流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;

	/* 如果仅是传输请求头时出错，则可以进行重试 */
	if (http_request_reforward(http_client) == 0) {
		/* 如果已经开始重试过程，则直接返回 */
		/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
		return (-1);
	}

	/* 防止向客户流写数据出错时提前关闭客户流 */
	acl_aio_refer(http_client->entry.client);

	/* 返回给客户端读服务端响应出错信息 */
	acl_aio_writen(http_client->entry.client,
		HTTP_SEND_ERROR, (int) strlen(HTTP_SEND_ERROR));

	/* 恢复客户流为可关闭状态 */
	acl_aio_unrefer(http_client->entry.client);

	/* 该会话完毕 */
	http_proxy_complete(http_client, -1);
	/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
	return (-1);
}

/* 发送请求头至服务器成功时的回调函数 */

static int send_request_hdr_complete(ACL_ASTREAM *server acl_unused, void *ctx)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT *) ctx;

	/* 取消服务流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;

	/* 关闭上次注册的写完成及出错的回调函数 */
	acl_aio_ctl(http_client->entry.server,
		ACL_AIO_CTL_WRITE_HOOK_DEL, send_request_hdr_complete, http_client,
		ACL_AIO_CTL_CLOSE_HOOK_DEL, send_request_hdr_error, http_client,
		ACL_AIO_CTL_END);

	if (http_client->req_curr->hdr_req->hdr.content_length > 0) {
		/* 如果有请求体，则读取客户端请求体数据 */
		forward_request_body(http_client);
	} else {
		/* 没有请求体，则开始读服务端的返回数据 */
		begin_read_respond(http_client);
	}

	return (0);
}

/* 重新构建HTTP请求头 */

static void rebuild_request(HTTP_HDR_REQ *hdr_req, ACL_VSTRING *buf)
{
	ACL_ITER iter;
	HTTP_HDR_ENTRY *entry;
	int   i = 0;

	/* XXX: nginx 有时对含有 Proxy-Connection 的请求有时会有延迟? */
#if 0
	http_hdr_entry_off(&hdr_req->hdr, "Proxy-Connection");
#endif

#if 0
	acl_vstring_sprintf(buf, "%s http://%s%s HTTP/%d.%d\r\n",
		hdr_req->method, hdr_req->host,
		acl_vstring_str(hdr_req->url_part),
		hdr_req->hdr.version.major,
		hdr_req->hdr.version.minor);
#else
	acl_vstring_sprintf(buf, "%s %s HTTP/%d.%d\r\n",
		hdr_req->method,
		acl_vstring_str(hdr_req->url_part),
		hdr_req->hdr.version.major,
		hdr_req->hdr.version.minor);
#endif

	acl_foreach(iter, hdr_req->hdr.entry_lnk) {
		if (i++ == 0)
			continue;
		entry = (HTTP_HDR_ENTRY*) iter.data;
		if (entry->off)
			continue;
		acl_vstring_strcat(buf, entry->name);
		acl_vstring_strcat(buf, ": ");
		acl_vstring_strcat(buf, entry->value);
		acl_vstring_strcat(buf, "\r\n");
	}
	acl_vstring_strcat(buf, "\r\n");
}

/* 连接服务器成功，开始向服务器发送HTTP请求头 */
static void start_forward_request(HTTP_CLIENT *http_client)
{
	/* 分配动态内存 */
	if (http_client->buf == NULL) {
		http_client->buf = acl_vstring_alloc(HTTP_HDRLEN_DEF);
	} else {
		ACL_VSTRING_RESET(http_client->buf);
	}

	if (var_cfg_http_proxy_connection_off) {
		/* 主要是一些比较弱的类似GFW的东东似乎处理该字段有问题，会有延迟 */
		http_hdr_entry_off(&http_client->req_curr->hdr_req->hdr, "Proxy-Connection");
	}

	/* 重新创建 HTTP 请求头至 http_client->buf 中 */
	rebuild_request(http_client->req_curr->hdr_req, http_client->buf);

	/* 设置回调函数 */
	acl_aio_ctl(http_client->entry.server,
		ACL_AIO_CTL_WRITE_HOOK_ADD, send_request_hdr_complete, http_client,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, send_request_hdr_error, http_client,
		ACL_AIO_CTL_END);

	/* 锁定服务端 */
	http_client->flag |= HTTP_FLAG_SERVER_LOCKED;

	/* 向服务器转发客户端的HTTP请求头数据 */
	acl_aio_writen(http_client->entry.server,
		acl_vstring_str(http_client->buf),
		(int) ACL_VSTRING_LEN(http_client->buf));
}

/*----------------------------------------------------------------------------*/

/* 将客户端请求数据重新向后端其它服务器发送 */

static int http_request_reforward(HTTP_CLIENT *http_client)
{
	const char *myname = "http_request_reforward";
	ACL_ASTREAM *server = http_client->entry.server;

	/* 如果不是从连接池中取得的连接则将重试次数加 1 */
	if (!http_client->entry.flag_conn_reuse)
		http_client->entry.nretry_on_error++;

	if (http_client->hdr_res) {
		http_hdr_res_free(http_client->hdr_res);
		http_client->hdr_res = NULL;
	}

	/* 断开与服务端的连接，但保持与浏览器端的连接 */
	if (server) {
		/* 使服务流与该会话分离 */
		client_entry_detach(&http_client->entry, acl_aio_vstream(server));
		/* 取消 HTTP 响应头的回调函数 */
		acl_aio_clean_hooks(server);
		/* only for test */
		if (acl_aio_iswset(server)) {
			acl_msg_info("%s(%d): defer free(%d)\n", myname, __LINE__, ACL_VSTREAM_SOCK(server->stream));
                } else
			acl_msg_info("%s(%d): not defer free(%d)\n", myname, __LINE__, ACL_VSTREAM_SOCK(server->stream));
		/* 仅异步关闭服务端流 */
		acl_aio_iocp_close(server);
	}

	/* 如果重试次数超过该域名所对应的IP主机个数则返回错误，不再重试 */
	if (http_client->entry.nretry_on_error > http_client->entry.dns_ctx.ip_cnt) {
		acl_msg_error("%s(%d): has retried before(%d,%d), reuse connecion %s",
			myname, __LINE__, http_client->entry.nretry_on_error,
			http_client->entry.dns_ctx.ip_cnt,
			http_client->entry.flag_conn_reuse ? "yes" : "no");
		return (-1);
	}

	/* 如果重试次数超过阀值，则不再重试直接返回错误 */
	if (http_client->entry.nretry_on_error >= MAX_RETRIED) {
		acl_msg_error("%s(%d): has retried before(%d,%d)",
			myname, __LINE__, http_client->entry.nretry_on_error,
			MAX_RETRIED);
		return (-1);
	}

	/* 开始重试连接下一个IP */
	forward_start((CLIENT_ENTRY*) http_client);
	return (0);
}

/*----------------------------------------------------------------------------*/

static char HTTP_CONNECT_FIRST[] = "HTTP/1.0 200 Connection established\r\n\r\n";

/* 当服务端流关闭时的回调函数 */

static int on_close_server(ACL_ASTREAM *server acl_unused, void *ctx)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) ctx;

	/* 取消服务端流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_SERVER_LOCKED;
	http_client->flag |= HTTP_FLAG_SERVER_CLOSED | HTTP_FLAG_FINISH;
	http_proxy_complete(http_client, -1);
	/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
	return (-1);
}

static int http_proxy_connect_complete(CLIENT_ENTRY *entry)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) entry;
	const char *method = http_client->req_curr->hdr_req->method;

	/* 添加服务流关闭时的回调函数 */
	acl_aio_add_close_hook(http_client->entry.server,
		on_close_server, http_client);

	/* CONNECT 请求, 转向纯 TCP 代理模式, 从而方便支持 SSL */
	if (var_cfg_http_method_connect_enable && strcasecmp(method, "CONNECT") == 0) {
		if (entry->client == NULL) {
			acl_msg_warn("%s(%d): client null", __FILE__, __LINE__);
			http_proxy_complete(http_client, -1);
			return (0);
		}

		acl_aio_writen(entry->client, HTTP_CONNECT_FIRST,
			(int) strlen(HTTP_CONNECT_FIRST));
		if (entry->client && entry->server) {
			tcp_start(entry);
			return (0);
		} else {
			http_proxy_complete(http_client, -1);
			return (0);
		}
	}

	if (strcasecmp(method, "GET") != 0 && strcasecmp(method, "POST") != 0) {
		if (entry->client == NULL) {
			acl_msg_error("%s(%d): client null", __FILE__, __LINE__);
		} else
			acl_aio_writen(entry->client, HTTP_REQUEST_INVALID,
				(int) strlen(HTTP_REQUEST_INVALID));
		acl_msg_error("%s(%d): method(%s) invalid",
			__FILE__, __LINE__, method);
		http_proxy_complete(http_client, -1);
		return (0); /* 返回－1以使异步框架关闭该异步流 */
	}

	/* 处理 GET、POST 请求 */
	start_forward_request(http_client);
	return (0);
}

static void http_proxy_connect_timeout(CLIENT_ENTRY *entry)
{
	const char *myname = "http_proxy_connect_timeout";

	if (entry->client == NULL) {
		acl_msg_error("%s(%d): client null", myname, __LINE__);
		http_proxy_complete((HTTP_CLIENT*) entry, -1);
	} else {
		acl_msg_error("%s(%d): connect(%s, %s) timeout",
			myname, __LINE__, entry->domain_key,
			entry->domain_addr);
		acl_aio_refer(entry->client);
		acl_aio_writen(entry->client, HTTP_CONNECT_TIMEOUT,
			(int) strlen(HTTP_CONNECT_TIMEOUT));
		acl_aio_unrefer(entry->client);
		http_proxy_complete((HTTP_CLIENT*) entry, -1);
	}
}

static void http_proxy_connect_error(CLIENT_ENTRY *entry)
{
	const char *myname = "http_proxy_connect_error";

	if (entry->client == NULL) {
		acl_msg_error("%s(%d): client null", myname, __LINE__);
		http_proxy_complete((HTTP_CLIENT*) entry, -1);
	} else {
		acl_msg_error("%s(%d): connect(%s, %s) error",
			myname, __LINE__, entry->domain_key,
			entry->domain_addr);
		acl_aio_refer(entry->client);
		acl_aio_writen(entry->client, HTTP_CONNECT_ERROR,
			(int) strlen(HTTP_CONNECT_ERROR));
		acl_aio_unrefer(entry->client);
		http_proxy_complete((HTTP_CLIENT*) entry, -1);
	}
}

static void nslookup_complete_fn(CLIENT_ENTRY *entry, int status)
{
	if (status == NSLOOKUP_OK) {
		/* 设置连接成功后的回调函数 */
		entry->connect_notify_fn = http_proxy_connect_complete;
		entry->connect_timeout_fn = http_proxy_connect_timeout;
		entry->connect_error_fn = http_proxy_connect_error;
		forward_start(entry);
	} else {
		acl_aio_refer(entry->client);
		acl_aio_writen(entry->client, HTTP_REPLY_DNS_ERR,
			(int) strlen(HTTP_REPLY_DNS_ERR));
		acl_aio_unrefer(entry->client);
		http_proxy_complete((HTTP_CLIENT*) entry, -1);
	}
}

static void handle_one(HTTP_CLIENT *http_client, HTTP_CLIENT_REQ *req)
{
	int   ret;
	http_client->req_curr = req;  /* 设置当前可以处理的请求 */

	/* 先检查用户自定义过滤器 */
	if ((ret = http_client_req_filter(http_client))) {
		/* 如果返回非0值则表示请求过滤器接管了该请求 */
		return;
	}

	/* 设置DNS查询回调函数 */
	http_client->entry.nslookup_notify_fn = nslookup_complete_fn;
	http_client->entry.dns_errmsg = NULL;

	/* 从浏览器的请求头中获取服务端的端口号 */
	dns_lookup(&http_client->entry, req->hdr_req->host, req->hdr_req->port);
}

/**
 * 成功读到HTTP请求头后的回调函数
 */
static int request_header_ready(int status, void *arg)
{
	const char *myname = "request_header_ready";
	HTTP_CLIENT_REQ *req = (HTTP_CLIENT_REQ *) arg;
	HTTP_CLIENT *http_client = req->http_client;
	const char *via;
	static char *via_static = NULL;
	static int   via_max = 256;

	/* 取消客户端流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;

	if (status != HTTP_CHAT_OK) {
		http_proxy_complete(http_client, -1);
		return (0);
	}

	if (http_hdr_req_parse3(req->hdr_req, 0 , 0) < 0) {
		acl_msg_error("%s: parse hdr_req error", myname);
		http_proxy_complete(http_client, -1);
		return (0);
	}

#ifdef WIN32
#define snprintf _snprintf
#endif

	if (via_static == NULL) {
		via_static = (char*) acl_mycalloc(1, via_max);
		snprintf(via_static, via_max, "jaws-%d", getpid());
	}

	/* 检查是否产生回路现象 */

	via = http_hdr_entry_value(&req->hdr_req->hdr, "x-via-jaws");
	if (via == NULL) {
		http_hdr_put_str(&req->hdr_req->hdr, "x-via-jaws", via_static);
	} else if (strcasecmp(via, via_static) == 0) {
		acl_msg_warn("%s(%d): loop tested, via(%s), url(http://%s%s)",
			myname, __LINE__, via, req->hdr_req->host,
			acl_vstring_str(req->hdr_req->url_part));

		/* 锁定客户端流 */
		http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;

		acl_aio_writen(http_client->entry.client,
			HTTP_REQUEST_LOOP,
			(int) strlen(HTTP_REQUEST_LOOP));

		/* 取消客户端流的锁定状态 */
		http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;
		http_proxy_complete(http_client, -1);
		return (0);
	}

	/* 该请求已经完成，取消其等待状态 */
	req->flag &= ~CLIENT_READ_WAIT;

	if (http_client->req_curr != NULL) {
		/* 如果前一个请求还未处理完毕，则返回 */
		return (0);
	}

	/* 当前没有正在处理的请求过程，所以开始处理该请求 */

	/* 从队列中弹出该请求，以免被重复使用 */
	if (acl_fifo_pop(&http_client->req_list) != req)
		acl_msg_fatal("%s(%d): request invalid", myname, __LINE__);

	handle_one(http_client, req);
	return (0);
}

static int http_proxy_next(HTTP_CLIENT *http_client)
{
	HTTP_CLIENT_REQ *req = acl_fifo_head(&http_client->req_list);

	if (req) {
		if (!(req->flag & CLIENT_READ_WAIT))
			handle_one(http_client, req);
	} else {
		http_proxy_req_get(http_client);
	}

	return (0);
}

/* 发送响应体至客户端失败时的回调函数 */

static int on_close_client(ACL_ASTREAM *client acl_unused, void *ctx)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) ctx;

#if 0
	acl_msg_info("%s(%d): close client(%lx, fd=%d) now, server %s",
		__FUNCTION__, __LINE__, (long) client,
		ACL_VSTREAM_SOCK(client->stream),
		http_client->entry.server ? "not null" : "null");
#endif
	/* 取消客户端流的锁定状态 */
	http_client->flag &= ~HTTP_FLAG_CLIENT_LOCKED;
	/* 设置客户流的关闭状态 */
	http_client->flag |= HTTP_FLAG_CLIENT_CLOSED | HTTP_FLAG_FINISH;
	http_proxy_complete(http_client, -1);
	/* 必须返回 -1, 因为不希望继续调用其它的关闭回调函数 */
	return (-1);
}

static void http_proxy_req_get(HTTP_CLIENT *http_client)
{
	HTTP_CLIENT_REQ *req = http_client_req_new(http_client);

	req->flag |= CLIENT_READ_WAIT;
	req->hdr_req = http_hdr_req_new();
	acl_fifo_push(&http_client->req_list, req);

	/* 设置从客户流读数据失败或出现其它错误时的回调函数 */
	acl_aio_add_close_hook(http_client->entry.client,
		on_close_client, http_client);

	/* 锁定客户端流 */
	http_client->flag |= HTTP_FLAG_CLIENT_LOCKED;

	http_client->total_size = 0;
	http_client->flag &= ~HTTP_FLAG_CLIENT_KEEP_ALIVE;
	http_client->flag &= ~HTTP_FLAG_SERVER_KEEP_ALIVE;

	/* 开始读取HTTP请求头 */
	http_hdr_req_get_async(req->hdr_req,
		http_client->entry.client,
		request_header_ready,
		req,
		http_client->entry.service->rw_timeout);
}

int http_proxy_start(HTTP_CLIENT *http_client)
{
acl_msg_info("%s(%d)", __FUNCTION__, __LINE__); /* only for test */
	http_proxy_req_get(http_client);
	return (0);
}
