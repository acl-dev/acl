#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "net/acl_net.h"
#include "aio/acl_aio.h"

#endif

#include "../event/events.h"
#include "aio.h"

#if 0

#define READ_SAFE_ENABLE(x, callback) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISRD) == 0) {  \
		(x)->flag |= ACL_AIO_FLAG_ISRD;  \
		acl_event_enable_read((x)->aio->event, (x)->stream,  \
			(x)->timeout, callback, (x));  \
	}  \
} while (0)

#define READ_SAFE_DISABLE(x) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISRD) != 0) {  \
		(x)->flag &= ~ACL_AIO_FLAG_ISRD;  \
		(x)->can_read_fn = NULL;  \
		(x)->can_read_ctx = NULL;  \
		acl_event_disable_read((x)->aio->event, (x)->stream);  \
	}  \
} while (0)

#else

#define READ_SAFE_ENABLE(x, callback) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISRD) == 0) {  \
		(x)->flag |= ACL_AIO_FLAG_ISRD;  \
		(x)->aio->event->enable_read_fn((x)->aio->event,  \
			(x)->stream, (x)->timeout, callback, (x));  \
	}  \
} while (0)

#define READ_SAFE_DISABLE(x) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISRD) != 0) {  \
		(x)->flag &= ~ACL_AIO_FLAG_ISRD;  \
		(x)->can_read_fn = NULL;  \
		(x)->can_read_ctx = NULL;  \
		(x)->aio->event->disable_read_fn((x)->aio->event, (x)->stream);  \
	}  \
} while (0)

#endif

# define READ_IOCP_CLOSE(x) do {  \
	READ_SAFE_DISABLE((x));  \
	(x)->flag |= ACL_AIO_FLAG_IOCP_CLOSE;  \
	acl_aio_iocp_close((x));  \
} while (0)

/* 统一的读事件处理回调接口 */
static void main_read_callback(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	ACL_ASTREAM *astream = (ACL_ASTREAM*) context;

	/* 调用 __gets_notify_callback/__read_notify_callback/__readn_notify_callback */

	astream->event_read_callback(event_type, astream);
}

static int read_complete_callback(ACL_ASTREAM *astream, char *data, int len)
{
	int   ret = 0;

	/* 必须将缓存区复位，否则在下一次读事件(如有数据或出错)触发时，
	 * 因为上述的 if (astream->count <= n) {} 而导致 fatal
	 * ---2008.11.5, zsx
	 */
	/* 需要将缓冲区清空，以防被重复利用 */
	ACL_VSTRING_RESET(&astream->strbuf);

	/* 流引用计数加1，以防止流被异常关闭 */
	astream->nrefer++;

	if (astream->read_handles) {
		ACL_ITER iter;
		AIO_READ_HOOK *handle;

		/* XXX: 必须将各个回调句柄从回调队列中一一提出置入一个单独队列中,
		 * 因为 ACL_AIO 在回调过程中有可能发生嵌套，防止被重复调用
		 */

		while (1) {
			handle = astream->read_handles->pop_back(
				astream->read_handles);
			if (handle == NULL)
				break;
			astream->reader_fifo.push_back(&astream->reader_fifo, handle);
		}

		acl_foreach_reverse(iter, &astream->reader_fifo) {
			handle = (AIO_READ_HOOK*) iter.data;
			if (handle->disable)
				continue;
			ret = handle->callback(astream, handle->ctx, data, len);
			if (ret != 0) {
				astream->nrefer--;
				return ret;
			}
		}
	}

	astream->nrefer--;
	return ret;
}

/* 尝试性读一行数据
 * @param astream {ACL_ASTREAM*}
 * @return {int} 返回值
 *  -1: 表示出错，或希望关闭流
 *   0: 表示数据未准备好或用户设置了非连续读
 *   1: 表示数据已准备好，且已经调用过一次用户的回调函数且用户希望继续读
 */

static int __gets_peek(ACL_ASTREAM *astream)
{
	int   n, ready = 0;
	
	n = astream->read_ready_fn(astream->stream, &astream->strbuf, &ready);

	if (n == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (astream->stream->errnum == ACL_EWOULDBLOCK)
#else
		if (astream->stream->errnum == ACL_EWOULDBLOCK
			|| astream->stream->errnum == ACL_EAGAIN)
#endif
		{
			READ_SAFE_ENABLE(astream, main_read_callback);
			return 0;
		}

		/* XXX: 必须查看缓冲区中是否还有数据,
		 * 必须兼容数据读不够的情况!
		 */
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0) {
			char *ptr = acl_vstring_str(&astream->strbuf);
			int   len = (int) ACL_VSTRING_LEN(&astream->strbuf);

			(void) read_complete_callback(astream, ptr, len);
		}
		/* 读出错，需要关闭流 */
		astream->flag |= ACL_AIO_FLAG_DEAD;
		READ_IOCP_CLOSE(astream);
		return -1;
	} else if (ready) {
		char *ptr = acl_vstring_str(&astream->strbuf);
		int   len = (int) ACL_VSTRING_LEN(&astream->strbuf);

		/* 回调用户的读行成功处理函数 */
		n = read_complete_callback(astream, ptr, len);

		/* 当用户希望关闭流或该流正处于延迟关闭状态，则需要调用
		 * 流的IO延迟关闭过程
		 */
		if (n < 0 || (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE)) {
			READ_IOCP_CLOSE(astream);
			return -1;
		} else if (astream->keep_read == 0
			|| (astream->flag & ACL_AIO_FLAG_ISRD) == 0)
		{
			return 0;
		}
		return len;
	}

	/* 未读到所要求的一行数据，继续监控该流的读行事件 */
	READ_SAFE_ENABLE(astream, main_read_callback);
	return 0;
}

/* 由事件监控过程回调触发的读行事件处理过程 */

static void __gets_notify_callback(int event_type, ACL_ASTREAM *astream)
{
	const char *myname = "__gets_notify_callback";

	if (astream->keep_read == 0)
		READ_SAFE_DISABLE(astream);

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		/* 该流出错，但是有可能关闭的事件通知到达时流依然可读，
		 * 则应该保证读优先，直到把操作系统缓冲区中的数据读完
		 * 为止，最后再处理关闭事件，即关闭流
		 */
		int   ret;
		acl_non_blocking(ACL_VSTREAM_SOCK(astream->stream),
			ACL_NON_BLOCKING);
		do {
			astream->stream->read_ready = 1;
			ret = __gets_peek(astream);
			if (astream->keep_read == 0)
				break;
		} while (ret > 0);
		READ_IOCP_CLOSE(astream);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		/* 读流超时，如果应用返回值大于等于0，则希望继续读,
		 * 如果返回值小于0则希望关闭流。有人会有这种需求吗？
		 */
		if (aio_timeout_callback(astream) < 0) {
			READ_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因为本次读IO已经
			 * 成功完成，所以需要完成流的IO延迟关闭过程
			 */
			READ_IOCP_CLOSE(astream);
		} else {
			READ_SAFE_ENABLE(astream, main_read_callback);
		}

		return;
	}

	if ((event_type & ACL_EVENT_READ) == 0)
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);

	/* 尝试性地读数据 */
	while (1) {
		if (__gets_peek(astream) <= 0 || astream->keep_read == 0)
			break;
	}
}

/**
 * 异步读一行数据
 * @param astream {ACL_ASTREAM*}
 * @param nonl {int} 是否自动去掉尾部的 \r\n
 */
static void __aio_gets(ACL_ASTREAM *astream, int nonl)
{
	const char *myname = "__aio_gets";

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return;
	if (astream->stream == NULL)
		acl_msg_fatal("%s: astream->stream null", myname);

	/* 设置读流函数 */
	if (nonl)
		astream->read_ready_fn = acl_vstream_gets_nonl_peek;
	else
		astream->read_ready_fn = acl_vstream_gets_peek;

	if (astream->line_length > 0)
		astream->strbuf.maxlen = astream->line_length;

	astream->event_read_callback = __gets_notify_callback;

	ACL_VSTRING_RESET(&astream->strbuf);

	/* 将嵌套计数加1，以防止嵌套层次太深而使栈溢出 */
	astream->read_nested++;

	/* 当满足回调条件时，有可能是从系统缓冲区中读取数据，也有可能从用户
	 * 缓冲区读数据，对于持续读过程，当用户在回调中取消了读监听，则当用户
	 * 缓冲区中无数据时，而无法监控该流的系统缓冲区，所以对于持续流的读
	 * 操作，必须保证流处于读监听状态
	 */ 
	if (astream->keep_read)
		READ_SAFE_ENABLE(astream, main_read_callback);

	/* 如果嵌套调用次数小于阀值，则允许进行嵌套调用 */
	if (astream->read_nested < astream->read_nested_limit) {
		/* 尝试性地读数据 */
		while (1) {
			if (__gets_peek(astream) <= 0 || astream->keep_read == 0)
				break;
		}
		astream->read_nested--;
		return;
	}

	/* 递归嵌套读次数达到了规定的阀值，
	 * 只需记个警告信息即可，因为有嵌套限制
	 */
	if (acl_msg_verbose)
		acl_msg_warn("%s: read_nested(%d) >= max(%d)", myname,
			astream->read_nested, astream->read_nested_limit);
	/* 否则，不允许继续嵌套，将读事件置于事件监控循环中，以减少嵌套层次 */

	astream->read_nested--;

	/* 将该流的读事件置入事件监控中 */
	READ_SAFE_ENABLE(astream, main_read_callback);
}

void acl_aio_gets(ACL_ASTREAM *astream)
{
	__aio_gets(astream, 0);
}

void acl_aio_gets_nonl(ACL_ASTREAM *astream)
{
	__aio_gets(astream, 1);
}

/* 尝试性读数据
 * @param astream {ACL_ASTREAM*}
 * @return {int} 返回值
 *  -1: 表示出错，或希望关闭流
 *   0: 表示数据未准备好或用户设置了非连续读
 *   1: 表示数据已准备好，且已经调用过一次用户的回调函数且用户希望继续读
 */

static int __read_peek(ACL_ASTREAM *astream)
{
	int   n;

	/* 尝试性地读数据 */
	n = acl_vstream_read_peek(astream->stream, &astream->strbuf);

	if (n == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (astream->stream->errnum == ACL_EWOULDBLOCK)
#else
		if (astream->stream->errnum == ACL_EAGAIN
			|| astream->stream->errnum == ACL_EWOULDBLOCK)
#endif
		{
			READ_SAFE_ENABLE(astream, main_read_callback);
			return 0;
		}

		/* 必须查看缓冲区中是否还有数据, 必须兼容数据读不够的情况! */
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0) {
			char *ptr = acl_vstring_str(&astream->strbuf);
			int   len = (int) ACL_VSTRING_LEN(&astream->strbuf);

			(void) read_complete_callback(astream, ptr, len);
		}
		/* 读出错，需要关闭流 */
		astream->flag |= ACL_AIO_FLAG_DEAD;
		READ_IOCP_CLOSE(astream);
		return -1;
	} else if (n > 0) {
		char *ptr = acl_vstring_str(&astream->strbuf);
		int   len = (int) ACL_VSTRING_LEN(&astream->strbuf);

		/* 回调用户的读成功处理函数 */
		n = read_complete_callback(astream, ptr, len);

		/* 当用户希望关闭流或该流正处于延迟关闭状态，则需要调用
		 * 流的IO延迟关闭过程
		 */
		if (n < 0 || astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			READ_IOCP_CLOSE(astream);
			return -1;
		} else if (astream->keep_read == 0
			|| (astream->flag & ACL_AIO_FLAG_ISRD) == 0)
		{
			return 0;
		}
		return len;
	} else {
		/* 读数据不符合要求，继续监控该读事件 */
		READ_SAFE_ENABLE(astream, main_read_callback);
		return 0;
	}
}

/* 由事件监控过程回调触发的读事件处理过程 */

static void __read_notify_callback(int event_type, ACL_ASTREAM *astream)
{
	const char *myname = "__read_notify_callback";

	if (astream->keep_read == 0)
		READ_SAFE_DISABLE(astream);

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		/* 该流出错，但是有可能关闭的事件通知到达时流依然可读，
		 * 则应该保证读优先，直到把操作系统缓冲区中的数据读完
		 * 为止，最后再处理关闭事件，即关闭流
		 */
		int   ret;
		acl_non_blocking(ACL_VSTREAM_SOCK(astream->stream),
			ACL_NON_BLOCKING);
		do {
			astream->stream->read_ready = 1;
			ret = __read_peek(astream);
		} while (ret > 0);

		READ_IOCP_CLOSE(astream);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		/* 读流超时，如果应用返回值大于等于0，则希望继续读,
		 * 如果返回值小于0则希望关闭流。有人会有这种需求吗？
		 */
		if (aio_timeout_callback(astream) < 0) {
			/* 用户希望关闭流 */
			READ_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因为本次读IO已经成功完成，
			 * 所以需要完成流的IO延迟关闭过程
			 */
			READ_IOCP_CLOSE(astream);
		} else {
			READ_SAFE_ENABLE(astream, main_read_callback);
		}

		return;
	}

	if ((event_type & ACL_EVENT_READ) == 0)
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);

	/* 尝试性地读数据 */
	while (1) {
		if (__read_peek(astream) <= 0 || astream->keep_read == 0)
			break;
	}
}

void acl_aio_read(ACL_ASTREAM *astream)
{
	const char *myname = "acl_aio_read";

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return;
	if (astream->stream == NULL)
		acl_msg_fatal("%s: astream(%p)->stream null",
			myname, astream);

	astream->event_read_callback = __read_notify_callback;
	/* XXX: 必须将缓冲区重置 */
	ACL_VSTRING_RESET(&astream->strbuf);

	/* 当满足回调条件时，有可能是从系统缓冲区中读取数据，也有可能从用户
	 * 缓冲区读数据，对于持续读过程，当用户在回调中取消了读监听，则当用户
	 * 缓冲区中无数据时，而无法监控该流的系统缓冲区，所以对于持续流的读
	 * 操作，必须保证流处于读监听状态
	 */ 
	if (astream->keep_read)
		READ_SAFE_ENABLE(astream, main_read_callback);

	/* 将嵌套计数加1，以防止嵌套层次太深而使栈溢出 */
	astream->read_nested++;

	/* 如果嵌套调用次数小于阀值，则允许进行嵌套调用 */
	if (astream->read_nested < astream->read_nested_limit) {
		/* 尝试性地读数据 */
		while (1) {
			if (__read_peek(astream) <= 0 || astream->keep_read == 0)
				break;
		}
		astream->read_nested--;
		return;
	}

	/* 递归嵌套读次数达到了规定的阀值，只需记个警告信息，因为有嵌套限制 */
	if (acl_msg_verbose)
		acl_msg_warn("%s: read_nested(%d) >= max(%d)", myname,
			astream->read_nested, astream->read_nested_limit);

	/* 否则，不允许继续嵌套，将读事件置于事件监控循环中，减少嵌套层次 */

	astream->read_nested--;

	/* 将该流的读事件置入事件监控中 */
	READ_SAFE_ENABLE(astream, main_read_callback);
}

/* 尝试性读规定数据量的数据
 * @param astream {ACL_ASTREAM*}
 * @return {int} 返回值
 *  -1: 表示出错，或希望关闭流
 *   0: 表示数据未准备好或用户设置了非连续读
 *   1: 表示数据已准备好，且已经调用过一次用户的回调函数且用户希望继续读
 */

static int __readn_peek(ACL_ASTREAM *astream)
{
	const char *myname = "__readn_peek";
	int   n, ready = 0;

	n = (int) ACL_VSTRING_LEN(&astream->strbuf);

	if (astream->count <= n)
		acl_msg_fatal("%s: count(%d) < strlen(%d), read_netsted(%d)",
			myname, astream->count, n, astream->read_nested);

	/* 尝试性地读数据 */
	n = acl_vstream_readn_peek(astream->stream, &astream->strbuf,
		astream->count - n, &ready);
	if (n == ACL_VSTREAM_EOF) {
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (astream->stream->errnum == ACL_EWOULDBLOCK)
#else
		if (astream->stream->errnum == ACL_EAGAIN
			|| astream->stream->errnum == ACL_EWOULDBLOCK)
#endif
		{
			READ_SAFE_ENABLE(astream, main_read_callback);
			return 0;
		}
		/* XXX: 查看缓冲区中是否还有数据, 必须兼容数据读不够的情况! */
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0) {
			char *ptr = acl_vstring_str(&astream->strbuf);
			int   len = (int) ACL_VSTRING_LEN(&astream->strbuf);

			acl_msg_warn("%s: nneed(%d), nread(%d),"
				" read_netsted(%d), nrefer(%d)",
				myname, astream->count, len,
				astream->read_nested, astream->nrefer);

			(void) read_complete_callback(astream, ptr, len);
		}
		/* 读出错或读关闭，需要关闭流 */
		astream->flag |= ACL_AIO_FLAG_DEAD;
		READ_IOCP_CLOSE(astream);
		return -1;
	} else if (ready) {
		/* ok, 已经满足读条件，即已经获得了所要求数据长度的数据 */
		char *ptr = acl_vstring_str(&astream->strbuf);
		int   len = (int) ACL_VSTRING_LEN(&astream->strbuf);

		if (len != astream->count)
			acl_msg_fatal("%s: len: %d != count: %d",
				myname, len, astream->count);

		/* 回调用户的读成功处理函数 */
		n = read_complete_callback(astream, ptr, len);
		if (n < 0 || astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			READ_IOCP_CLOSE(astream);
			return -1;
		} else if (astream->keep_read == 0
			|| (astream->flag & ACL_AIO_FLAG_ISRD) == 0)
		{
			return 0;
		}
		return len;
	} else {
		/* 读数据不符合要求，继续监控该读事件 */
		READ_SAFE_ENABLE(astream, main_read_callback);
		return 0;
	}
}

/* 读事件触发回调处理函数 */

static void __readn_notify_callback(int event_type, ACL_ASTREAM *astream)
{
	const char *myname = "__readn_notify_callback";

	if (astream->keep_read == 0)
		READ_SAFE_DISABLE(astream);

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		/* 该流出错，但是有可能关闭的事件通知到达时流依然可读，
		 * 则应该保证读优先，直到把操作系统缓冲区中的数据读完
		 * 为止，最后再处理关闭事件，即关闭流
		 */
		int   ret;
		acl_non_blocking(ACL_VSTREAM_SOCK(astream->stream),
			ACL_NON_BLOCKING);
		do {
			astream->stream->read_ready = 1;
			ret = __readn_peek(astream);
		} while (astream->keep_read && ret > 0);

		READ_IOCP_CLOSE(astream);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		/* 读流超时，如果应用返回值大于等于0，则希望继续读,
		 * 如果返回值小于0则希望关闭流。有人会有这种需求吗？
		 */
		if (aio_timeout_callback(astream) < 0) {
			READ_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因为本次读IO已经成功完成，
			 * 所以需要完成流的IO延迟关闭过程
			 */
			READ_IOCP_CLOSE(astream);
		} else {
			READ_SAFE_ENABLE(astream, main_read_callback);
		}
		return;
	}

	if ((event_type & ACL_EVENT_READ) == 0)
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);

	if (astream->stream == NULL)
		acl_msg_fatal("%s: stream null", myname);

	while (1) {
		if (__readn_peek(astream) <= 0 || astream->keep_read == 0)
			break;
	}
}

void acl_aio_readn(ACL_ASTREAM *astream, int count)
{
	const char *myname = "acl_aio_readn";

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return;
	if (count <= 0)
		acl_msg_fatal("%s: count(%d) <= 0", myname, count);

	/* 设置回调函数 */
	astream->event_read_callback = __readn_notify_callback;
	/* count 表示用户希望读的数据总长度 */
	astream->count = count;

	ACL_VSTRING_RESET(&astream->strbuf);

	/* 当满足回调条件时，有可能是从系统缓冲区中读取数据，也有可能从用户
	 * 缓冲区读数据，对于持续读过程，当用户在回调中取消了读监听，则当用户
	 * 缓冲区中无数据时，而无法监控该流的系统缓冲区，所以对于持续流的读
	 * 操作，必须保证流处于读监听状态
	 */ 
	if (astream->keep_read)
		READ_SAFE_ENABLE(astream, main_read_callback);

	/* 将嵌套计数加1，以防止嵌套层次太深而使栈溢出 */
	astream->read_nested++;

	/* 如果嵌套调用次数小于阀值，则允许进行嵌套调用 */
	if (astream->read_nested < astream->read_nested_limit) {
		/* 尝试性地读数据 */
		while (1) {
			if (__readn_peek(astream) <= 0 || astream->keep_read == 0)
				break;
		}
		astream->read_nested--;
		return;
	}

	/* 递归嵌套读次数达到了规定的阀值，只需记个警告信息，因为有嵌套限制 */
	if (acl_msg_verbose)
		acl_msg_warn("%s: read_nested(%d) >= max(%d)", myname,
			astream->read_nested, astream->read_nested_limit);

	/* 否则，不允许继续嵌套，将读事件置于事件监控循环中，减少嵌套层次 */

	astream->read_nested--;
	
	/* 将该流的读事件置入事件监控中 */
	READ_SAFE_ENABLE(astream, main_read_callback);
}

ACL_VSTRING *acl_aio_gets_peek(ACL_ASTREAM *astream)
{
	int   ready = 0;

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return NULL;
	if (acl_vstream_gets_peek(astream->stream,
			&astream->strbuf, &ready) == ACL_VSTREAM_EOF
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		&& astream->stream->errnum != ACL_EAGAIN
#endif
		&& astream->stream->errnum != ACL_EWOULDBLOCK)
	{
		astream->flag |= ACL_AIO_FLAG_DEAD;
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0)
			return (&astream->strbuf);
		else
			return NULL;
	} else if (ready)
		return &astream->strbuf;
	else
		return NULL;
}

ACL_VSTRING *acl_aio_gets_nonl_peek(ACL_ASTREAM *astream)
{
	int   ready = 0;

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return NULL;
	if (acl_vstream_gets_nonl_peek(astream->stream,
		&astream->strbuf, &ready) == ACL_VSTREAM_EOF
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		&& astream->stream->errnum != ACL_EAGAIN
#endif
		&& astream->stream->errnum != ACL_EWOULDBLOCK)
	{
		astream->flag |= ACL_AIO_FLAG_DEAD;
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0)
			return &astream->strbuf;
		else
			return NULL;
	} else if (ready)
		return &astream->strbuf;
	else
		return NULL;
}

ACL_VSTRING *acl_aio_read_peek(ACL_ASTREAM *astream)
{
	int   n;

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return NULL;
	if ((n = acl_vstream_read_peek(astream->stream,
		&astream->strbuf)) == ACL_VSTREAM_EOF
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		&& astream->stream->errnum != ACL_EAGAIN
#endif
		&& astream->stream->errnum != ACL_EWOULDBLOCK)
	{
		astream->flag |= ACL_AIO_FLAG_DEAD;
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0)
			return &astream->strbuf;
		else
			return NULL;
	} else if (n > 0)
		return &astream->strbuf;
	else
		return NULL;
}

ACL_VSTRING *acl_aio_readn_peek(ACL_ASTREAM *astream, int count)
{
	int   ready = 0;

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return NULL;
	if (acl_vstream_readn_peek(astream->stream,
		&astream->strbuf, count, &ready) == ACL_VSTREAM_EOF
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		&& astream->stream->errnum != ACL_EAGAIN
#endif
		&& astream->stream->errnum != ACL_EWOULDBLOCK)
	{
		astream->flag |= ACL_AIO_FLAG_DEAD;
		if (ACL_VSTRING_LEN(&astream->strbuf) > 0)
			return &astream->strbuf;
		else
			return NULL;
	} else if (ready)
		return &astream->strbuf;
	else
		return NULL;
}

int acl_aio_can_read(ACL_ASTREAM *astream)
{
	return acl_vstream_can_read(astream->stream);
}

static void can_read_callback(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	ACL_ASTREAM *astream = (ACL_ASTREAM*) context;

	if (astream->keep_read == 0)
		READ_SAFE_DISABLE(astream);

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		READ_IOCP_CLOSE(astream);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		if (aio_timeout_callback(astream) < 0) {
			READ_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因为本次读IO已经成功完成，
			 * 所以需要完成流的IO延迟关闭过程
			 */
			READ_IOCP_CLOSE(astream);
		} else {
			READ_SAFE_ENABLE(astream, can_read_callback);
		}
		return;
	}

	astream->nrefer++;
	if (astream->can_read_fn(astream, astream->can_read_ctx) < 0) {
		astream->nrefer--;
		READ_IOCP_CLOSE(astream);
	} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
		astream->nrefer--;
		READ_IOCP_CLOSE(astream);
	} else
		astream->nrefer--;
}

void acl_aio_enable_read(ACL_ASTREAM *astream,
	ACL_AIO_NOTIFY_FN can_read_fn, void *context)
{
	int   ret;

	if ((astream->flag & ACL_AIO_FLAG_DELAY_CLOSE))
		return;

	READ_SAFE_ENABLE(astream, can_read_callback);

	astream->can_read_fn = can_read_fn;
	astream->can_read_ctx = context;

	++astream->read_nested;

	if ((ret = acl_vstream_can_read(astream->stream)) == ACL_VSTREAM_EOF) {
		READ_IOCP_CLOSE(astream);
		astream->flag |= ACL_AIO_FLAG_DEAD;
	} else if (ret > 0 && astream->read_nested < astream->read_nested_limit) {
		can_read_callback(ACL_EVENT_READ, astream->aio->event,
			astream->stream , astream);
	}

	--astream->read_nested;
}

void acl_aio_disable_read(ACL_ASTREAM *astream)
{
	if ((astream->flag & ACL_AIO_FLAG_ISRD) == 0)
		return;
	astream->flag &= ~ACL_AIO_FLAG_ISRD;
	astream->can_read_fn = NULL;
	astream->can_read_ctx = NULL;
	if (astream->stream)
		acl_event_disable_read(astream->aio->event, astream->stream);
}

int acl_aio_isrset(ACL_ASTREAM *astream)
{
	if (astream->stream == NULL)
		return 0;

	return acl_event_isrset(astream->aio->event, astream->stream);
}

void acl_aio_stream_set_line_length(ACL_ASTREAM *astream, int len)
{
	astream->line_length = len;
}

int acl_aio_stream_get_line_length(ACL_ASTREAM *astream)
{
	return astream->line_length;
}
