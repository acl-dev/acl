#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/lib_http.h"
#include "http.h"

/* 标识是请求还是响应 */
typedef enum { CTX_TYPE_REQ, CTX_TYPE_RES } ctx_type;

typedef struct HTTP_CHAT_CTX {
	HTTP_HDR  *hdr;                     /**< 通用HTTP头(请求头或响应头) */
	ACL_ASTREAM *stream;                /**< 流指针 */
	int   timeout;                      /**< 读写超时时间，秒 */
	int   chunked;                      /**< 是否是块传输模式 */
	ctx_type type;                      /**< 仅为了调试方便 */
	int   status;                       /**< 当前所处IO状态 */
#define	CHAT_S_HDR		(1 << 0)    /**< 读 HTTP 头 */
#define	CHAT_S_CHUNK_HDR        (1 << 1)    /**< 读 chunk 头 */
#define	CHAT_S_CHUNK_DAT        (1 << 2)    /**< 读 chunk 体 */
#define	CHAT_S_CHUNK_SEP        (1 << 3)    /**< 读 chunk 体分隔行 */
#define	CHAT_S_CHUNK_TAL        (1 << 4)    /**< 读结束尾 */

	http_off_t   chunk_len;             /**< 当前数据块所需要读的数据长度(字节) */
	http_off_t   read_cnt;              /**< 当前数据块所读数据长度(字节) */
	http_off_t   body_len;              /**< 所读到数据体总长度(字节) */
	union {
		HTTP_HDR_NOTIFY  hdr_notify;    /**< 数据头回调函数 */
		HTTP_BODY_NOTIFY body_notify;   /**< 数据体回调函数 */
	} notify;                           /**< 回调函数 */
	void *arg;                          /**< 回调参数 */
} HTTP_CHAT_CTX;

#define	HTTP_LEN_ROUND(_ctx) \
    (_ctx->chunk_len > 0 ? \
        (_ctx->chunk_len > _ctx->read_cnt + var_http_buf_size ? \
            var_http_buf_size : _ctx->chunk_len - _ctx->read_cnt) \
        : var_http_buf_size)

#if 1
#define	DISABLE_READ(x) do {  \
	acl_aio_disable_read((x)); \
} while (0)
#else
#define	DISABLE_READ	(void)
#endif
/*----------------------------------------------------------------------------*/
static HTTP_CHAT_CTX *new_ctx(ctx_type type)
{
	const char *myname = "__new_ctx";
	HTTP_CHAT_CTX *ctx;

	ctx = (HTTP_CHAT_CTX*) acl_mycalloc(1, sizeof(HTTP_CHAT_CTX));
	if (ctx == NULL) {
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	}

	ctx->type = type;
	return (ctx);
}

static void free_ctx_fn(void *ctx)
{
	acl_myfree(ctx);
}

/*----------------------------------------------------------------------------*/

/* 分析一行数据, 是否是一个完整的HTTP协议头 */

static int hdr_ready(HTTP_HDR *hdr, char *line, int dlen)
{
	const char *myname = "hdr_ready";
	HTTP_HDR_ENTRY *entry;

	hdr->cur_lines++;
	if (hdr->max_lines > 0 && hdr->cur_lines > hdr->max_lines) {
		acl_msg_error("%s(%d): cur_lines(%d) > max_lines(%d)",
			myname, __LINE__, hdr->cur_lines, hdr->max_lines);
		return HTTP_CHAT_ERR_TOO_MANY_LINES;
	}

	if (dlen > 0) {
		hdr->valid_lines++;
	}

	if (dlen == 0) {
		if (hdr->valid_lines > 0) {
			return HTTP_CHAT_OK;
		} else {
			return HTTP_CHAT_CONTINUE;
		}
	}

	if (hdr->valid_lines == 1) {
		entry = http_hdr_entry_head(line);
	} else {
		entry = http_hdr_entry_new2(line);
	}
	if (entry == NULL) { /* ignore invalid entry line */
		return HTTP_CHAT_CONTINUE;
	}

	http_hdr_append_entry(hdr, entry);
	return HTTP_CHAT_CONTINUE;
}

/* 异步读取一行数据的回调函数  */

#if 0
static int hdr_gets_ready(ACL_ASTREAM *astream, void *context,
	char *data, int dlen)
{
	HTTP_CHAT_CTX *ctx = (HTTP_CHAT_CTX *) context;
	HTTP_HDR *hdr = ctx->hdr;
	HTTP_HDR_NOTIFY notify = ctx->notify.hdr_notify;
	void *arg = ctx->arg;
	ACL_VSTRING *sbuf;
	int   ret;

	while (1) {
		ret = hdr_ready(hdr, data, dlen);
		switch (ret) {
		case HTTP_CHAT_CONTINUE:
			break;
		case HTTP_CHAT_OK:
			acl_aio_del_read_hook(astream, hdr_gets_ready, context);
			if (notify(ret, arg) < 0) {
				return (-1);
			}
			return (0);
		default:
			acl_aio_del_read_hook(astream, hdr_gets_ready, context);
			(void) notify(ret, arg);
			return (-1);
		}
		sbuf = acl_aio_gets_nonl_peek(astream);
		if (sbuf == NULL)
			break;
		data = acl_vstring_str(sbuf);
		dlen = (int) ACL_VSTRING_LEN(sbuf);
		ACL_VSTRING_RESET(sbuf);
	}

	acl_aio_gets_nonl(astream);
	return (0);
}
#endif

static int hdr_can_read(ACL_ASTREAM *astream, void *context)
{
	HTTP_CHAT_CTX *ctx = (HTTP_CHAT_CTX *) context;
	HTTP_HDR *hdr = ctx->hdr;
	HTTP_HDR_NOTIFY notify = ctx->notify.hdr_notify;
	void *arg = ctx->arg;
	ACL_VSTRING *sbuf;
	char *data;
	int   dlen, ret;

	while (1) {
		if ((ret = acl_aio_can_read(astream)) == ACL_VSTREAM_EOF) {
			notify(HTTP_CHAT_ERR_IO, arg);
			return -1;
		} else if (ret == 0) {
			break;
		}
		sbuf = acl_aio_gets_nonl_peek(astream);
		if (sbuf == NULL) {
			break;
		}
		data = acl_vstring_str(sbuf);
		dlen = (int) ACL_VSTRING_LEN(sbuf);
		ACL_VSTRING_RESET(sbuf);

		ret = hdr_ready(hdr, data, dlen);
		switch (ret) {
		case HTTP_CHAT_CONTINUE:
			break;
		case HTTP_CHAT_OK:
			DISABLE_READ(astream);
			if (notify(ret, arg) < 0) {
				return -1;
			}
			return 0;
		default:
			DISABLE_READ(astream);
			if (notify(ret, arg) < 0) {
				return -1;
			}
			return 0;
		}
	}

	acl_aio_enable_read(astream, hdr_can_read, ctx);
	return 0;
}

/* 异步读取一个完整的HTTP协议头 */

static void hdr_get_async(ctx_type type, HTTP_HDR *hdr, ACL_ASTREAM *astream,
	HTTP_HDR_NOTIFY notify, void *arg, int timeout)
{
	const char *myname = "hdr_get_async";
	HTTP_CHAT_CTX *ctx;
	ACL_VSTREAM *stream;

	if (hdr == NULL || astream == NULL || notify == NULL) {
		acl_msg_fatal("%s: input invalid", myname);
	}

	stream = acl_aio_vstream(astream);
	stream->rw_timeout = 0; /* bug fix, --zsx, 2007.7.7 */

	if (hdr->chat_ctx == NULL) {
		hdr->chat_ctx = new_ctx(type);
		hdr->chat_free_ctx_fn = free_ctx_fn;
	}
	ctx                    = (HTTP_CHAT_CTX*) hdr->chat_ctx;
	ctx->hdr               = hdr;
	ctx->timeout           = timeout;
	ctx->notify.hdr_notify = notify;
	ctx->arg               = arg;
	ctx->status            = CHAT_S_HDR;

#if 0
    acl_aio_ctl(astream,
        ACL_AIO_CTL_READ_HOOK_ADD, hdr_gets_ready, ctx,
        ACL_AIO_CTL_TIMEOUT, timeout,
        ACL_AIO_CTL_END);
    acl_aio_gets_nonl(astream);
#else
    acl_aio_ctl(astream, ACL_AIO_CTL_TIMEOUT, timeout,
        ACL_AIO_CTL_END);
    acl_aio_enable_read(astream, hdr_can_read, ctx);
#endif
}

void http_hdr_req_get_async(HTTP_HDR_REQ *hdr_req, ACL_ASTREAM *astream,
	HTTP_HDR_NOTIFY notify, void *arg, int timeout)
{
	hdr_get_async(CTX_TYPE_REQ, &hdr_req->hdr, astream, notify, arg, timeout);
}

void http_hdr_res_get_async(HTTP_HDR_RES *hdr_res, ACL_ASTREAM *astream,
		HTTP_HDR_NOTIFY notify, void *arg, int timeout)
{
	hdr_get_async(CTX_TYPE_RES, &hdr_res->hdr, astream, notify, arg, timeout);
}

/*------------------------ read http body data -------------------------------*/

/* 当读到块数据体中的最后一个分隔行时的回调函数 */

static int chunked_data_endl(ACL_ASTREAM *astream, HTTP_CHAT_CTX *ctx)
{
	HTTP_BODY_NOTIFY notify = ctx->notify.body_notify;
	void *arg = ctx->arg;
	ACL_VSTRING *sbuf;
	char *data;
	int   dlen;

	sbuf = acl_aio_gets_peek(astream);
	if (sbuf == NULL) {
		return 0;
	}

	data = acl_vstring_str(sbuf);
	dlen = (int) ACL_VSTRING_LEN(sbuf);
	ACL_VSTRING_RESET(sbuf);

	ctx->body_len += dlen;

	if (notify(HTTP_CHAT_CHUNK_DATA_ENDL, data, dlen, arg) < 0) {
		return -1;
	}
	ctx->status = CHAT_S_CHUNK_HDR;
	return 0;
}

static int chunked_data(ACL_ASTREAM *astream, HTTP_CHAT_CTX *ctx)
{
	HTTP_BODY_NOTIFY notify = ctx->notify.body_notify;
	void *arg = ctx->arg;
	ACL_VSTRING *sbuf = NULL;
	char *data;
	int   dlen, ret;

	if (ctx->chunked) {
		ret = (int) HTTP_LEN_ROUND(ctx);
		sbuf = acl_aio_readn_peek(astream, &ret);
	} else if (ctx->hdr->content_length <= 0) {
		sbuf = acl_aio_read_peek(astream, &ret);
	} else {
		ret = (int) HTTP_LEN_ROUND(ctx);
		if (ret <= 0) {
			/* 说明本次 HTTP 数据已经读完且遇到对方关闭
			 * 或对方发来了多余的数据，所以需要返回 -1
			 */
			DISABLE_READ(astream);
			if (notify(HTTP_CHAT_OK, NULL, 0, arg) < 0) {
				return -1;
			}
			return -1;
		} else {
			sbuf = acl_aio_readn_peek(astream, &ret);
		}
	}

	ctx->body_len += ret;
	ctx->read_cnt += ret;

	if (sbuf == NULL) {
		return 0;
	}

	data = acl_vstring_str(sbuf);
	dlen = (int) ACL_VSTRING_LEN(sbuf);
	ACL_VSTRING_RESET(sbuf);

	if (ctx->chunk_len > 0 && ctx->read_cnt >= ctx->chunk_len) {
		if (!ctx->chunked) {
			/* 如果读到完了整块数据且非 CHUNK 传输，
			 * 则认为读完 HTTP 响应
			 */
			/* xxx: 禁止连续读 */
			DISABLE_READ(astream);
			if (notify(HTTP_CHAT_OK, data, dlen, arg) < 0) {
				return -1;
			}
			return 1;
		}

		/* 对于 chunk 传输，读完本数据块 */
		if (notify(HTTP_CHAT_DATA, data, dlen, arg) < 0) {
			return -1;
		}

		/**
		 * printf(">>%s: chunk_len=%d, read=%d\r\n", __FUNCTION__,
		 *	(int) ctx->chunk_len, (int) ctx->read_cnt);
		 */
		/* 设置标志位开始读取块数据体的分隔行数据 */
		ctx->status = CHAT_S_CHUNK_SEP;
		return 0;
	}

	if (notify(HTTP_CHAT_DATA, data, dlen, arg) < 0) {
		return -1;
	}
	return 0;
}

static int chunked_trailer(ACL_ASTREAM *astream, HTTP_CHAT_CTX *ctx)
{
	HTTP_BODY_NOTIFY notify = ctx->notify.body_notify;
	void *arg = ctx->arg;
	ACL_VSTRING *sbuf;
	char *data;
	int   dlen;

	sbuf = acl_aio_gets_peek(astream);
	if (sbuf == NULL) {
		return 0;
	}

	data = acl_vstring_str(sbuf);
	dlen = (int) ACL_VSTRING_LEN(sbuf);
	ACL_VSTRING_RESET(sbuf);

	ctx->body_len += dlen;

	if (strcmp(data, "\r\n") == 0 || strcmp(data, "\n") == 0) {
		DISABLE_READ(astream);
		if ((dlen = notify(HTTP_CHAT_OK, data, dlen, arg)) < 0) {
			return -1;
		}
		return 1;
	}
	if (notify(HTTP_CHAT_CHUNK_TRAILER, data, dlen, arg) < 0) {
		return -1;
	}
	return 0;
}

static int chunked_hdr(ACL_ASTREAM *astream, HTTP_CHAT_CTX *ctx)
{
	HTTP_BODY_NOTIFY notify = ctx->notify.body_notify;
	void *arg = ctx->arg;
	ACL_VSTRING *sbuf;
	char *data, *ptr;
	int   dlen;

	sbuf = acl_aio_gets_peek(astream);
	if (sbuf == NULL) {
		return 0;
	}

	data = acl_vstring_str(sbuf);
	dlen = (int) ACL_VSTRING_LEN(sbuf);
	ACL_VSTRING_RESET(sbuf);

	ctx->body_len += dlen;
	ctx->read_cnt = 0;

	ptr = strchr(data, ' ');
	if (ptr) {
		*ptr = 0;
	}
	ctx->chunk_len = strtoul(data, NULL, 16);
	if (ptr) {
		*ptr = ' ';
	}

	if (notify(HTTP_CHAT_CHUNK_HDR, data, dlen, arg) < 0) {
		return -1;
	}

	if (ctx->chunk_len == 0) {
		ctx->status = CHAT_S_CHUNK_TAL;
		return 0;
	}

	ctx->status = CHAT_S_CHUNK_DAT;
	return 0;
}

static int body_can_read(ACL_ASTREAM *astream, void *context)
{
	const char *myname = "body_can_read";
	HTTP_CHAT_CTX *ctx = (HTTP_CHAT_CTX*) context;
	HTTP_BODY_NOTIFY notify = ctx->notify.body_notify;
	void *arg = ctx->arg;
	int   ret;

	while (1) {
		if ((ret = acl_aio_can_read(astream)) == ACL_VSTREAM_EOF) {
			(void) notify(HTTP_CHAT_ERR_IO, NULL, 0, arg);
			return -1;
		} else if (ret == 0) {
			break;
		}

		switch (ctx->status) {
		case CHAT_S_CHUNK_HDR:
			ret = chunked_hdr(astream, ctx);
			break;
		case CHAT_S_CHUNK_DAT:
			ret = chunked_data(astream, ctx);
			break;
		case CHAT_S_CHUNK_SEP:
			ret = chunked_data_endl(astream, ctx);
			break;
		case CHAT_S_CHUNK_TAL:
			ret = chunked_trailer(astream, ctx);
			break;
		default:
			acl_msg_error("%s(%d): unknown status(%d)",
				myname, __LINE__, ctx->status);
			return -1;
		}
		if (ret < 0) {
			return -1;
		} else if (ret == 1) {
			return 0;
		}
	}

	acl_aio_enable_read(astream, body_can_read, ctx);
	return 0;
}

static void body_get(ACL_ASTREAM *astream, HTTP_CHAT_CTX *ctx)
{
	if (ctx->chunked) {
		ctx->status = CHAT_S_CHUNK_HDR;
	} else {
		ctx->status = CHAT_S_CHUNK_DAT;
	}

	acl_aio_enable_read(astream, body_can_read, ctx);
}

void http_req_body_get_async(HTTP_REQ *request, ACL_ASTREAM *astream,
	 HTTP_BODY_NOTIFY notify, void *arg, int timeout)
{
	const char *myname = "http_req_body_get_async";
	HTTP_CHAT_CTX *ctx;
	HTTP_HDR *hdr;

	if (request == NULL || astream == NULL || notify == NULL) {
		acl_msg_fatal("%s: input invalid", myname);
	}
	if (request->hdr_req == NULL) {
		acl_msg_fatal("%s: hdr null", myname);
	}

	hdr = &(request->hdr_req->hdr);
	if (hdr->content_length <= 0) {
		acl_msg_warn("%s: content_length(%lld)",
			myname, hdr->content_length);
	}

	ctx                     = request->hdr_req->hdr.chat_ctx;
	ctx->hdr                = hdr;
	ctx->stream             = astream;
	ctx->timeout            = timeout;
	ctx->chunked            = 0;
	ctx->chunk_len          = hdr->content_length;
	ctx->read_cnt           = 0;
	ctx->body_len           = 0;
	ctx->notify.body_notify = notify;
	ctx->arg                = arg;

	body_get(astream, ctx);
}

void http_res_body_get_async(HTTP_RES *respond, ACL_ASTREAM *astream,
	HTTP_BODY_NOTIFY notify, void *arg, int timeout)
{
	const char *myname = "http_res_body_get_async";
	HTTP_CHAT_CTX *ctx;
	HTTP_HDR *hdr;

	if (respond == NULL || astream == NULL || notify == NULL) {
		acl_msg_fatal("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
	}
	if (respond->hdr_res == NULL) {
		acl_msg_fatal("%s: hdr null", myname);
	}

	hdr = &(respond->hdr_res->hdr);
	if (hdr->debug && hdr->content_length <= 0 && !hdr->chunked) {
		acl_msg_warn("%s, %s(%d): content_length(%lld), status(%d)",
			__FILE__, myname, __LINE__, hdr->content_length,
			respond->hdr_res->reply_status);
	}

	ctx                     = respond->hdr_res->hdr.chat_ctx;
	ctx->hdr                = hdr;
	ctx->stream             = astream;
	ctx->timeout            = timeout;
	ctx->chunked            = hdr->chunked;
	ctx->chunk_len          = hdr->content_length;
	ctx->read_cnt           = 0;
	ctx->body_len           = 0;
	ctx->notify.body_notify = notify;
	ctx->arg                = arg;

	body_get(astream, ctx);
}
/*----------------------------------------------------------------------------*/
