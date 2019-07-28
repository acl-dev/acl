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

#include "aio.h"

#define	WRITE_SAFE_ENABLE(x, callback) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISWR) == 0) {  \
		(x)->flag |= ACL_AIO_FLAG_ISWR;  \
		acl_event_enable_write((x)->aio->event, (x)->stream,  \
			(x)->timeout, callback, (x));  \
	}  \
} while (0)

#define WRITE_SAFE_DIABLE(x) do {  \
	if (((x)->flag & ACL_AIO_FLAG_ISWR) != 0) {  \
		(x)->flag &= ~ACL_AIO_FLAG_ISWR;  \
		(x)->can_write_fn = NULL;  \
		(x)->can_write_ctx = NULL;  \
		acl_event_disable_write((x)->aio->event, (x)->stream);  \
	}  \
} while (0)

#define WRITE_IOCP_CLOSE(x) do {  \
	WRITE_SAFE_DIABLE((x));  \
	(x)->flag |= ACL_AIO_FLAG_IOCP_CLOSE;  \
	acl_aio_iocp_close((x));  \
} while (0)

static int write_complete_callback(ACL_ASTREAM *astream)
{
	int   ret = 0;

	/* 流引用计数加1，以防止流被异常关闭 */
	astream->nrefer++;

	if (astream->write_handles) {
		ACL_ITER iter;
		AIO_WRITE_HOOK *handle;

		/* 必须将各个回调句柄从回调队列中一一提出置入一个单独队列中,
		 * 因为 ACL_AIO 在回调过程中有可能发生嵌套，防止被重复调用
		 */

		while (1) {
			handle = astream->write_handles->pop_back(
				astream->write_handles);
			if (handle == NULL)
				break;
			astream->writer_fifo.push_back(
					&astream->writer_fifo, handle);
		}

		acl_foreach_reverse(iter, &astream->writer_fifo) {
			handle = (AIO_WRITE_HOOK*) iter.data;
			if (handle->disable)
				continue;

			/* 回调写成功注册函数 */
			ret = handle->callback(astream, handle->ctx);
			if (ret != 0) {
				astream->nrefer--;
				return (ret);
			}
		}
	}

	astream->nrefer--;
	return (ret);
}

/* 尝试发送流写队列里的数据，返回值为写队列里还剩余的数据长度或写失败 */

static int __try_fflush(ACL_ASTREAM *astream)
{
	const char *myname = "__try_fflush";
	ACL_VSTRING *str;
	const char *ptr;
	int   n, dlen;
	int   i = 0;

	/* 针对写队列，也许调用 writev 会更好，应该那个写的效率会更高些
	 * --- zsx */

	while (1) {
		/* 提取流写队列的数据头部分 */
		str = acl_fifo_head(&astream->write_fifo);
		if (str == NULL) {
			/* 说明写队列已经为空 */
			if (astream->write_left != 0)
				acl_msg_fatal("%s: write_left(%d) != 0",
					myname, astream->write_left);
			return (astream->write_left);
		}

		/* 计算本数据块的长度及数据开始位置, write_offset 仅是本数据块
		 * 的相对位置, 即相对于数据块的起始位置的相对长度，所以 dlen
		 * 仅是本数据块中在本次写操作需要被写的数据的长度；而
		 * write_left 则是全局性的，是整个写队列的数据长度，需要将该
		 * 这两个变量区分开，将来也许应该将 write_offset 也设定为全局
		 * 性的。--- zsx :)
		 */

		dlen = (int) ACL_VSTRING_LEN(str) - astream->write_offset;
		ptr = acl_vstring_str(str) + astream->write_offset;
		/* 开始进行非阻塞式写操作 */
		n = acl_vstream_write(astream->stream, ptr, dlen);
		if (n == ACL_VSTREAM_EOF) {
			if (acl_last_error() != ACL_EAGAIN) {
				astream->flag |= ACL_AIO_FLAG_DEAD;
				return (-1);
			}
			/* 本次写操作未写入数据，仅需要返回剩余数据长度即可 */
			return (astream->write_left);
		}

		/* 重新计算写队列里剩余数据的总长度 */
		astream->write_left -= n;

		if (n < dlen) {
			/* 未能将本数据块的可写数据全部写入，所以需要重新计算
			 * 该数据块的可写数据的相对偏移位置
			 */
			astream->write_offset += n;
			return (astream->write_left);
		}

		/* 将本数据块从写队列中剔除并释放该数据块所占的内存 */

		str = acl_fifo_pop(&astream->write_fifo);
		acl_vstring_free(str);

		/* 重置 write_offset 以为写入下一个数据块做准备, 将来该变量
		 * 作为写队列的相对偏移变量后会更好些. --- zsx
		 */
		astream->write_offset = 0;

		/* 如果本轮写队列操作的循环次数过多，则应返回，以给其它的数据
		 * 连接可读写的机会, 别忘了，在单线程条件下进行非阻塞写时可能
		 * 会有很多数据连接需要被处理，总之，数据面前大家平等:)
		 */
		if (++i >= 10) {
			if (acl_msg_verbose)
				acl_msg_warn("%s: write_left=%d, loop=%d",
					myname, astream->write_left, i);
			return (astream->write_left);
		}
	}
}

/* 流出错或流可写时触发了流的写事件处理函数 */

static void __writen_notify_callback(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	const char *myname = "__write_notify_callback";
	ACL_ASTREAM *astream = (ACL_ASTREAM *) context;
	int   nleft;

	WRITE_SAFE_DIABLE(astream);

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		/* 流发生了错误，启动IO完成延迟关闭关闭 */
		WRITE_IOCP_CLOSE(astream);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		/* 写操作超时，若用户的回调函数返回 -1 则采用 IO 完成延迟
		 * 关闭过程 */
		if (aio_timeout_callback(astream) < 0) {
			WRITE_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因本次写IO已经成功完成，
			 * 所以需要完成流的IO延迟关闭过程
			 */
			WRITE_IOCP_CLOSE(astream);
		} else {
			/* 说明用户希望继续等待写事件 */
			WRITE_SAFE_ENABLE(astream, __writen_notify_callback);
		}
		return;
	}

	if ((event_type & ACL_EVENT_WRITE) == 0)
		acl_msg_fatal("%s: unknown event: %d", myname, event_type);

	/* 尝试发送流的写队列里的数据 */
	nleft = __try_fflush(astream);

	if (nleft < 0) {
		/* 尝试写失败则启动IO完成延迟过程 */
		WRITE_IOCP_CLOSE(astream);
	} else if (nleft == 0) {
		/* 之前流的写队列为空或已经成功清空了写队列里的所有数据 */
		int   ret;

		ret = write_complete_callback(astream);
		if (ret < 0) {
			/* 用户希望关闭该流，则启动IO完成延迟关闭过程 */
			WRITE_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 之前该流已经被设置了IO完成延迟关闭标志位，
			 * 则再次启动IO完成延迟关闭过程
			 */
			WRITE_IOCP_CLOSE(astream);
		}
	} else {
		/* 说明写队列里的数据未发送完毕，需要再次发送，所以将写事件置
		 * 入事件监控中 */
		WRITE_SAFE_ENABLE(astream, __writen_notify_callback);
	}
}

void acl_aio_writen(ACL_ASTREAM *astream, const char *data, int dlen)
{
	const char *myname = "acl_aio_writen";
	ACL_VSTRING *str;
	int   n;

	if ((astream->flag & (ACL_AIO_FLAG_DELAY_CLOSE | ACL_AIO_FLAG_DEAD)))
		return;

	/* 将嵌套计数加1，以防止嵌套层次太深而使栈溢出 */
	astream->write_nested++;

	if (astream->write_nested >= astream->write_nested_limit) {
		/* 递归写次数达到了阀值，只需记个警告信息，因为有嵌套限制 */
		if (acl_msg_verbose)
			acl_msg_warn("%s(%d): write_nested(%d) >= max(%d)",
				myname, __LINE__, astream->write_nested,
				astream->write_nested_limit);
		n = 0;
	}

	/* 如果嵌套调用次数小于阀值，则允许进行嵌套调用 */
	/* 先尝试写流的写队列中的数据 */
	else if ((n = __try_fflush(astream)) < 0) {
		/* 说明尝试写失败，需要关闭流 */
		astream->write_nested--;
		WRITE_IOCP_CLOSE(astream);
		return;
	} else if (n > 0) {
		/* __try_fflush 并未全部写完写队列的所有的数据，所以
		 * 也需要本次的数据全部加入流的写队列中
		 */
		n = 0;
	}

	/* __try_fflush 返回 0, 队列中数据已经清空，可以真正调用一次写操作 */
	else if ((n = acl_vstream_write(astream->stream, data, dlen)) == dlen)
	{
		/* 说明已经成功写入了全部的数据 */

		if (write_complete_callback(astream) < 0) {
			astream->write_nested--;

			/* 调用者希望关闭流 */
			WRITE_IOCP_CLOSE(astream);
		} else if ((astream->flag & ACL_AIO_FLAG_IOCP_CLOSE)) {
			astream->write_nested--;

			/* 因为本次写IO已经成功完成，所以需要
			 * 完成流的IO延迟关闭过程
			 */
			WRITE_IOCP_CLOSE(astream);
		} else
			astream->write_nested--;

		return;
	} else if (n == ACL_VSTREAM_EOF) {
		if (acl_last_error() != ACL_EAGAIN) {
			astream->write_nested--;
			WRITE_IOCP_CLOSE(astream);
			astream->flag |= ACL_AIO_FLAG_DEAD;
			return;
		}

		/* 若未写任何数据且诊断该流的对等点并未关闭，
		 * 则将此次数据置入该流的写队列中
		 */
		n = 0;
	}

	/* 否则，禁止继续嵌套，将写事件置于事件监控中，从而减少嵌套层次 */

	astream->write_nested--;

	/* XXX: In acl_vstring_memcpy, vstring_extend should not be called */

	/* 将数据置入该流的写队列中 */

	str = acl_vstring_alloc(dlen - n + 1);
	acl_vstring_memcpy(str, data + n, dlen - n);
	acl_fifo_push(&astream->write_fifo, str);
	astream->write_left += dlen - n;

	/* 将该流的写事件置入事件监控中 */
	WRITE_SAFE_ENABLE(astream, __writen_notify_callback);
}

void acl_aio_vfprintf(ACL_ASTREAM *astream, const char *fmt, va_list ap)
{
	const char *myname = "acl_aio_vfprintf";
	ACL_VSTRING *str;
	int   n = 0, len;

	if ((astream->flag & (ACL_AIO_FLAG_DELAY_CLOSE | ACL_AIO_FLAG_DEAD)))
		return;

	str = acl_vstring_alloc(__default_line_length);
	acl_vstring_vsprintf(str, fmt, ap);

	/* 将嵌套计数加1，以防止嵌套层次太深而使栈溢出 */
	astream->write_nested++;

	if (astream->write_nested >= astream->write_nested_limit) {
		/* 递归嵌套写次数达到了规定的阀值，只需记个警告信息即可，
		 * 因为有嵌套限制 */
		if (acl_msg_verbose)
			acl_msg_warn("%s: write_nested(%d) >= max(%d)",
				myname, astream->write_nested,
				astream->write_nested_limit);
		n = 0;
	}

	/* 如果嵌套调用次数小于阀值，则允许进行嵌套调用 */
	/* 先尝试写流的写队列中的数据 */
	else if ((n = __try_fflush(astream)) < 0) {
		/* 说明尝试写失败，需要关闭流 */
		astream->write_nested--;
		WRITE_IOCP_CLOSE(astream);
		return;
	} else if (n == 0) {
		/* __try_fflush 返回的是队列中的数据已经清空，
		 * 本次可以真正调用一次写操作
		 */
		const char *ptr = acl_vstring_str(str);
		len = (int) ACL_VSTRING_LEN(str);
		n = acl_vstream_write(astream->stream, ptr, len);
		if (n == ACL_VSTREAM_EOF) {
			if (acl_last_error() != ACL_EAGAIN) {
				astream->flag |= ACL_AIO_FLAG_DEAD;
				astream->write_nested--;
				WRITE_IOCP_CLOSE(astream);
				return;
			}
			/* 如果未写任何数据且诊断该流的对等点并未关闭，
			 * 则将此次数据置入该流的写队列中
			 */
			n = 0;
		} else if (n == len) {
			/* 说明已经成功写入了全部的数据 */

			int ret = write_complete_callback(astream);
			acl_vstring_free(str);
			astream->write_nested--;

			if (ret < 0) /* 调用者希望关闭流 */
				WRITE_IOCP_CLOSE(astream);
			else if ((astream->flag & ACL_AIO_FLAG_IOCP_CLOSE)) {
				/* 因为本次写IO已经成功完成，所以需要
				 * 完成流的IO延迟关闭过程 */
				WRITE_IOCP_CLOSE(astream);
			}

			return;
		}
	}

	/* 只是成功写入了部分流，n 表示还剩余的数据长度 */
	else {
		/* __try_fflush 并未全部写完写队列的所有的数据，所以也
		 * 需要本次的数据全部加入流的写队列中
		 */
		n = 0;
	}

	/* 否则，禁止继续嵌套，将写事件置于事件监控中，从而减少嵌套层次 */

	astream->write_nested--;

	acl_assert(n >= 0);

	len = (int) ACL_VSTRING_LEN(str);
	if (n < len)
		acl_vstring_memmove(str, acl_vstring_str(str) + n, len - n);

	/* 将数据置入该流的写队列中 */
	acl_fifo_push(&astream->write_fifo, str);
	astream->write_left += (int) ACL_VSTRING_LEN(str);

	/* 将该流的写事件置入事件监控中 */
	WRITE_SAFE_ENABLE(astream, __writen_notify_callback);
}

void acl_aio_fprintf(ACL_ASTREAM *astream, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	acl_aio_vfprintf(astream, fmt, ap);
	va_end(ap);
}

void acl_aio_writev(ACL_ASTREAM *astream, const struct iovec *vector, int count)
{
	const char *myname = "acl_aio_writev";
	ACL_VSTRING *str;
	int   n, i, j, dlen;

	acl_assert(vector);
	acl_assert(count);

	if ((astream->flag & (ACL_AIO_FLAG_DELAY_CLOSE | ACL_AIO_FLAG_DEAD)))
		return;

	/* 将嵌套计数加1，以防止嵌套层次太深而使栈溢出 */
	astream->write_nested++;

	if (astream->write_nested >= astream->write_nested_limit) {
		/* 递归嵌套写次数达到了规定的阀值，只需记个警告信息即可，
		 * 因为有嵌套限制 */
		if (acl_msg_verbose)
			acl_msg_warn("%s(%d): write_nested(%d) >= max(%d)",
				myname, __LINE__, astream->write_nested,
				astream->write_nested_limit);
		n = 0;

	}

	/* 如果嵌套调用次数小于阀值，则允许进行嵌套调用 */
	/* 先尝试写流的写队列中的数据 */
	else if ((n = __try_fflush(astream)) < 0) {
		/* 说明尝试写失败，需要关闭流 */
		astream->write_nested--;
		WRITE_IOCP_CLOSE(astream);
		return;
	} else if (n > 0) {
		/* __try_fflush 并未全部写完写队列的所有的数据，所以也
		 * 需要本次的数据全部加入流的写队列中
		 */
		n = 0;
	}

	/* __try_fflush 返回的是队列中的数据已经清空，
	 * 本次可以真正调用一次写操作
	 */
	else if ((n = acl_vstream_writev(astream->stream, vector, count))
		== ACL_VSTREAM_EOF)
	{
		if (acl_last_error() != ACL_EAGAIN) {
			astream->flag |= ACL_AIO_FLAG_DEAD;
			astream->write_nested--;
			WRITE_IOCP_CLOSE(astream);
			return;
		}
		/* 如果未写任何数据且诊断该流的对等点并未关闭，
		 * 则将此次数据置入该流的写队列中
		 */
		n = 0;
	}

	/* 计算剩余的未发送的数据长度 */

	for (i = 0; i < count; i++) {
		if (n >= (int) vector[i].iov_len) {
			/* written */
			n -= (int) vector[i].iov_len;
		} else {
			/* partially written */
			break;
		}
	}

	if (i >= count) {
		int   ret;

		acl_assert(n == 0);
		ret = write_complete_callback(astream);
		astream->write_nested--;

		if (ret < 0) {
			/* 调用者希望关闭流 */
			WRITE_IOCP_CLOSE(astream);
		} else if ((astream->flag & ACL_AIO_FLAG_IOCP_CLOSE)) {
			/* 因为本次写IO已经成功完成，所以需要完成流的IO延迟
			 * 关闭过程 */
			WRITE_IOCP_CLOSE(astream);
		}

		return;
	}

	/* 否则，禁止继续嵌套，将写事件置于事件监控中，从而减少嵌套层次 */

	astream->write_nested--;

	/* 计算剩余的数据总长度 */

	j = i;
	dlen = (int) vector[i].iov_len - n;
	i++;  /* skipt this */
	for (; i < count; i++) {
		dlen += (int) vector[i].iov_len;
	}

	/* 将数据置入该流的写队列中 */

	/* 先分配一个足够大的缓冲区以能容下所有的剩余的数据 */
	str = acl_vstring_alloc(dlen + 1);
 
	acl_vstring_memcpy(str, (const char*) vector[j].iov_base + n,
		vector[j].iov_len - n);
	for (i = j + 1; i < count; i++) {
		acl_vstring_memcat(str, vector[i].iov_base, vector[i].iov_len);
	}

	acl_fifo_push(&astream->write_fifo, str);
	astream->write_left += dlen;

	/* 将该流的写事件置入事件监控中 */
	WRITE_SAFE_ENABLE(astream, __writen_notify_callback);
}

static void can_write_callback(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	const char *myname = "can_write_callback";
	ACL_ASTREAM *astream = (ACL_ASTREAM*) context;

	WRITE_SAFE_DIABLE(astream);

	if ((event_type & ACL_EVENT_XCPT) != 0) {
		WRITE_IOCP_CLOSE(astream);
		return;
	} else if ((event_type & ACL_EVENT_RW_TIMEOUT) != 0) {
		if (aio_timeout_callback(astream) < 0) {
			WRITE_IOCP_CLOSE(astream);
		} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
			/* 该流正处于IO延迟关闭状态，因为本次读IO已经成功完成，
			* 所以需要完成流的IO延迟关闭过程
			*/
			WRITE_IOCP_CLOSE(astream);
		} else {
			WRITE_SAFE_ENABLE(astream, can_write_callback);
		}
		return;
	}

	if (astream->can_write_fn == NULL)
		printf("%s(%d): can_write_fn null for astream(%p)",
			myname, __LINE__, astream);

	astream->nrefer++;
	if (astream->can_write_fn(astream, astream->can_write_ctx) < 0) {
		astream->nrefer--;
		WRITE_IOCP_CLOSE(astream);
	} else if (astream->flag & ACL_AIO_FLAG_IOCP_CLOSE) {
		astream->nrefer--;
		WRITE_IOCP_CLOSE(astream);
	} else {
		astream->nrefer--;
	}
}
void acl_aio_enable_write(ACL_ASTREAM *astream,
	ACL_AIO_NOTIFY_FN can_write_fn, void *context)
{
	if ((astream->flag & (ACL_AIO_FLAG_DELAY_CLOSE | ACL_AIO_FLAG_DEAD)))
		return;
        astream->can_write_fn = can_write_fn;
        astream->can_write_ctx = context;
	WRITE_SAFE_ENABLE(astream, can_write_callback);
}

void acl_aio_disable_write(ACL_ASTREAM *astream)
{
	if ((astream->flag & ACL_AIO_FLAG_ISWR) == 0)
		return;
	astream->flag &= ~ACL_AIO_FLAG_ISWR;
        astream->can_write_fn = NULL;
        astream->can_write_ctx = NULL;
	if (astream->stream)
		acl_event_disable_write(astream->aio->event, astream->stream);
}

int acl_aio_iswset(ACL_ASTREAM *astream)
{
	const char *myname = "acl_aio_iswset";

	if (astream == NULL)
		acl_msg_fatal("%s: input invalid", myname);
	if (astream->stream == NULL)
		return (0);

	return (acl_event_iswset(astream->aio->event, astream->stream));
}
