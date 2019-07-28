#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/lib_http.h"
#include "http.h"

typedef struct HTTP_CHAT_CTX {
	HTTP_HDR  *hdr;
	ACL_VSTREAM *stream;
	unsigned int flag;                  /**<　继承的标志位, defined as HTTP_CHAT_FLAG_XXX */
	int   timeout;
	int   chunked;
	http_off_t   chunk_len;             /**< 当前数据块所需要读的数据长度(字节) */
	http_off_t   read_cnt;              /**< 当前数据块所读数据长度(字节) */
	http_off_t   body_len;              /**< 所读到数据体总长度(字节) */
	union {
		HTTP_HDR_NOTIFY  hdr_notify;
		HTTP_BODY_NOTIFY body_notify;
	} notify;
	void *arg;
	struct {
		int   chunk_oper;
#define CHUNK_OPER_HEAD	1
#define CHUNK_OPER_BODY	2
#define CHUNK_OPER_LINE	3
#define CHUNK_OPER_TAIL	4
	} chunk;
} HTTP_CHAT_CTX;

/*----------------------------------------------------------------------------*/
static HTTP_CHAT_CTX *new_ctx(void)
{
	const char *myname = "new_ctx";
	HTTP_CHAT_CTX *ctx;

	ctx = (HTTP_CHAT_CTX*) acl_mycalloc(1, sizeof(HTTP_CHAT_CTX));
	if (ctx == NULL) {
		char  ebuf[256];

		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	}

	return ctx;
}

static void free_ctx(void *ctx)
{
	acl_myfree(ctx);
}

/*----------------------------------------------------------------------------*/
/* 分析一行数据, 是否是一个完整的HTTP协议头 */

static int hdr_ready(HTTP_HDR *hdr, const char *line, int dlen)
{
	HTTP_HDR_ENTRY *entry;

	hdr->cur_lines++;
	if (hdr->max_lines > 0 && hdr->cur_lines > hdr->max_lines)
		return HTTP_CHAT_ERR_TOO_MANY_LINES;

	if (dlen > 0)
		hdr->valid_lines++;

	if (dlen == 0) {
		if (hdr->valid_lines > 0)
			return HTTP_CHAT_OK;
		else
			return HTTP_CHAT_CONTINUE;
	}

	entry = http_hdr_entry_new(line);
	if (entry == NULL)  /* ignore invalid entry line */
		return HTTP_CHAT_CONTINUE;

	http_hdr_append_entry(hdr, entry);
	return HTTP_CHAT_CONTINUE;
}

/* 同步读取一个完整的HTTP协议头 */

static int hdr_get(HTTP_HDR *hdr, ACL_VSTREAM *stream, int timeout)
{
	char  buf[8192];
	int   ret;

	stream->rw_timeout = timeout;

	while (1) {
		ret = acl_vstream_gets_nonl(stream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF)
			return HTTP_CHAT_ERR_IO;

		ret = hdr_ready(hdr, buf, ret);
		if (ret != HTTP_CHAT_CONTINUE)
			break;
	}

	/*  ret: HTTP_CHAT_OK or error */
	return ret;
}

int http_hdr_req_get_sync(HTTP_HDR_REQ *hdr_req, ACL_VSTREAM *stream, int timeout)
{
	return hdr_get(&hdr_req->hdr, stream, timeout) == HTTP_CHAT_OK ? 0 : -1;
}

int http_hdr_res_get_sync(HTTP_HDR_RES *hdr_res, ACL_VSTREAM *stream, int timeout)
{
	return hdr_get(&hdr_res->hdr, stream, timeout) == HTTP_CHAT_OK ? 0 : -1;
}
/*------------------------ read http body data -------------------------------*/

static http_off_t chunked_data_get(HTTP_CHAT_CTX *ctx, void *buf, int size)
{
	if (ctx->chunk_len == 0)
		return 0;
	else if (ctx->chunk_len > 0) {
		char *ptr = buf;
		http_off_t ntotal = 0, ret, n;

		n = ctx->chunk_len - ctx->read_cnt;
		n = n > size ? size : n;
		while (n > 0) {
			ret = acl_vstream_read(ctx->stream, ptr, (size_t) n);
			if (ret == ACL_VSTREAM_EOF) {
				if (ntotal == 0)
					ntotal = -1;
				break;
			}
			ntotal += ret;
			n -= ret;
			ptr += ret;
			ctx->body_len += ret;
			ctx->read_cnt += ret;
			if ((ctx->flag & HTTP_CHAT_FLAG_BUFFED) == 0)
				break;
		}
		return ntotal;
	} else {
		http_off_t   ret;

		ret = acl_vstream_read(ctx->stream, buf, (size_t) size);
		if (ret == ACL_VSTREAM_EOF)
			return -1;
		ctx->body_len += ret;
		ctx->read_cnt += ret;
		return ret;
	}
}

static int chunked_hdr_get(HTTP_CHAT_CTX *ctx)
{
	const char *myname = "chunked_hdr_get";
#if defined(ACL_WINDOWS) && _MSC_VER >= 1500
	char  ext[64];
#else
	char *ext = NULL;
#endif
	char  buf[HTTP_BSIZE];
	int   ret, n, chunk_len;

	n = acl_vstream_gets(ctx->stream, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF)
		return -1;

	ctx->read_cnt = 0;  /* reset the len to be read */
	ctx->body_len += n;

#ifdef ACL_WINDOWS
# if _MSC_VER >= 1500
	ret = sscanf_s(buf, "%X %s", (unsigned int *) &chunk_len, ext, (int) sizeof(ext));
# else
	ret = sscanf(buf, "%X %s", (unsigned int *) &chunk_len, ext);
# endif
#else 
	ret = sscanf(buf, "%X %s", (unsigned int *) &chunk_len, ext);
#endif

	if (ret < 0 || chunk_len < 0) {
		acl_msg_error("%s(%d): chunked hdr(%s) invalid, dlen(%d), "
			"'\\n': %d, %d", myname, __LINE__, buf, n, buf[0], '\n');
		return -1;
	}

	ctx->chunk_len = chunk_len;
	return 0;
}

static int chunked_sep_gets(HTTP_CHAT_CTX *ctx)
{
	const char *myname = "chunked_sep_gets2";
	char  buf[HTTP_BSIZE];
	int   n;

	n = acl_vstream_gets(ctx->stream, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): gets sep line error", myname, __LINE__);
		return -1;
	}

	ctx->body_len += n;
	return 0;
}

static int chunked_trailer_get(HTTP_CHAT_CTX *ctx)
{
	const char *myname = "chunked_tailer_get2";
	char  buf[HTTP_BSIZE];
	int   n;

	while (1) {
		n = acl_vstream_gets(ctx->stream, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): get line error", myname, __LINE__);
			return -1;
		}
		ctx->body_len += n;
		if (strcmp(buf, "\r\n") == 0 || strcmp(buf, "\n") == 0)
			break;
	}

	return 0;
}

static http_off_t body_get(HTTP_CHAT_CTX *ctx, void *buf, int size)
{
	const char *myname = "body_get";
	http_off_t   ret;

	/* Transfer-Encoding: chunked 的优先级要高于 Conteng-Length */

	if (!ctx->chunked) {
		if (ctx->chunk_len > 0 && ctx->read_cnt >= ctx->chunk_len)
			return (0);
		return chunked_data_get(ctx, buf, size);
	}

	while (1) {
		if (ctx->chunk.chunk_oper == CHUNK_OPER_HEAD) {
			ret = chunked_hdr_get(ctx);
			if (ret < 0)
				return -1;
			if (ctx->chunk_len == 0)
				ctx->chunk.chunk_oper = CHUNK_OPER_TAIL;
			else
				ctx->chunk.chunk_oper = CHUNK_OPER_BODY;
		} else if (ctx->chunk.chunk_oper == CHUNK_OPER_BODY) {
			ret = chunked_data_get(ctx, buf, size);
			if (ret < 0)
				return -1;
			if (ctx->read_cnt >= ctx->chunk_len) {
				if (chunked_sep_gets(ctx) < 0)
					return -1;
				ctx->chunk.chunk_oper = CHUNK_OPER_HEAD;
			}
			return (ret);
		} else if (ctx->chunk.chunk_oper == CHUNK_OPER_TAIL) {
			ret = chunked_trailer_get(ctx);
			return ret;
		} else {
			acl_msg_error("%s(%d): unknown oper status(%d)",
				myname, __LINE__, ctx->chunk.chunk_oper);
			return -1;
		}
	}
}

http_off_t http_req_body_get_sync(HTTP_REQ *request, ACL_VSTREAM *stream,
	void *buf, int size)
{
	HTTP_CHAT_CTX *ctx;

	if (request->hdr_req->hdr.content_length == 0) {
		/* 扩展了HTTP请求协议部分, 允许请求数据为块传输 */
		if (request->hdr_req->hdr.chunked == 0)
			return 0;
	}

	if (request->ctx == NULL) {
		ctx            = new_ctx();
		ctx->hdr       = &request->hdr_req->hdr;
		ctx->stream    = stream;

		/* 扩展了HTTP请求协议部分, 允许请求数据为块传输 */
		ctx->chunked   = request->hdr_req->hdr.chunked;
		ctx->chunk_len = request->hdr_req->hdr.content_length;
		ctx->body_len  = 0;
		ctx->read_cnt  = 0;
		if (ctx->chunked)
			ctx->chunk.chunk_oper = CHUNK_OPER_HEAD;
		request->ctx = (void*) ctx;
		request->free_ctx = free_ctx;
	} else
		ctx = (HTTP_CHAT_CTX*) request->ctx;

	ctx->flag = request->flag;
	return body_get(ctx, buf, size);
}

http_off_t http_res_body_get_sync(HTTP_RES *respond, ACL_VSTREAM *stream,
	void *buf, int size)
{
	HTTP_CHAT_CTX *ctx;

	if (respond->hdr_res->hdr.content_length == 0) {
		/* 块传输协议优先于 content-length */
		if (respond->hdr_res->hdr.chunked == 0)
			return 0;
	}

	if (respond->ctx == NULL) {
		ctx            = new_ctx();
		ctx->hdr       = &respond->hdr_res->hdr;
		ctx->stream    = stream;
		ctx->chunked   = respond->hdr_res->hdr.chunked;
		ctx->chunk_len = respond->hdr_res->hdr.content_length;
		ctx->body_len  = 0;
		ctx->read_cnt  = 0;
		if (ctx->chunked)
			ctx->chunk.chunk_oper = CHUNK_OPER_HEAD;
		respond->ctx = (void*) ctx;
		respond->free_ctx = free_ctx;
	} else
		ctx = (HTTP_CHAT_CTX*) respond->ctx;

	ctx->flag = respond->flag;
	return body_get(ctx, buf, size);
}

void http_chat_sync_reqctl(HTTP_REQ *request, int name, ...)
{
	const char *myname = "http_chat_sync_reqctl";
	va_list ap;
	int   n;

	va_start(ap, name);
	for (; name != HTTP_CHAT_SYNC_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case HTTP_CHAT_CTL_BUFF_ONOFF:
			n = va_arg(ap, int);
			if (n)
				request->flag |= HTTP_CHAT_FLAG_BUFFED;
			else
				request->flag &= ~HTTP_CHAT_FLAG_BUFFED;
			break;
		default:
			acl_msg_panic("%s, %s(%d): bad name %d",
				myname, __FILE__, __LINE__, name);
			break;
		}
	}
	va_end(ap);
}

void http_chat_sync_resctl(HTTP_RES *respond, int name, ...)
{
	const char *myname = "http_chat_sync_resctl";
	va_list ap;
	int   n;

	va_start(ap, name);
	for (; name != HTTP_CHAT_SYNC_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case HTTP_CHAT_CTL_BUFF_ONOFF:
			n = va_arg(ap, int);
			if (n)
				respond->flag |= HTTP_CHAT_FLAG_BUFFED;
			else
				respond->flag &= ~HTTP_CHAT_FLAG_BUFFED;
			break;
		default:
			acl_msg_panic("%s, %s(%d): bad name %d",
				myname, __FILE__, __LINE__, name);
			break;
		}
	}
	va_end(ap);
}
/*----------------------------------------------------------------------------*/
