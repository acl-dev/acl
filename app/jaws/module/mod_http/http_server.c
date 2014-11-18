#include "lib_acl.h"
#include "lib_protocol.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef ACL_UNIX
#include <unistd.h>
#endif
#include "service.h"
#include "http_service.h"

#ifdef	ACL_UNIX
#include <sys/uio.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

static int http_server_keep_alive_next(HTTP_CLIENT *client)
{
	http_client_reset(client);
	http_service_start(client);

	return (0);
}

static int http_reply_error(HTTP_CLIENT *client, int status)
{
	const ACL_VSTRING *tmpl_error = http_tmpl_get(status);

	client->hdr_res = http_hdr_res_error(status);
	http_hdr_build(&client->hdr_res->hdr, client->buf);
	acl_vstring_strcat(client->buf, STR(tmpl_error));
	WRITE_TO_CLIENT(client, STR(client->buf), LEN(client->buf));

	return (-1);
}

static int http_reply_file(HTTP_CLIENT *client)
{
	char  buf[8192];
	int   n;

	n = acl_vstream_read(client->fp, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF) {
		/* only for test */
#if 0
		acl_msg_info("200 http://%s/%s", client->hdr_req->host,
			acl_vstring_str(client->hdr_req->url_path));
#endif
		if (client->sent_size != client->total_size) {
			if (client->cache)
				file_cache_free(client->cache);
			client->cache = NULL;
		}
		if (client->hdr_res->hdr.keep_alive)
			return (http_server_keep_alive_next(client));
		return (-1);
	}

	if (client->cache)
		file_cache_push(client->cache, buf, n);
	client->sent_size += n;
	
	WRITE_TO_CLIENT(client, buf, n);
	return (0);
}

static int send_file_ready(ACL_ASTREAM *stream acl_unused, void *ctx)
{
	HTTP_CLIENT *client = (HTTP_CLIENT*) ctx;

	return (http_reply_file(client));
}

static int send_file_hdr_ready(ACL_ASTREAM *stream, void *ctx)
{
	HTTP_CLIENT *client = (HTTP_CLIENT*) ctx;

	acl_aio_ctl(stream,
		ACL_AIO_CTL_WRITE_HOOK_ADD, send_file_ready, ctx,
		ACL_AIO_CTL_END);
	return (http_reply_file(client));
}

static int http_doc_file(HTTP_CLIENT *client, const char *filepath)
{
	const char *myname = "http_doc_file";
	struct acl_stat sbuf;

	if(client->buf == NULL)
		client->buf = acl_vstring_alloc(HTTP_HDRLEN_DEF);

	assert(client->fp == NULL);
	client->fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 4096);
	if (client->fp == NULL) {
		char  ebuf[256];

		acl_msg_error("%s(%d): file(%s) no exist, error(%s)",
			myname, __LINE__, filepath,
			acl_last_strerror(ebuf, sizeof(ebuf)));

		if (errno == EACCES)
			return (http_reply_error(client, 403));
		else if (errno == ENOENT || errno == EISDIR)
			return (http_reply_error(client, 404));
		else
			return (http_reply_error(client, 500));
	}

	if (acl_stat(filepath, &sbuf) < 0) {
		acl_msg_error("%s(%d): fstat file(%s) error(%s)",
			myname, __LINE__, filepath, strerror(errno));
		return (http_reply_error(client, 404));
	}

	client->total_size = (int) sbuf.st_size;
	client->sent_size = 0;
	if (client->use_cache)
		client->cache = file_cache_new(filepath, sbuf.st_mtime);
	client->hdr_res = http_hdr_res_static(200);
	http_hdr_set_keepalive(client->req_curr->hdr_req, client->hdr_res);
	http_hdr_put_int(&client->hdr_res->hdr, "Content-Length", (int) sbuf.st_size);
	http_hdr_put_int(&client->hdr_res->hdr, "Last-Modified", (int) sbuf.st_mtime);

	http_hdr_build(&client->hdr_res->hdr, client->buf);

	/* xxx: only for test: ACL_AIO_CTL_CLOSE_FN */
	acl_aio_ctl(client->entry.client,
		ACL_AIO_CTL_WRITE_HOOK_ADD, send_file_hdr_ready, client,
		ACL_AIO_CTL_CTX, client,
		ACL_AIO_CTL_END);

	/* write http respond header */
	WRITE_TO_CLIENT(client, STR(client->buf), LEN(client->buf));
	return (0);
}

static int http_doc_cache_send(HTTP_CLIENT *client, const char *hdr, size_t len)
{
#ifndef	UIO_MAX
#define	UIO_MAX	512
#endif

	struct iovec chunks[UIO_MAX];
	BUFFER *buffer;
	int   i;

	if (hdr && len > 0) {
		chunks[0].iov_base = STR(client->buf);
		chunks[0].iov_len = LEN(client->buf);
		i = 1;
	} else {
		i = 0;
	}

	for (; i < UIO_MAX - 1; i++) {
		buffer = file_cache_next_buffer(&client->cache_iter);
		if (buffer == NULL)
			break;
		chunks[i].iov_base = buffer->buf;
		chunks[i].iov_len = buffer->size;
	}
	chunks[i].iov_base = NULL;
	chunks[i].iov_len = 0;

	if (i == 0) {
		/* only for test */
#if 0
		acl_msg_info("200 http://%s/%s", client->hdr_req->host,
			acl_vstring_str(client->hdr_req->url_path));
#endif
		if (client->hdr_res->hdr.keep_alive)
			return (http_server_keep_alive_next(client));
		return (-1);
	}

	WRITEV_TO_CLIENT(client, chunks, i);
	return (0);
}

static int send_cache_ready(ACL_ASTREAM *stream acl_unused, void *ctx)
{
	HTTP_CLIENT *client = (HTTP_CLIENT*) ctx;

	return (http_doc_cache_send(client, NULL, 0));
}

static int http_doc_cache(HTTP_CLIENT *client, FILE_CACHE *cache)
{
	client->cache = cache;
	file_cache_iter(cache, &client->cache_iter);

	client->hdr_res = http_hdr_res_static(200);
	http_hdr_set_keepalive(client->req_curr->hdr_req, client->hdr_res);
	http_hdr_put_int(&client->hdr_res->hdr, "Content-Length", (int) cache->size);
	http_hdr_put_str(&client->hdr_res->hdr, "Last-Modified", cache->tm_mtime);

	if(client->buf == NULL)
		client->buf = acl_vstring_alloc(HTTP_HDRLEN_DEF);

	http_hdr_build(&client->hdr_res->hdr, client->buf);

	/* only for test */
#if 0
        acl_aio_disable_read(client->entry.client);
#endif

	acl_aio_ctl(client->entry.client,
		ACL_AIO_CTL_WRITE_HOOK_ADD, send_cache_ready, client,
		ACL_AIO_CTL_CTX, client,
		ACL_AIO_CTL_END);

	return (http_doc_cache_send(client, STR(client->buf), LEN(client->buf)));
}

/**     
 * 成功读到HTTP请求头后的回调函数
 */                     
static int request_header_ready(int status, void *arg)
{
	const char *myname = "request_header_ready";
	HTTP_CLIENT_REQ *req = (HTTP_CLIENT_REQ*) arg;
	HTTP_CLIENT *http_client = req->http_client;
	const HTTP_VHOST *vhost;
	const HTTP_VPATH *vpath;
	FILE_CACHE *cache;
	ACL_VSTRING *file_path = ((HTTP_SERVICE*)
			http_client->entry.service)->file_path;
	char *ptr;

	/* 先禁止读操作，因为目前还不支持 pipeline 模式 */
	acl_aio_disable_read(http_client->entry.client);

	if (status != HTTP_CHAT_OK) {
		acl_debug(20, 2) ("%s: status(%d)", myname, status);
		return (-1);
	}

	if (http_hdr_req_parse(req->hdr_req) < 0) {
		acl_msg_error("%s: parse hdr_req error", myname);
		return (-1);
	}

	vhost = http_vhost_find(req->hdr_req->host);
	if (vhost == NULL) {
		acl_msg_error("%s(%d): host(%s) not found", myname, __LINE__,
			req->hdr_req->host);
		return (-1);
	}
	vpath = http_vpath_find(vhost, acl_vstring_str(req->hdr_req->url_path));
	if (vpath == NULL) {
		acl_msg_error("%s(%d): vpath(%s) not found", myname, __LINE__,
			acl_vstring_str(req->hdr_req->url_path));
		return (-1);
	}
	if (vpath->type != HTTP_TYPE_DOC) {
		acl_msg_error("%s(%d): type(%d) not supported yet",
			myname, __LINE__, vpath->type);
		return (-1);
	}

	if (ACL_VSTRING_LEN(req->hdr_req->url_path) <= 0) {
		acl_msg_error("%s: url_path(%s)'s len(%d)",
			myname, acl_vstring_str(req->hdr_req->url_path),
			ACL_VSTRING_LEN(req->hdr_req->url_path));
		return (-1);
	}

	if (http_client->req_curr != NULL) {
		/* 如果前一个请求还未处理完毕，则返回 */
		return (0);
	}
	http_client->req_curr = req;  /* 设置当前可以处理的请求 */

	/* 先检查用户自定义过滤器 */
	if (http_client_req_filter(http_client)) {
		/* 返回 -1 仅是为了让异步框架自动关闭该异步流对象，
		 * 因为该异步流已经与数据流分离，所以当关闭异步流
		 * 时，并不真正关闭与浏览器之间的数据流，也不关闭
		 * 与服务器之间的数据流
		 */
		return (-1);
	}

	ptr = STR(req->hdr_req->url_path);
	if (*ptr == '/')
		ptr++;
	acl_vstring_strcpy(file_path, STR(vpath->path));
	acl_vstring_strcat(file_path, ptr);

	if (*(acl_vstring_end(req->hdr_req->url_path) - 1) == '/')
		acl_vstring_strcat(file_path, vhost->default_page);

	if (http_client->use_cache) {
		cache = file_cache_find(STR(file_path));
		if (cache)
			return (http_doc_cache(http_client, cache));
	}
	return (http_doc_file(http_client, acl_vstring_str(file_path)));
}

int http_server_start(HTTP_CLIENT *http_client)
{
	HTTP_CLIENT_REQ *req = http_client_req_new(http_client);

	req->hdr_req = http_hdr_req_new();
	acl_fifo_push(&http_client->req_list, req);

	http_hdr_req_get_async(req->hdr_req,
			http_client->entry.client,
			request_header_ready,
			req,
			http_client->entry.service->rw_timeout);
	return (0);
}
