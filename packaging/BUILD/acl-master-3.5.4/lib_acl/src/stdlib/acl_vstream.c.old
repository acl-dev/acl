#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h> /* for S_IREAD */

#ifdef  ACL_MS_WINDOWS
# include <io.h>
#elif defined(ACL_UNIX)
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <arpa/inet.h>
#else
# error "unknown OS type"
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_sane_inet.h"
#include "stdlib/acl_vstream.h"

#endif

#include "../event/events_fdtable.h"

static char __empty_string[] = "";

 /*
  * Initialization of the three pre-defined streams. Pre-allocate a static
  * I/O buffer for the standard error stream, so that the error handler can
  * produce a diagnostic even when memory allocation fails.
  */

static unsigned char __vstream_stdin_buf[ACL_VSTREAM_BUFSIZE];
static unsigned char __vstream_stdout_buf[ACL_VSTREAM_BUFSIZE];
static unsigned char __vstream_stderr_buf[ACL_VSTREAM_BUFSIZE];

static int __sys_getc(ACL_VSTREAM *stream);

ACL_VSTREAM acl_vstream_fstd[] = {              
	{       
#ifdef ACL_UNIX
		{ STDIN_FILENO },               /* h_file */
#elif defined(ACL_MS_WINDOWS)
		-1,                             /* h_file */
#endif
		0,                              /* is_nonblock */
		ACL_VSTREAM_TYPE_FILE,          /* type */
		0,                              /* offset */
		0,                              /* sys_offset */
		0,                              /* wbuf */
		0,                              /* wbuf_size */
		0,                              /* wbuf_dlen */
		__vstream_stdin_buf,            /* read_buf */
		sizeof(__vstream_stdin_buf),    /* read_buf_len */
		0,                              /* read_cnt */
		__vstream_stdin_buf,            /* read_ptr */
		0,                              /* sys_read_ready */
		0,                              /* total_read_cnt */
		0,                              /* total_write_cnt */
		NULL,                           /* ioctl_read_ctx */
		NULL,                           /* ioctl_write_ctx */
		NULL,                           /* fdp */
		ACL_VSTREAM_FLAG_READ,          /* flag */
		"\0",                           /* errbuf */
		0,                              /* errnum */
		0,                              /* rw_timeout */
		"\0",                           /* addr_local */
		"\0",                           /* addr_peer */
		NULL,                           /* sa_local */
		NULL,                           /* sa_peer */
		0,                              /* sa_local_size */
		0,                              /* sa_peer_size */
		0,                              /* sa_local_len */
		0,                              /* sa_peer_len */
		NULL,                           /* path */
		NULL,                           /* context */
		NULL,                           /* close_handle_lnk */
		__sys_getc,                     /* sys_getc */
		acl_socket_read,                /* read_fn */
		NULL,                           /* write_fn */
		NULL,                           /* writev_fn */
		acl_file_read,                  /* fread_fn */
		NULL,                           /* fwrite_fn */
		NULL,                           /* fwritev_fn */
		acl_socket_close,               /* close_fn */
		acl_file_close,                 /* fclose_fn */
		0,                              /* oflags */
		0,                              /* nrefer */
		0,                              /* pid */
#ifdef ACL_MS_WINDOWS
		NULL,                           /* hproc */
		ACL_SOCKET_INVALID,             /* iocp_sock */
#endif
	},

	{
#ifdef ACL_UNIX
		{ STDOUT_FILENO },              /* h_file */
#elif defined(ACL_MS_WINDOWS)
		-1,                             /* h_file */
#endif
		0,                              /* is_nonblock */
		ACL_VSTREAM_TYPE_FILE,          /* type */
		0,                              /* offset */
		0,                              /* sys_offset */
		0,                              /* wbuf */
		0,                              /* wbuf_size */
		0,                              /* wbuf_dlen */
		__vstream_stdout_buf,           /* read_buf */
		sizeof(__vstream_stdout_buf),   /* read_buf_len */
		0,                              /* read_cnt */
		__vstream_stdout_buf,           /* read_ptr */
		0,                              /* sys_read_ready */
		0,                              /* total_read_cnt */
		0,                              /* total_write_cnt */
		NULL,                           /* ioctl_read_ctx */
		NULL,                           /* ioctl_write_ctx */
		NULL,                           /* fdp */
		ACL_VSTREAM_FLAG_WRITE,         /* flag */
		"\0",                           /* errbuf */
		0,                              /* errnum */
		0,                              /* rw_timeout */
		"\0",                           /* addr_local */
		"\0",                           /* addr_peer */
		NULL,                           /* sa_local */
		NULL,                           /* sa_peer */
		0,                              /* sa_local_size */
		0,                              /* sa_peer_size */
		0,                              /* sa_local_len */
		0,                              /* sa_peer_len */
		NULL,                           /* path */
		NULL,                           /* context */
		NULL,                           /* close_handle_lnk */
		__sys_getc,                     /* sys_getc */
		NULL,                           /* read_fn */
		acl_socket_write,               /* write_fn */
		acl_socket_writev,              /* writev_fn */
		NULL,                           /* fread_fn */
		acl_file_write,                 /* fwrite_fn */
		acl_file_writev,                /* fwritev_fn */
		acl_socket_close,               /* close_fn */
		acl_file_close,                 /* fclose_fn */
		0,                              /* oflags */
		0,                              /* nrefer */
		0,                              /* pid */
#ifdef ACL_MS_WINDOWS
		NULL,                           /* hproc */
		ACL_SOCKET_INVALID,             /* iocp_sock */
#endif
	},
	{
#ifdef ACL_UNIX
		{ STDERR_FILENO },              /* h_file */
#elif defined(ACL_MS_WINDOWS)
		-1,                             /* h_file */
#endif
		0,                              /* is_nonblock */
		ACL_VSTREAM_TYPE_FILE,          /* type */
		0,                              /* offset */
		0,                              /* sys_offset */
		0,                              /* wbuf */
		0,                              /* wbuf_size */
		0,                              /* wbuf_dlen */
		__vstream_stderr_buf,           /* read_buf */
		sizeof(__vstream_stderr_buf),   /* read_buf_len */
		0,                              /* read_cnt */
		__vstream_stderr_buf,           /* read_ptr */
		0,                              /* sys_read_ready */
		0,                              /* total_read_cnt */
		0,                              /* total_write_cnt */
		NULL,                           /* ioctl_read_ctx */
		NULL,                           /* ioctl_write_ctx */
		NULL,                           /* fdp */
		ACL_VSTREAM_FLAG_WRITE,         /* flag */
		"\0",                           /* errbuf */
		0,                              /* errnum */
		0,                              /* rw_timeout */
		"\0",                           /* addr_local */
		"\0",                           /* addr_peer */
		NULL,                           /* sa_local */
		NULL,                           /* sa_peer */
		0,                              /* sa_local_size */
		0,                              /* sa_peer_size */
		0,                              /* sa_local_len */
		0,                              /* sa_peer_len */
		NULL,                           /* path */
		NULL,                           /* context */
		NULL,                           /* close_handle_lnk */
		__sys_getc,                     /* sys_getc */
		NULL,                           /* read_fn */
		acl_socket_write,               /* write_fn */
		acl_socket_writev,              /* writev_fn */
		NULL,                           /* fread_fn */
		acl_file_write,                 /* fwrite_fn */
		acl_file_writev,                /* fwritev_fn */
		acl_socket_close,               /* close_fn */
		acl_file_close,                 /* fclose_fn */
		0,                              /* oflags */
		0,                              /* nrefer */
		0,                              /* pid */
#ifdef ACL_MS_WINDOWS
		NULL,                           /* hproc */
		ACL_SOCKET_INVALID,             /* iocp_sock */
#endif
	},
};

void acl_vstream_init()
{
	static int __called = 0;

	if (__called)
		return;
	__called = 1;

#ifdef ACL_MS_WINDOWS
# if 0
	ACL_VSTREAM_IN->fd.h_file = (HANDLE) _get_osfhandle(_fileno(stdin));
	ACL_VSTREAM_OUT->fd.h_file = (HANDLE) _get_osfhandle(_fileno(stdout));
	ACL_VSTREAM_ERR->fd.h_file = (HANDLE) _get_osfhandle(_fileno(stderr));
# else
	ACL_VSTREAM_IN->fd.h_file = GetStdHandle(STD_INPUT_HANDLE);
	ACL_VSTREAM_OUT->fd.h_file = GetStdHandle(STD_OUTPUT_HANDLE);
	ACL_VSTREAM_ERR->fd.h_file = GetStdHandle(STD_ERROR_HANDLE);
# endif
#endif
}

static int __vstream_sys_read(ACL_VSTREAM *stream)
{
	/* 清除可读标志位 */
	stream->sys_read_ready = 0;

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID)
			return -1;
	} else if (ACL_VSTREAM_SOCK(stream) == ACL_SOCKET_INVALID)
		return -1;

AGAIN:
	if (stream->rw_timeout > 0
	    && acl_read_wait(ACL_VSTREAM_SOCK(stream), stream->rw_timeout) < 0) {

		stream->errnum = acl_last_error();

		if (stream->errnum != ACL_ETIMEDOUT) {
			(void) acl_strerror(stream->errnum, stream->errbuf,
				    sizeof(stream->errbuf));
			stream->flag |= ACL_VSTREAM_FLAG_ERR;
		} else {
			stream->flag |= ACL_VSTREAM_FLAG_TIMEOUT;
			ACL_SAFE_STRNCPY(stream->errbuf, "read timeout",
				sizeof(stream->errbuf));
		}

		return -1;
	}

	acl_set_error(0);

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		stream->read_cnt = stream->fread_fn(ACL_VSTREAM_FILE(stream),
			stream->read_buf, (size_t) stream->read_buf_len,
			stream->rw_timeout, stream, stream->context);
		if (stream->read_cnt > 0)
			stream->sys_offset += stream->read_cnt;
	} else
		stream->read_cnt = stream->read_fn(ACL_VSTREAM_SOCK(stream),
			stream->read_buf, (size_t) stream->read_buf_len,
			stream->rw_timeout, stream, stream->context);

	if (stream->read_cnt < 0) {
		stream->errnum = acl_last_error();
		if (stream->errnum == ACL_EINTR) {
			goto AGAIN;
		} else if (stream->errnum == ACL_ETIMEDOUT) {
			stream->flag |= ACL_VSTREAM_FLAG_TIMEOUT;
			ACL_SAFE_STRNCPY(stream->errbuf, "read timeout",
				sizeof(stream->errbuf));
		} else if (stream->errnum != ACL_EWOULDBLOCK
			&& stream->errnum != ACL_EAGAIN)
		{
			stream->flag |= ACL_VSTREAM_FLAG_ERR;
			acl_strerror(stream->errnum, stream->errbuf,
				sizeof(stream->errbuf));
		}
		/* XXX: should do something where, 2009.12.25 -- zsx */

		stream->read_cnt = 0;	/* xxx: why? */
		return -1;
	} else if (stream->read_cnt == 0) { /* closed by peer */
		stream->flag = ACL_VSTREAM_FLAG_EOF;
		stream->errnum = 0;
		snprintf(stream->errbuf, sizeof(stream->errbuf),
			"closed by peer(%s)", acl_last_serror());

		return 0;
	}

	stream->read_ptr = stream->read_buf;
	stream->flag &= ~ACL_VSTREAM_FLAG_BAD;
	stream->errnum = 0;
	stream->errbuf[0] = 0;
	stream->total_read_cnt += stream->read_cnt;

	return (int) stream->read_cnt;
}

static int __sys_getc(ACL_VSTREAM *stream)
{
	stream->read_cnt = __vstream_sys_read(stream);
	if (stream->read_cnt <= 0)
		return ACL_VSTREAM_EOF;
	else
		return ACL_VSTREAM_GETC(stream);
}

int acl_vstream_getc(ACL_VSTREAM *stream)
{
	if (stream == NULL)
		return ACL_VSTREAM_EOF;
	if (stream->read_cnt <= 0) {
		if (__vstream_sys_read(stream) <= 0)
			return ACL_VSTREAM_EOF;
	}

	stream->read_cnt--;
	stream->offset++;
	return *stream->read_ptr++;
}

int acl_vstream_nonb_readn(ACL_VSTREAM *stream, char *buf, int size)
{
	const char *myname = "acl_vstream_nonb_readn";
	int   n, nread, read_cnt;
	unsigned char *ptr;
	int   rw_timeout;
#ifdef	ACL_UNIX
	int   flags;
#endif

	if (stream == NULL || buf == NULL || size <= 0)
		return (ACL_VSTREAM_EOF);

	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(%d) < 0, fd(%d)",
			myname, __FILE__, __LINE__,
			(int) stream->read_cnt, ACL_VSTREAM_SOCK(stream));

	ptr = (unsigned char *) buf;
	nread = 0;

	if (stream->read_cnt > 0) {
		n = size > (int) stream->read_cnt ? (int) stream->read_cnt : size;
		read_cnt = acl_vstream_bfcp_some(stream, ptr, n);
		if (read_cnt <= 0) {
			acl_msg_error("%s, %s(%d): internal error, read_cnt = %d",
				myname, __FILE__, __LINE__, read_cnt);
			return ACL_VSTREAM_EOF;
		}
		size  -= read_cnt;
		ptr   += read_cnt;
		nread += read_cnt;
		if (size == 0)
			return (read_cnt);
		else if (size < 0) {
			acl_msg_error("%s, %s(%d): internal error, size = %d",
				myname, __FILE__, __LINE__, size);
			return ACL_VSTREAM_EOF;
		}
	}

#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(stream), F_GETFL, 0);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			myname, __FILE__, __LINE__, acl_last_serror(),
			ACL_VSTREAM_SOCK(stream));
		return ACL_VSTREAM_EOF;
	}
	acl_non_blocking(ACL_VSTREAM_SOCK(stream), 1);
#elif defined(ACL_MS_WINDOWS)
	if (stream->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(stream), 1);
#endif

	/* 先保留读写超时时间值，并将该流的超时值置为0，以免
	 * 启动读超时过程(select)。
	 */
	rw_timeout = stream->rw_timeout;
	stream->rw_timeout = 0;
	stream->errnum = 0;

	read_cnt = __vstream_sys_read(stream);

	stream->rw_timeout = rw_timeout;

	/* 恢复该套接字的原有标记位 */
#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(stream), F_SETFL, flags);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			myname, __FILE__, __LINE__,
			acl_last_serror(), ACL_VSTREAM_SOCK(stream));
		return ACL_VSTREAM_EOF;
	}

#elif	defined(ACL_MS_WINDOWS)
	if (stream->is_nonblock == 0 && stream->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(stream), ACL_BLOCKING);
#endif

	if (read_cnt < 0) {
#ifdef	ACL_MS_WINDOWS
		if (stream->errnum == ACL_EWOULDBLOCK)
#elif	defined(ACL_UNIX)
		if (stream->errnum == ACL_EWOULDBLOCK
			|| stream->errnum == ACL_EAGAIN)
#endif
		{
			return nread;
		}
		else if (nread == 0)
			return (ACL_VSTREAM_EOF);
		else
			return nread;
	} else if (read_cnt == 0) {
		if (nread == 0)
			return ACL_VSTREAM_EOF;
		else
			return nread;
	}

	if (stream->read_cnt > 0) {
		n = size > (int) stream->read_cnt ? (int) stream->read_cnt : size;
		read_cnt = acl_vstream_bfcp_some(stream, ptr, n);
		if (read_cnt <= 0) {
			acl_msg_error("%s, %s(%d): internal error, read_cnt = %d",
				myname, __FILE__, __LINE__, read_cnt);
			return ACL_VSTREAM_EOF;
		}

		nread += read_cnt;
	}

	stream->rw_timeout = rw_timeout;

	return nread;
}

int acl_vstream_probe_status(ACL_VSTREAM *stream)
{
#ifdef	ACL_UNIX
	const char *myname = "acl_vstream_probe_status";
	int   flags;
#endif
	int   ch;
	int   rw_timeout;

	if (stream == NULL)
		return -1;

#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(stream), F_GETFL, 0);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			myname, __FILE__, __LINE__,
			acl_last_serror(), ACL_VSTREAM_SOCK(stream));
		return -1;
	}
	acl_non_blocking(ACL_VSTREAM_SOCK(stream), 1);
#elif defined(ACL_MS_WINDOWS)
	if (stream->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(stream), 1);
#endif

	rw_timeout = stream->rw_timeout;

	/* 先保留读写超时时间值，并将该流的超时值置为0，以免
	 * 启动读超时过程(select)。
	 */
	stream->rw_timeout = 0;
	stream->errnum = 0;

	ch = acl_vstream_getc(stream);

	stream->rw_timeout = rw_timeout;

	/* 恢复该套接字的原有标记位 */
#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(stream), F_SETFL, flags);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			myname, __FILE__, __LINE__,
			acl_last_serror(), ACL_VSTREAM_SOCK(stream));
		return -1;
	}

#elif	defined(ACL_MS_WINDOWS)
	if (stream->is_nonblock == 0 && stream->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(stream), ACL_BLOCKING);
#endif

	if (ch == ACL_VSTREAM_EOF) {
#ifdef	ACL_MS_WINDOWS
		if (stream->errnum == ACL_EWOULDBLOCK)
#elif	defined(ACL_UNIX)
		if (stream->errnum == ACL_EWOULDBLOCK
			|| stream->errnum == ACL_EAGAIN)
#endif
		{
			return 0;
		}
		else
			return -1;
	} else {
		/* 将读到的数据再放回原处:) */
		stream->read_cnt++;
		stream->read_ptr--;
		stream->offset--;
		if (stream->read_ptr < stream->read_buf)
			return -1;
		return 0;
	}
}

int acl_vstream_ungetc(ACL_VSTREAM *stream, int ch)
{
	unsigned char c;

	c = (unsigned char) ch;
	(void) acl_vstream_unread(stream, &c, 1);
	return ch;
}

static void *vstream_memmove(ACL_VSTREAM *stream, size_t n)
{
#if 1
	char *src, *dst, *dst_saved;

	if (stream->read_cnt == 0)
		return stream->read_buf;

	src = (char*) stream->read_ptr + stream->read_cnt - 1;
	dst_saved = dst = (char*) stream->read_ptr + n + stream->read_cnt - 1;

	/* 为了防止内存数据覆盖问题, 采用数据从尾部拷贝方式 */
	while (src >= (char*) stream->read_ptr)
		*dst-- = *src--;
	return dst_saved;
#else
	return memmove((char*) stream->read_ptr + n,
			stream->read_ptr, stream->read_cnt);
#endif
}

int acl_vstream_unread(ACL_VSTREAM *stream, const void *ptr, size_t length)
{
	size_t capacity = stream->read_ptr - stream->read_buf;
	ssize_t k = capacity - length;

	/* 如果读缓冲中前部分空间不足, 则需要调整数据位置或扩充读缓冲区空间 */

	if (k < 0) {
		void *pbuf;
		size_t n, min_delta = 4096;

		n = (size_t) -k;

		/* 如果读缓冲区后部分空间够用, 则只需后移缓冲区中的数据 */

		if (stream->read_buf_len - stream->read_cnt > (acl_off_t) length) {
			if (stream->read_cnt > 0)
				vstream_memmove(stream, n);

			memcpy(stream->read_buf, ptr, length);
			stream->read_ptr = stream->read_buf;
			stream->read_cnt += length;
			stream->offset = 0;
			return (int) length;
		}

		/* 说明整个缓冲区的空间都不够用, 所以需要扩充缓冲区空间 */

		n = min_delta * ((n + min_delta - 1) / min_delta);
		acl_assert(n > 0);
		stream->read_buf_len += n;
		pbuf = acl_mymalloc((size_t) stream->read_buf_len);

		memcpy(pbuf, ptr, length);
		if (stream->read_cnt > 0)
			memcpy((char*) pbuf + length, stream->read_ptr,
				(size_t) stream->read_cnt);
		acl_myfree(stream->read_buf);

		stream->read_buf = pbuf;
		stream->read_ptr = stream->read_buf;
		stream->read_cnt += length;
		stream->offset = 0;
		return ((int) length);
	}

	stream->read_ptr -= length;
	memcpy(stream->read_ptr, ptr, length);
	stream->read_cnt += length;
	stream->offset -= length;
	return (int) length;
}

int acl_vstream_bfcp_some(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	const char *myname = "acl_vstream_bfcp_some";
	int   n;

	/* input params error */
	if (stream == NULL || vptr == NULL || maxlen <= 0)
		acl_msg_fatal("%s, %s(%d): input error",
			myname, __FILE__, __LINE__);

	/* internal fatal error */
	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	/* there is no any data in buf */
	if (stream->read_cnt == 0) {
		stream->read_ptr = stream->read_buf;
		return 0;
	}

	if (stream->read_ptr >= stream->read_buf + (int) stream->read_buf_len) {
		stream->read_cnt = 0;
		stream->read_ptr = stream->read_buf;
		return 0;
	}

	n = (int) stream->read_cnt > (int) maxlen
		? (int) maxlen : (int) stream->read_cnt;

	memcpy(vptr, stream->read_ptr, n);

	stream->read_cnt -= n;
	stream->read_ptr += n;
	stream->offset += n;

	return n;
}

int acl_vstream_gets(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n, ch;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || maxlen <= 0)
		return ACL_VSTREAM_EOF;

	ptr = (unsigned char *) vptr;
	for (n = 1; n < (int) maxlen; n++) {
		/* left one byte for '\0' */

#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(stream);
#else
		ch = acl_vstream_getc(stream);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			stream->flag &= ~ACL_VSTREAM_FLAG_TAGYES;
			stream->flag |= ACL_VSTREAM_FLAG_TAGNO;

			/* EOF, nodata read */
			if (n == 1)
				return ACL_VSTREAM_EOF;
			/* EOF, some data was read */
			break;
		} else {
			*ptr++ = ch;
			if (ch == '\n'){
				/* newline is stored, like fgets() */

				stream->flag |= ACL_VSTREAM_FLAG_TAGYES;
				stream->flag &= ~ACL_VSTREAM_FLAG_TAGNO;
				break;
			}
		}
	}

	/* null terminate like fgets() */
	*ptr = 0;
	return n;
}

int acl_vstream_readtags(ACL_VSTREAM *stream, void *vptr, size_t maxlen,
	const char *tag, size_t taglen)
{
	int   n, ch, flag_match = 0;
	unsigned char *ptr;
	const unsigned char *ptr_haystack;
	const unsigned char *ptr_needle, *ptr_needle_end;

	if (stream == NULL || vptr == NULL || maxlen <= 0
	    || tag == NULL || taglen <= 0)
		return ACL_VSTREAM_EOF;

	ptr_needle_end = (const unsigned char *) tag;

	while(1) {
		taglen--;
		if (taglen == 0)
			break;
		ptr_needle_end++;
	}
	ptr = (unsigned char *) vptr;

	for (n = 1; n < (int) maxlen; n++) {
		/* left one byte for '\0' */

#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(stream);
#else
		ch = acl_vstream_getc(stream);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 1)
				return ACL_VSTREAM_EOF;  /* EOF, nodata read */
			else {
				stream->flag &= ~ACL_VSTREAM_FLAG_TAGYES;
				stream->flag |= ACL_VSTREAM_FLAG_TAGNO;
				break;  /* EOF, some data was read */
			}
		} else {
			*ptr = ch;
			if (ch == *ptr_needle_end) {
				ptr_haystack = ptr - 1;
				ptr_needle = ptr_needle_end - 1;
				flag_match = 0;
				while(1) {
					/* 已经成功比较完毕(匹配) */
					if (ptr_needle < (const unsigned char *) tag) {
						flag_match = 1;
						break;
					}

					/* 原字符串用完而匹配串还没有比较完(不匹配) */
					if (ptr_haystack < (unsigned char *) vptr)
						break;
					/* 不相等(不匹配) */
					if (*ptr_haystack != *ptr_needle)
						break;
					ptr_haystack--;
					ptr_needle--;
				}
			}
			ptr++;
			if (flag_match) {
				stream->flag |= ACL_VSTREAM_FLAG_TAGYES;
				stream->flag &= ~ACL_VSTREAM_FLAG_TAGNO;
				break;
			}
		} 
	}
	*ptr = 0;	/* null terminate like fgets() */
	return n;
}

int acl_vstream_gets_nonl(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n, ch;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || maxlen <= 0)
		return ACL_VSTREAM_EOF;

	ptr = (unsigned char *) vptr;
	for (n = 1; n < (int) maxlen; n++) {
#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(stream);
#else
		ch = acl_vstream_getc(stream);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			stream->flag &= ~ACL_VSTREAM_FLAG_TAGYES;
			stream->flag |= ACL_VSTREAM_FLAG_TAGNO;
			if (n == 1)
				return ACL_VSTREAM_EOF;  /* EOF, nodata read */
			else
				break;  /* EOF, some data was read */
		} else {
			*ptr++ = ch;
			if (ch == '\n') {
				stream->flag |= ACL_VSTREAM_FLAG_TAGYES;
				stream->flag &= ~ACL_VSTREAM_FLAG_TAGNO;
				break;  /* newline is stored, like fgets() */
			}
		}
	}

	*ptr = 0;  /* null terminate like fgets() */
	ptr--;
	while (ptr >= (unsigned char *) vptr) {
		if (*ptr != '\r' && *ptr != '\n')
			break;

		*ptr-- = 0;
		n--;
	}
	return n;
}

int acl_vstream_readn(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n, ch;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || (int) maxlen <= 0)
		return ACL_VSTREAM_EOF;

	ptr = (unsigned char *) vptr;
	for (n = 0; n < (int) maxlen; n++) {
#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(stream);
#else
		ch = acl_vstream_getc(stream);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 0)
				return ACL_VSTREAM_EOF;  /* EOF, nodata read */
			else
				break;  /* EOF, some data was read */
		} else
			*ptr++ = ch;
	}

	if (n != (int) maxlen) {
		snprintf(stream->errbuf, sizeof(stream->errbuf),
			"nread=%d, nneed=%d, errmsg=not read the needed data",
			n, (int) maxlen);
		stream->flag |= ACL_VSTREAM_FLAG_RDSHORT;

		return ACL_VSTREAM_EOF;
	}

	return n;
}

int acl_vstream_read(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	const char *myname = "acl_vstream_read";
	int   read_cnt;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || (int) maxlen <= 0)
		return ACL_VSTREAM_EOF;

	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	ptr = (unsigned char *) vptr;
	if (stream->read_cnt > 0) {
		read_cnt = acl_vstream_bfcp_some(stream, ptr, maxlen);
		return read_cnt;
	}

	/* stream->read_cnt == 0 */

	/* there is no data in buf, so need to read data from system */
	read_cnt = __vstream_sys_read(stream);
	if (read_cnt < 0)
		return ACL_VSTREAM_EOF;
	else if (read_cnt == 0)
		return ACL_VSTREAM_EOF;

	read_cnt = acl_vstream_bfcp_some(stream, ptr, maxlen);
	return read_cnt;
}

static void __bfgets_crlf_peek(ACL_VSTREAM *stream, ACL_VSTRING *strbuf, int *ready)
{
	if (stream->read_cnt <= 0)   /* XXX: sanity check */
		return;
	while (stream->read_cnt > 0) {
		ACL_VSTRING_ADDCH(strbuf, *(stream->read_ptr));
		stream->read_cnt--;
		stream->offset++;
		if (*(stream->read_ptr) == '\n') {
			*ready = 1;
			stream->read_ptr++;
			break;
		}
		stream->read_ptr++;
	}

	ACL_VSTRING_TERMINATE(strbuf);  /* set '\0' teminated */
}

int acl_vstream_gets_peek(ACL_VSTREAM *stream, ACL_VSTRING *strbuf, int *ready)
{
	const char *myname = "acl_vstream_gets_peek";
	int   n;

	if (stream == NULL || strbuf == NULL || ready == NULL)
		acl_msg_fatal("%s, %s(%d): invalid input",
			myname, __FILE__, __LINE__);

	*ready = 0;
	n = ACL_VSTRING_LEN(strbuf);

	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	if (stream->read_cnt > 0) {
		__bfgets_crlf_peek(stream, strbuf, ready);
		if (*ready)
			return ACL_VSTRING_LEN(strbuf) - n;
	}

	/* XXX: 调用者通过检查 *ready 值来判断是否读够数据, 系统IO读操作出错
	 * 或关闭时返回结束标记; 如果返回 ACL_VSTRING_EOF 则调用者应该通过
	 * 检查缓冲区长度来处理未被处理的数据
	 */

	if (stream->sys_read_ready) {
		if (__vstream_sys_read(stream) <= 0) {
			n = ACL_VSTRING_LEN(strbuf) - n;
			return n > 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (stream->read_cnt > 0)
		__bfgets_crlf_peek(stream, strbuf, ready);
	return ACL_VSTRING_LEN(strbuf) - n;
}

static void __bfgets_no_crlf_peek(ACL_VSTREAM *stream,
	ACL_VSTRING *strbuf, int *ready)
{
	int   n, ch;

	if (stream->read_cnt <= 0)   /* XXX: sanity check */
		return;

	while (stream->read_cnt > 0) {
		ACL_VSTRING_ADDCH(strbuf, *(stream->read_ptr));
		stream->read_cnt--;
		stream->offset++;
		if (*(stream->read_ptr) == '\n') {	/* ok, get '\n' */
			*ready = 1;			/* set to be ready */
			stream->read_ptr++;
			break;
		}
		stream->read_ptr++;
	}

	if (*ready == 1) {
		n = ACL_VSTRING_LEN(strbuf) - 1;
		while (n >= 0) {
			ch = acl_vstring_charat(strbuf, n);
			if (ch == '\r' || ch == '\n')
				n--;
			else
				break;
		}

		/* reset the offset position */
		acl_vstring_truncate(strbuf, n + 1);
	}

	ACL_VSTRING_TERMINATE(strbuf);		/* set '\0' teminated */
}

int acl_vstream_gets_nonl_peek(ACL_VSTREAM *stream,
	ACL_VSTRING *strbuf, int *ready)
{
	const char *myname = "acl_vstream_gets_nonl_peek";
	int   n;

	if (stream == NULL || strbuf == NULL || ready == NULL)
		acl_msg_fatal("%s, %s(%d): invalid input",
			myname, __FILE__, __LINE__);

	*ready = 0;
	n = ACL_VSTRING_LEN(strbuf);

	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	if (stream->read_cnt > 0) {
		__bfgets_no_crlf_peek(stream, strbuf, ready);
		if (*ready)
			return ACL_VSTRING_LEN(strbuf) - n;
	}

	/* XXX: 调用者通过检查 *ready 值来判断是否读够数据, 系统IO读操作出错
	 * 或关闭时返回结束标记; 如果返回 ACL_VSTRING_EOF 则调用者应该通过
	 * 检查缓冲区长度来处理未被处理的数据
	 */

	if (stream->sys_read_ready) {
		if (__vstream_sys_read(stream) <= 0) {
			n = ACL_VSTRING_LEN(strbuf) - n;
			return n > 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (stream->read_cnt > 0)
		__bfgets_no_crlf_peek(stream, strbuf, ready);
	return ACL_VSTRING_LEN(strbuf) - n;
}

static int __bfread_cnt_peek(ACL_VSTREAM *stream, ACL_VSTRING *strbuf,
	int cnt, int *ready)
{
	int   n;

	/* XXX: sanity check */
	if (stream->read_cnt <= 0)
		return 0;
	n = (int) (stream->read_cnt > cnt ? cnt : stream->read_cnt);
	acl_vstring_memcat(strbuf, (char*) stream->read_ptr, n);
	stream->read_cnt -= n;
	stream->offset += n;
	stream->read_ptr += n;
	cnt -= n;
	if (cnt == 0)
		*ready = 1;

	ACL_VSTRING_TERMINATE(strbuf);  /* set '\0' teminated */
	return n;
}

int acl_vstream_readn_peek(ACL_VSTREAM *stream, ACL_VSTRING *strbuf,
	int cnt, int *ready)
{
	const char *myname = "acl_vstream_readn_peek";
	int   cnt_saved = cnt;

	if (stream == NULL || strbuf == NULL || cnt <= 0 || ready == NULL)
		acl_msg_fatal("%s, %s(%d): invalid input, stream: %s, "
			"strbuf: %s, cnt: %d, ready: %s", myname, __FILE__,
			__LINE__, stream ? "not null" : "null",
			strbuf ? "not null" : "null", cnt,
			ready ? "not null" : "null");

	*ready = 0;
	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	if (stream->read_cnt > 0) {
		cnt -= __bfread_cnt_peek(stream, strbuf, cnt, ready);
		if (*ready)
			return cnt_saved - cnt;
	}

	/* XXX: 调用者通过检查 *ready 值来判断是否读够数据, 系统IO读操作出错
	 * 或关闭时返回结束标记, 如果返回 ACL_VSTRING_EOF 则调用者应该通过
	 * 检查缓冲区长度来处理未被处理的数据
	 */

	if (stream->sys_read_ready) {
		if (__vstream_sys_read(stream) <= 0) {
			int   n = cnt_saved - cnt;
			return n > 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (stream->read_cnt > 0)
		cnt -= __bfread_cnt_peek(stream, strbuf, cnt, ready);
	return cnt_saved - cnt;
}

static void __bfread_peek(ACL_VSTREAM *stream, ACL_VSTRING *strbuf)
{
	/* XXX: sanity check */
	if (stream->read_cnt <= 0)
		return;
	acl_vstring_memcat(strbuf, (char*) stream->read_ptr,
		(size_t) stream->read_cnt);
	stream->offset += stream->read_cnt;
	stream->read_ptr += stream->read_cnt;
	stream->read_cnt = 0;

	/* set '\0' teminated */
	ACL_VSTRING_TERMINATE(strbuf);
}

int acl_vstream_read_peek(ACL_VSTREAM *stream, ACL_VSTRING *strbuf)
{
	const char *myname = "acl_vstream_read_peek";
	int   n;

	if (stream == NULL || strbuf == NULL)
		acl_msg_fatal("%s, %s(%d): invalid input",
			myname, __FILE__, __LINE__);

	n = ACL_VSTRING_LEN(strbuf);

	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	if (stream->read_cnt > 0)
		__bfread_peek(stream, strbuf);

	/* 系统IO读操作出错或关闭时返回结束标记, 如果返回 ACL_VSTRING_EOF
	 * 则调用者应该通过检查缓冲区长度来处理未被处理的数据
	 */

	if (stream->sys_read_ready) {
		if (__vstream_sys_read(stream) <= 0) {
			n = ACL_VSTRING_LEN(strbuf) - n;
			return n > 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (stream->read_cnt > 0)
		__bfread_peek(stream, strbuf);
	return ACL_VSTRING_LEN(strbuf) - n;
}

int acl_vstream_can_read(ACL_VSTREAM *stream)
{
	const char *myname = "acl_vstream_can_read";

	if (stream->read_cnt < 0)
		acl_msg_fatal("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) stream->read_cnt);

	if (stream->flag & (ACL_VSTREAM_FLAG_ERR | ACL_VSTREAM_FLAG_EOF))
		return ACL_VSTREAM_EOF;
	else if (stream->read_cnt > 0)
		return 1;
	else if (stream->sys_read_ready == 0)
		return 0;
	else if ((stream->flag & ACL_VSTREAM_FLAG_PREREAD) != 0) {
		if (__vstream_sys_read(stream) <= 0)
			return ACL_VSTREAM_EOF;
		else
			return 1;
	}
	else
		return 1;
}

static int __vstream_sys_write(ACL_VSTREAM *stream, const void *vptr, int dlen)
{
	const char *myname = "__vstream_sys_write";
	int   n, neintr = 0;

	if (stream == NULL || vptr == NULL || dlen <= 0) {
		if (stream == NULL)
			acl_msg_error("%s, %s(%d): stream null",
				myname, __FILE__, __LINE__);
		if (vptr == NULL)
			acl_msg_error("%s, %s(%d): vptr null",
				myname, __FILE__, __LINE__);
		if (dlen <= 0)
			acl_msg_error("%s, %s(%d): dlen(%d) <= 0",
				myname, __FILE__, __LINE__, dlen);
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID) {
			acl_msg_error("%s, %s(%d): h_file invalid",
				myname, __FILE__, __LINE__);
			return ACL_VSTREAM_EOF;
		}
	} else if (ACL_VSTREAM_SOCK(stream) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s, %s(%d): sockfd invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

TAG_AGAIN:

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if ((stream->oflags & O_APPEND)) {
#ifdef ACL_MS_WINDOWS
			stream->sys_offset = acl_lseek(
				ACL_VSTREAM_FILE(stream), 0, SEEK_END);
			if (stream->sys_offset < 0)
				return ACL_VSTREAM_EOF;
#endif
		} else if ((stream->flag & ACL_VSTREAM_FLAG_CACHE_SEEK)
			&& stream->offset != stream->sys_offset)
		{

			stream->sys_offset = acl_lseek(ACL_VSTREAM_FILE(stream),
				stream->offset, SEEK_SET);
			if (stream->sys_offset == -1) {
				acl_msg_error("%s, %s(%d): lseek error(%s), "
					"offset(" ACL_FMT_I64D "), sys_offset("
					ACL_FMT_I64D ")", myname, __FILE__,
					__LINE__, acl_last_serror(),
					stream->offset, stream->sys_offset);
				return ACL_VSTREAM_EOF;
			}
			stream->offset = stream->sys_offset;
		}

		n = stream->fwrite_fn(ACL_VSTREAM_FILE(stream), vptr, dlen,
			stream->rw_timeout, stream, stream->context);
		if (n > 0) {
			stream->sys_offset += n;
			stream->offset = stream->sys_offset;

			/* 防止缓冲区内的数据与实际不一致, 仅对文件IO有效 */
			stream->read_cnt = 0;
		}
	} else
		n = stream->write_fn(ACL_VSTREAM_SOCK(stream), vptr, dlen,
			stream->rw_timeout, stream, stream->context);
	if (n < 0) {
		if (acl_last_error() == ACL_EINTR) {
			if (++neintr >= 5)
				return ACL_VSTREAM_EOF;

			goto TAG_AGAIN;
		}

		if (acl_last_error() == ACL_EAGAIN
			|| acl_last_error() == ACL_EWOULDBLOCK)
		{
			acl_set_error(ACL_EAGAIN);
		}

		return ACL_VSTREAM_EOF;
	}

	stream->total_write_cnt += n;
	return n;
}

static int __vstream_sys_writev(ACL_VSTREAM *stream,
	const struct iovec *vec, int count)
{
	const char *myname = "__vstream_sys_writev";
	int   n, neintr = 0;

	if (stream == NULL || vec == NULL || count <= 0) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID) {
			acl_msg_error("%s, %s(%d): h_file invalid",
				myname, __FILE__, __LINE__);
			return ACL_VSTREAM_EOF;
		}
	} else if (ACL_VSTREAM_SOCK(stream) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s, %s(%d): sockfd invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

TAG_AGAIN:

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if ((stream->oflags & O_APPEND)) {
#ifdef ACL_MS_WINDOWS
			stream->sys_offset = acl_lseek(
				ACL_VSTREAM_FILE(stream), 0, SEEK_END);
			if (stream->sys_offset < 0)
				return ACL_VSTREAM_EOF;
#endif
		} else if ((stream->flag & ACL_VSTREAM_FLAG_CACHE_SEEK)
			&& stream->offset != stream->sys_offset)
		{
			stream->sys_offset = acl_lseek(ACL_VSTREAM_FILE(stream),
					stream->offset, SEEK_SET);
			if (stream->sys_offset == -1) {
				acl_msg_error("%s, %s(%d): lseek error(%s), "
					"offset(" ACL_FMT_I64D "), sys_offset("
					ACL_FMT_I64D ")", myname, __FILE__,
					__LINE__, acl_last_serror(),
					stream->offset, stream->sys_offset);
				return ACL_VSTREAM_EOF;
			}
		}

		n = stream->fwritev_fn(ACL_VSTREAM_FILE(stream), vec, count,
			stream->rw_timeout, stream, stream->context);
		if (n > 0) {
			stream->sys_offset += n;
			stream->offset = stream->sys_offset;

			/* 防止缓冲区内的数据与实际不一致, 仅对文件IO有效 */
			stream->read_cnt = 0;
		}
	} else
		n = stream->writev_fn(ACL_VSTREAM_SOCK(stream), vec, count,
			stream->rw_timeout, stream, stream->context);
	if (n < 0) {
		if (acl_last_error() == ACL_EINTR) {
			if (++neintr >= 5)
				return ACL_VSTREAM_EOF;

			goto TAG_AGAIN;
		}

		if (acl_last_error() == ACL_EAGAIN
			|| acl_last_error() == ACL_EWOULDBLOCK)
		{
			acl_set_error(ACL_EAGAIN);
		}

		return ACL_VSTREAM_EOF;
	}

	stream->total_write_cnt += n;
	return n;
}

int acl_vstream_write(ACL_VSTREAM *stream, const void *vptr, int dlen)
{
	if (stream->wbuf_dlen > 0) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	return __vstream_sys_write(stream, vptr, dlen);
}

int acl_vstream_writev(ACL_VSTREAM *stream, const struct iovec *vec, int count)
{
	if (stream->wbuf_dlen > 0) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	return __vstream_sys_writev(stream, vec, count);
}

int acl_vstream_writevn(ACL_VSTREAM *stream, const struct iovec *vec, int count)
{
	const char *myname = "acl_vstream_writevn";
	int   n, i, dlen, k;
	struct iovec *vect;

	if (count <= 0 || vec == NULL)
		acl_msg_fatal("%s, %s(%d): invalid input",
			myname, __FILE__, __LINE__);

	if (stream->wbuf_dlen > 0) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	vect = (struct iovec*) acl_mycalloc(count, sizeof(struct iovec));
	for (i = 0; i < count; i++) {
		vect[i].iov_base = vec[i].iov_base;
		vect[i].iov_len = vec[i].iov_len;
	}

	dlen = 0;

	while (1) {
		n = __vstream_sys_writev(stream, vect, count);
		if (n == ACL_VSTREAM_EOF) {
			acl_myfree(vect);
			return ACL_VSTREAM_EOF;
		}
		dlen += n;

		k = 0;
		for (i = 0; i < count; i++) {
			if (n >= (int) vect[i].iov_len) {
				/* written */
				n -= vect[i].iov_len;
				k++;
			} else {
				/* partially written */
				vect[i].iov_base = (void *) ((unsigned char*)
					vect[i].iov_base + n);
				vect[i].iov_len -= n;
				break;
			}
		}

		if (i >= count) {
			acl_myfree(vect);
			return dlen;
		}
		count -= k;
		vec += k;
	}
}

int acl_vstream_vfprintf(ACL_VSTREAM *stream, const char *fmt, va_list ap)
{
	const char *myname = "acl_vstream_vfprintf";
	ACL_VSTRING *strbuf;
	int   n;

	if (stream == NULL || fmt == NULL || *fmt == 0) {
		acl_msg_error("%s, %s(%d): fmt invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	strbuf = acl_vstring_alloc(ACL_VSTREAM_BUFSIZE);
	if (strbuf == NULL) {
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
	}

	if (acl_vstring_vsprintf(strbuf, fmt, ap) == NULL)
		acl_msg_fatal("%s, %s(%d): vsprintf return null",
			myname, __FILE__, __LINE__);

	n = ACL_VSTRING_LEN(strbuf);
	if (n <= 0)
		acl_msg_fatal("%s, %s(%d): len(%d) <= 0",
			myname, __FILE__, __LINE__, n);

	n = acl_vstream_writen(stream, acl_vstring_str(strbuf), n);

	acl_vstring_free(strbuf);
	return n;
}

int acl_vstream_fprintf(ACL_VSTREAM *stream, const char *fmt, ...)
{
	const char *myname = "acl_vstream_fprintf";
	va_list ap;
	int   n;

	if (stream == NULL || fmt == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	va_start(ap, fmt);
	n = acl_vstream_vfprintf(stream, fmt, ap);
	va_end(ap);
	return n;
}

int acl_vstream_printf(const char *fmt, ...)
{
	const char *myname = "acl_vstream_printf";
	va_list ap;
	int   n;

	if (fmt == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (ACL_VSTREAM_OUT->fd.h_file == (ACL_FILE_HANDLE) -1)
		acl_vstream_init();
	if (ACL_VSTREAM_OUT->fd.h_file == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): ACL_VSTREAM_OUT can't be inited",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}
	va_start(ap, fmt);
	n = acl_vstream_vfprintf(ACL_VSTREAM_OUT, fmt, ap);
	va_end(ap);

	return n;
}

int acl_vstream_fputs(const char *s, ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_fputs";

	if (s == NULL || fp == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if ((*s) != 0 && acl_vstream_buffed_fwrite(fp, s, strlen(s))
		== ACL_VSTREAM_EOF)
	{
		return ACL_VSTREAM_EOF;
	}
	if (acl_vstream_buffed_fwrite(fp, "\r\n", 2) == ACL_VSTREAM_EOF)
		return ACL_VSTREAM_EOF;
	return acl_vstream_fflush(fp) == ACL_VSTREAM_EOF
		? ACL_VSTREAM_EOF : 0;
}

int acl_vstream_puts(const char *s)
{
	const char *myname = "acl_vstream_puts";

	if (ACL_VSTREAM_OUT->fd.h_file == (ACL_FILE_HANDLE) -1)
		acl_vstream_init();
	if (ACL_VSTREAM_OUT->fd.h_file == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): ACL_VSTREAM_OUT can't be inited",
			myname, __FILE__, __LINE__);
		return (ACL_VSTREAM_EOF);
	}

	return acl_vstream_fputs(s, ACL_VSTREAM_OUT);
}

static int __loop_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen)
{
	const unsigned char *ptr;
	int   n;

	ptr   = (const unsigned char *) vptr;
	while (dlen > 0) {
		n = __vstream_sys_write(stream, ptr, dlen);
		if (n <= 0) {
			if (acl_last_error() == ACL_EINTR
				|| acl_last_error() == ACL_EAGAIN)
			{
				continue;
			}
			return ACL_VSTREAM_EOF;
		}

		dlen  -= n;
		ptr   += n;
	}

	return ptr - (const unsigned char *) vptr;
}

int acl_vstream_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen)
{
	if (stream == NULL || vptr == NULL || dlen <= 0)
		return ACL_VSTREAM_EOF;

	if (stream->wbuf_dlen > 0) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	return __loop_writen(stream, vptr, dlen);
}

int acl_vstream_buffed_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen)
{
	if (stream == NULL || vptr == NULL || dlen == 0)
		return ACL_VSTREAM_EOF;

	if (stream->wbuf == NULL) {
		stream->wbuf_size = 8192;
		stream->wbuf = acl_mymalloc(stream->wbuf_size);
	}

	if (dlen >= (size_t) stream->wbuf_size) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
		else if (__loop_writen(stream, vptr, dlen) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
		else
			return dlen;
	} else if (dlen + (size_t) stream->wbuf_dlen >=
		(size_t) stream->wbuf_size)
	{
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}

	memcpy(stream->wbuf + (size_t) stream->wbuf_dlen, vptr, dlen);
	stream->wbuf_dlen += (int) dlen;
	return dlen;
}

int acl_vstream_buffed_vfprintf(ACL_VSTREAM *stream, const char *fmt, va_list ap)
{
	const char *myname = "acl_vstream_buffed_vfprintf";
	ACL_VSTRING *strbuf;
	int   n;

	if (stream == NULL || fmt == NULL || *fmt == 0) {
		acl_msg_error("%s, %s(%d): fmt invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	strbuf = acl_vstring_alloc(ACL_VSTREAM_BUFSIZE);
	if (strbuf == NULL) {
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
	}

	if (acl_vstring_vsprintf(strbuf, fmt, ap) == NULL)
		acl_msg_fatal("%s, %s(%d): vsprintf return null",
			myname, __FILE__, __LINE__);

	n = ACL_VSTRING_LEN(strbuf);
	if (n <= 0)
		acl_msg_fatal("%s, %s(%d): len(%d) <= 0",
			myname, __FILE__, __LINE__, n);

	n = acl_vstream_buffed_writen(stream, acl_vstring_str(strbuf), n);

	acl_vstring_free(strbuf);
	return n;
}

int acl_vstream_buffed_fprintf(ACL_VSTREAM *stream, const char *fmt, ...)
{
	const char *myname = "acl_vstream_buffed_fprintf";
	va_list ap;
	int   n;

	if (stream == NULL || fmt == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	va_start(ap, fmt);
	n = acl_vstream_buffed_vfprintf(stream, fmt, ap);
	va_end(ap);
	return n;
}

int acl_vstream_buffed_printf(const char *fmt, ...)
{
	const char *myname = "acl_vstream_buffed_printf";
	va_list ap;
	int   n;

	if (fmt == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (ACL_VSTREAM_OUT->fd.h_file == (ACL_FILE_HANDLE) -1)
		acl_vstream_init();
	if (ACL_VSTREAM_OUT->fd.h_file == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): ACL_VSTREAM_OUT can't be inited",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}
	va_start(ap, fmt);
	n = acl_vstream_buffed_vfprintf(ACL_VSTREAM_OUT, fmt, ap);
	va_end(ap);

	return n;
}

int acl_vstream_buffed_fputs(const char *s, ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_buffed_fputs";

	if (s == NULL || fp == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if ((*s) != 0 && acl_vstream_buffed_fwrite(fp, s, strlen(s))
		== ACL_VSTREAM_EOF)
	{
		return ACL_VSTREAM_EOF;
	}
	if (acl_vstream_buffed_fwrite(fp, "\r\n", 2) == ACL_VSTREAM_EOF)
		return ACL_VSTREAM_EOF;
	return 0;
}

int acl_vstream_buffed_puts(const char *s)
{
	const char *myname = "acl_vstream_buffed_puts";

	if (ACL_VSTREAM_OUT->fd.h_file == (ACL_FILE_HANDLE) -1)
		acl_vstream_init();
	if (ACL_VSTREAM_OUT->fd.h_file == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): ACL_VSTREAM_OUT can't be inited",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	return acl_vstream_buffed_fputs(s, ACL_VSTREAM_OUT);
}

int acl_vstream_fsync(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_fsync";

	if (fp == NULL) {
		acl_msg_error("%s(%d): fp null", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	}
	if (fp->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s(%d): not a file stream", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): fflush fp stream's buff error(%s)",
			myname, __LINE__, acl_last_serror());
		return ACL_VSTREAM_EOF;
	}
	
	if (acl_file_fflush(ACL_VSTREAM_FILE(fp), fp, fp->context) < 0) {
		acl_msg_error("%s(%d): fflush to disk error(%s)",
			myname, __LINE__, acl_last_serror());
			return ACL_VSTREAM_EOF;
	}

	return 0;
}

void acl_vstream_buffed_space(ACL_VSTREAM *stream)
{
	if (stream->wbuf == NULL) {
		stream->wbuf_size = 8192;
		stream->wbuf_dlen = 0;
		stream->wbuf = acl_mymalloc(stream->wbuf_size);
	}
}

int acl_vstream_fflush(ACL_VSTREAM *stream)
{
	const char *myname = "acl_vstream_fflush";
	unsigned char *ptr;
	int   n;

	if (stream == NULL) {
		acl_msg_error("%s(%d): stream null", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	} else if (stream->wbuf == NULL || stream->wbuf_dlen == 0)
		return 0;

	ptr = stream->wbuf;
	while (stream->wbuf_dlen > 0) {
		n = __vstream_sys_write(stream, ptr, (int) stream->wbuf_dlen);
		if (n <= 0) {
			if (acl_last_error() == ACL_EINTR
				|| acl_last_error() == ACL_EAGAIN)
			{
				continue;
			}
			return ACL_VSTREAM_EOF;
		}

		stream->wbuf_dlen -= n;
		ptr += n;
	}

	if (stream->wbuf_dlen < 0)
		acl_msg_fatal("%s(%d): wbuf_dlen(%d) < 0",
			myname, __LINE__, (int) stream->wbuf_dlen);

	return ptr - stream->wbuf;
}

int acl_vstream_peekfd(ACL_VSTREAM *stream)
{
#ifdef ACL_UNIX
	int   n;

	if (stream != NULL && ACL_VSTREAM_SOCK(stream) != ACL_SOCKET_INVALID) {
		n = acl_peekfd(ACL_VSTREAM_SOCK(stream));
		if (n < 0)
			return (-1);
		n += ACL_VSTREAM_BFRD_CNT(stream);

		return n;
	}

	return -1;
#else
	const char *myname = "acl_vstream_peekfd";

	acl_msg_fatal("%s: not implement yet", myname);

	/* not reached */
	return -1;
#endif /* ACL_UNIX */
} 

ACL_VSTREAM *acl_vstream_fhopen(ACL_FILE_HANDLE fh, unsigned int oflags)
{
	const char *myname = "acl_vstream_fhopen";
	ACL_VSTREAM *fp;

	if (fh == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): fh invalid",
			myname, __FILE__, __LINE__);
		return NULL;
	}

	fp = acl_vstream_fdopen(ACL_SOCKET_INVALID, oflags,
		4096, 0, ACL_VSTREAM_TYPE_FILE);
	if (fp == NULL)
		return NULL;

	fp->fd.h_file = fh;
	return fp;
}

/* 定义流的缓冲区的默认大小 */

#define ACL_VSTREAM_DEF_MAXLEN  8192

ACL_VSTREAM *acl_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
	size_t buflen, int rw_timeo, int fdtype)
{
	const char *myname = "acl_vstream_fdopen";
	ACL_VSTREAM *stream;

	stream = (ACL_VSTREAM *) acl_mycalloc(1, sizeof(ACL_VSTREAM));

	if (buflen <= 0 && !(fdtype & (ACL_VSTREAM_TYPE_LISTEN_INET
		| ACL_VSTREAM_TYPE_LISTEN_UNIX)))
	{
		acl_msg_warn("%s(%d): buflen(%d) invalid",
			myname, __LINE__, (int) buflen);
	}

	if (buflen < ACL_VSTREAM_DEF_MAXLEN)
		buflen = ACL_VSTREAM_DEF_MAXLEN;

	/* XXX: 只有非监听流才需要有读缓冲区 */
	/* XXX: 目前 UDP 服务端口号在 MASTER 框架中暂时当监听套接口用，所以
	   需要给其分配读缓冲区
	 */

	if ((fdtype & ACL_VSTREAM_TYPE_LISTEN_INET)
	    || (fdtype & ACL_VSTREAM_TYPE_LISTEN_UNIX))
	{
		fdtype |= ACL_VSTREAM_TYPE_LISTEN;
	}

	stream->read_buf = (unsigned char *) acl_mymalloc(buflen + 1);

	if (fdtype == 0) {
		fdtype = ACL_VSTREAM_TYPE_SOCK;
		acl_msg_warn("%s(%d): fdtype(0), set to ACL_VSTREAM_TYPE_SOCK",
			myname, __LINE__);
	}

	stream->read_buf_len     = buflen;
	stream->type             = fdtype;
	ACL_VSTREAM_SOCK(stream) = fd;
#ifdef ACL_MS_WINDOWS
	stream->iocp_sock        = ACL_SOCKET_INVALID;
#endif
	stream->read_ptr         = stream->read_buf;
	stream->oflags           = oflags;
	ACL_SAFE_STRNCPY(stream->errbuf, "OK", sizeof(stream->errbuf));

	if (rw_timeo > 0)
		stream->rw_timeout = rw_timeo;
	else
		stream->rw_timeout = 0;

	stream->sys_getc = __sys_getc;
	if (fdtype == ACL_VSTREAM_TYPE_FILE) {
		stream->fread_fn  = acl_file_read;
		stream->fwrite_fn = acl_file_write;
		stream->fwritev_fn = acl_file_writev;
		stream->fclose_fn = acl_file_close;
	} else {
		stream->read_fn  = acl_socket_read;
		stream->write_fn = acl_socket_write;
		stream->writev_fn = acl_socket_writev;
		stream->close_fn = acl_socket_close;
	}

	stream->addr_local = __empty_string;
	stream->addr_peer = __empty_string;
	stream->path = __empty_string;

	stream->close_handle_lnk = acl_array_create(5);
	return stream;
}

ACL_VSTREAM *acl_vstream_clone(const ACL_VSTREAM *from)
{
	const char *myname = "acl_vstream_clone";
	ACL_VSTREAM *to;
	ACL_VSTREAM_CLOSE_HANDLE *handle_from, *handle_to;
	int   i, n;

	to = (ACL_VSTREAM *) acl_mycalloc(1, sizeof(ACL_VSTREAM));
	memcpy(to, from, sizeof(ACL_VSTREAM));
	to->read_buf = (unsigned char *)
		acl_mymalloc((int) to->read_buf_len + 1);
	memcpy(to->read_buf, from->read_buf, (size_t) to->read_buf_len);
	to->read_ptr = to->read_buf + (from->read_ptr - from->read_buf);

	if (from->addr_peer && from->addr_peer != __empty_string)
		to->addr_peer = acl_mystrdup(from->addr_peer);
	else
		to->addr_peer = __empty_string;

	if (from->addr_local && from->addr_local != __empty_string)
		to->addr_local = acl_mystrdup(from->addr_local);
	else
		to->addr_local = __empty_string;

	if (from->sa_peer) {
		to->sa_peer = (struct sockaddr_in*)
			acl_mymalloc(sizeof(struct sockaddr_in));
		memcpy(to->sa_peer, from->sa_peer,
			sizeof(struct sockaddr_in));
	}
	if (from->sa_local) {
		to->sa_local = (struct sockaddr_in*)
			acl_mymalloc(sizeof(struct sockaddr_in));
		memcpy(to->sa_local, from->sa_local,
			sizeof(struct sockaddr_in));
	}

	if (from->path && from->path != __empty_string)
		to->path = acl_mystrdup(from->path);
	else
		to->path = __empty_string;

	to->ioctl_read_ctx = NULL;
	to->ioctl_write_ctx = NULL;
	to->fdp = NULL;
	to->context = from->context;
	to->close_handle_lnk = acl_array_create(5);

	if (from->close_handle_lnk == NULL)
		return to;

	n = acl_array_size(from->close_handle_lnk);
	for (i = 0; i < n; i++) {
		handle_from = (ACL_VSTREAM_CLOSE_HANDLE *)
			acl_array_index(from->close_handle_lnk, i);
		if (handle_from == NULL)
			continue;
		if (handle_from->close_fn == NULL)
			continue;

		handle_to = (ACL_VSTREAM_CLOSE_HANDLE *)
			acl_mycalloc(1, sizeof(ACL_VSTREAM_CLOSE_HANDLE));
		handle_to->close_fn = handle_from->close_fn;
		handle_to->context = handle_from->context;

		if (acl_array_append(to->close_handle_lnk, handle_to) < 0)
			acl_msg_fatal("%s, %s(%d): acl_array_append error=%s",
				myname, __FILE__, __LINE__, acl_last_serror());
	}

	return to;
}

int acl_vstream_set_fdtype(ACL_VSTREAM *stream, int type)
{
	if (type == ACL_VSTREAM_TYPE_FILE) {
		stream->fread_fn  = acl_file_read;
		stream->fwrite_fn = acl_file_write;
		stream->fclose_fn = acl_file_close;
	} else if (type == ACL_VSTREAM_TYPE_SOCK) {
		stream->read_fn  = acl_socket_read;
		stream->write_fn = acl_socket_write;
		stream->close_fn = acl_socket_close;
	}

	return -1;
}
/* acl_vstream_fopen - open buffered file stream */

ACL_VSTREAM *acl_vstream_fopen(const char *path, unsigned int oflags,
	int mode, size_t buflen)
{
	ACL_VSTREAM *fp;
	ACL_FILE_HANDLE fh;

	/* for linux2.6 */
#ifdef  _LARGEFILE64_SOURCE
	oflags |= O_LARGEFILE;
#endif

#ifdef	ACL_MS_WINDOWS
	oflags |= O_BINARY;
#endif

	fh = acl_file_open(path, oflags, mode);

	if (fh == ACL_FILE_INVALID)
		return NULL;

	fp = acl_vstream_fdopen(ACL_SOCKET_INVALID, oflags, buflen,
		0, ACL_VSTREAM_TYPE_FILE);
	if (fp == NULL)
		return NULL;

	fp->fd.h_file = fh;
	acl_vstream_set_path(fp, path);
	return fp;
}

char *acl_vstream_loadfile(const char *path)
{
	return acl_vstream_loadfile2(path, NULL);
}

char *acl_vstream_loadfile2(const char *path, ssize_t *size)
{
	const char *myname = "acl_vstream_loadfile";
	ACL_VSTREAM *fp;
#ifdef	ACL_MS_WINDOWS
	int   oflags = O_RDONLY | O_BINARY;
#else
	int   oflags = O_RDONLY;
#endif
	int   mode = S_IREAD;
	int   ret;
	ACL_VSTRING *vbuf;
	unsigned char buf[4096];

	if (size)
		*size = -1;

	if (path == NULL || *path == 0) {
		acl_msg_error("%s, %s(%d):path invalid",
			myname, __FILE__, __LINE__);
		return NULL;
	}

	fp = acl_vstream_fopen(path, oflags, mode, 4096);
	if (fp == NULL) {
		acl_msg_error("%s, %s(%d): open file(%s) error(%s)",
			myname, __FILE__, __LINE__,
			path, acl_last_serror());
		return NULL;
	}

	vbuf = acl_vstring_alloc(1024);
	if (buf == NULL) {
		acl_msg_error("%s, %s(%d): alloc vstring error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
		acl_vstream_close(fp);
		return NULL;
	}

	while (1) {
		ret = acl_vstream_read(fp, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			break;
		acl_vstring_memcat(vbuf, (char *) buf, ret);
	}

	if (size)
		*size = (ssize_t) ACL_VSTRING_LEN(vbuf);
	acl_vstream_close(fp);
	ACL_VSTRING_TERMINATE(vbuf);
	return acl_vstring_export(vbuf);
}

/* acl_vstream_ctl - fine control */

void acl_vstream_ctl(ACL_VSTREAM *stream, int name,...)
{
	const char *myname = "acl_vstream_ctl";
	va_list ap;
	int   n;
	char *ptr;

	va_start(ap, name);
	for (; name != ACL_VSTREAM_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_VSTREAM_CTL_READ_FN:
			stream->read_fn = va_arg(ap, ACL_VSTREAM_RD_FN);
			break;
		case ACL_VSTREAM_CTL_WRITE_FN:
			stream->write_fn = va_arg(ap, ACL_VSTREAM_WR_FN);
			break;
		case ACL_VSTREAM_CTL_CONTEXT:
			stream->context = va_arg(ap, char *);
			break;
		case ACL_VSTREAM_CTL_PATH:
			ptr = va_arg(ap, char*);
			ACL_SAFE_STRNCPY(stream->addr_peer, ptr,
				sizeof(stream->addr_peer));
			break;
		case ACL_VSTREAM_CTL_FD:
			ACL_VSTREAM_SOCK(stream) = va_arg(ap, ACL_SOCKET);
			break;
		case ACL_VSTREAM_CTL_TIMEOUT:
			stream->rw_timeout = va_arg(ap, int);
			break;
		case ACL_VSTREAM_CTL_CACHE_SEEK:
			n = va_arg(ap, int);
			if (n)
				stream->flag |= ACL_VSTREAM_FLAG_CACHE_SEEK;
			else
				stream->flag &= ~ACL_VSTREAM_FLAG_CACHE_SEEK;
			break;
		default:
			acl_msg_panic("%s, %s(%d): bad name %d",
				myname, __FILE__, __LINE__, name);
		}
	}
	va_end(ap);
}

acl_off_t acl_vstream_fseek2(ACL_VSTREAM *stream, acl_off_t offset, int whence)
{
	const char *myname = "acl_vstream_fseek2";
	acl_off_t n;

	if (stream == NULL || ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID)
		acl_msg_fatal("%s, %s(%d): input error",
			myname, __FILE__, __LINE__);

	if (stream->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s, %s(%d): type(%d) not ACL_VSTREAM_TYPE_FILE",
			myname, __FILE__, __LINE__, stream->type);
		return -1;
	}

	if (stream->wbuf_dlen > 0) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF) {
			acl_msg_error("%s, %s(%d): acl_vstream_fflush error",
				myname, __FILE__, __LINE__);
			return -1;
		}
	}

	if ((stream->flag & ACL_VSTREAM_FLAG_CACHE_SEEK) == 0) {
		stream->read_cnt = 0;
		goto SYS_SEEK2;
	}

	/* 获得真正的当前文件指针位置 */
	n = acl_lseek(ACL_VSTREAM_FILE(stream), (acl_off_t) 0, SEEK_CUR);
	if (n < 0)
		return -1;

	if (whence == SEEK_CUR) {
		if (stream->read_cnt >= offset) {
			stream->read_cnt -= (int) offset;
			n = -stream->read_cnt;  /* 计算出真实的文件位置 */
			stream->read_cnt = 0;
		} else if (stream->read_cnt >= 0) {
			offset -= stream->read_cnt;
			n = offset;  /* 计算出真实的文件位置 */
			stream->read_cnt = 0;
		} else { /* stream->read_cnt < 0 ? */
			acl_msg_fatal("%s, %s(%d): invalud read_cnt = %d",
				myname, __FILE__, __LINE__,
				(int) stream->read_cnt);
		}
	} else {
		n = offset;
		stream->read_cnt = 0;
	}

SYS_SEEK2:
	/* 定位到合适的位置 */
	stream->sys_offset = acl_lseek(
		ACL_VSTREAM_FILE(stream), offset, whence);
	stream->offset = stream->sys_offset;
	return stream->offset;
}

acl_off_t acl_vstream_fseek(ACL_VSTREAM *stream, acl_off_t offset, int whence)
{
	const char *myname = "acl_vstream_fseek";
	acl_off_t n;

	if (stream == NULL || ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID)
		acl_msg_fatal("%s, %s(%d): input error",
			myname, __FILE__, __LINE__);

	if (stream->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s, %s(%d): type(%d) not ACL_VSTREAM_TYPE_FILE",
			myname, __FILE__, __LINE__, stream->type);
		return -1;
	}

	if (stream->wbuf_dlen > 0) {
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF) {
			acl_msg_error("%s, %s(%d): acl_vstream_fflush error",
				myname, __FILE__, __LINE__);
			return -1;
		}
	}

	if ((stream->flag & ACL_VSTREAM_FLAG_CACHE_SEEK) == 0) {
		stream->read_cnt = 0;
		goto SYS_SEEK;
	}

	if (whence == SEEK_CUR) {
		/* 相对当前流位置 stream->offset 开始偏移 offset 的位置 */

		/* 必须严格检验 */
		if (stream->offset + stream->read_cnt != stream->sys_offset) {
			acl_msg_error("%s, %s(%d): offset(" ACL_FMT_I64D
				") + read_cnt(%d) != sys_offset("
				ACL_FMT_I64D ")", myname, __FILE__, __LINE__,
				stream->offset, stream->read_cnt,
				stream->sys_offset);
			stream->read_cnt = 0;
			goto SYS_SEEK;
		}

		/* 是否在读缓冲区间内 */
		if (stream->read_cnt >= offset) {
			/* 因为要从 stream->offset 偏移 offset 个字节后, 读指针
			 * stream->read_ptr 依然在缓冲区内, 所以只需要移动读指针
			 * 且减少缓冲区字节数、增加 stream->offset 偏移量即可.
			 */
			stream->read_cnt -= (int) offset;
			stream->read_ptr += (int) offset;
			stream->offset += offset;
			return stream->offset;
		} else if (stream->read_cnt >= 0) {
			/* 因为要计算从当前流位置 stream->offset 开始偏移 offset
			 * 的位置,而且流中还存在一定的缓存数据(stream->read_cnt),
			 * 所以需要先从 stream->offset 开始移动 stream->read_cnt
			 * 个字节(移动出读缓冲区),然后再移动剩余的字节
			 * (即 offset - stream->read_cnt) 即可; 因为已经称出读缓
			 * 冲区，所以需要将 stream->read_cnt 置 0.
			 */
			offset -= stream->read_cnt;
			stream->read_cnt = 0;
		} else { /* stream->read_cnt < 0 ? */
			acl_msg_error("%s, %s(%d): invalud read_cnt = %d",
				myname, __FILE__, __LINE__,
				(int) stream->read_cnt);
			stream->read_cnt = 0;
		}
	} else if (whence == SEEK_SET) {
#if 0
		/* 获得真正的当前文件指针位置 */
		stream->sys_offset = acl_lseek(ACL_VSTREAM_FILE(stream),
			(off_t) 0, SEEK_CUR);
#endif
		/* 利用缓存的偏移位置 */

		if (stream->sys_offset < 0) {
			acl_msg_error("%s, %s(%d): seek n(" ACL_FMT_I64D
				") invalid", myname, __FILE__, __LINE__,
				stream->sys_offset);
			stream->read_cnt = 0;
			goto SYS_SEEK;
		}

		/* 必须严格检验 */
		if (stream->offset + stream->read_cnt != stream->sys_offset) {
			acl_msg_error("%s, %s(%d): offset(" ACL_FMT_I64D
				") + read_cnt(%d) != sys_offset("
				ACL_FMT_I64D ")", myname, __FILE__, __LINE__,
				stream->offset, stream->read_cnt,
				stream->sys_offset);
			stream->read_cnt = 0;
			goto SYS_SEEK;
		}

		/* 如果读数据指针经过移动，可以将其回移，因为缓冲区内数据
		 * 并未破坏，可以复用
		 */
		if (stream->read_ptr > stream->read_buf) {
			n = stream->read_ptr - stream->read_buf;
			stream->offset -= n;
			stream->read_ptr = stream->read_buf;
			stream->read_cnt += (int) n;
		}

		/* 判断请求的偏移位置是否在读缓存区间内 */
		if (offset >= stream->offset && offset <= stream->sys_offset) {
			n = offset - stream->offset;
			stream->read_cnt -= (int) n;
			stream->read_ptr += n;
			stream->offset += n;
			return stream->offset;
		}
		stream->read_cnt = 0;
	} else
		stream->read_cnt = 0;

SYS_SEEK:
	/* 调用系统调用定位位置 */
	stream->sys_offset = acl_lseek(ACL_VSTREAM_FILE(stream), offset, whence);
	stream->offset = stream->sys_offset;

	return stream->offset;
}

#ifdef ACL_MS_WINDOWS
int acl_file_ftruncate(ACL_VSTREAM *fp, acl_off_t length)
{
	const char *myname = "acl_file_ftruncate";
	ACL_FILE_HANDLE hf = ACL_VSTREAM_FILE(fp);

	/* 参见：C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\crt\src
	* osfinfo.c
	* _open_osfhandle: 将WIN32 API的文件句柄转换为标准C的文件句柄
	* _get_osfhandle: 根据标准C文件句柄查询WIN32 API文件句柄
	* _free_osfhnd: 释放由 _open_osfhandle 打开的标准C文件句柄的资源，
	*               但并不实际关闭该WIN32 API句柄，所以还得要对其真实
	*               WIN32 API文件句柄进行关闭
	* close.c
	* _close: 关闭并释放标准C的文件句柄
	*/

	if (acl_vstream_fseek(fp, length, SEEK_SET) < 0) {
		acl_msg_error("%s, %s(%d): fseek error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
		return -1;
	}

	if (!SetEndOfFile(hf)) {
		acl_msg_error("%s, %s(%d): SetEndOfFile error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
		return -1;
	}

	if (acl_vstream_fseek(fp, 0, SEEK_SET) < 0) {
		acl_msg_error("%s, %s(%d): fseek error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

int acl_file_truncate(const char *path, acl_off_t length)
{
	const char *myname = "acl_file_truncate";
	ACL_VSTREAM* fp;

	fp = acl_vstream_fopen(path, O_WRONLY | O_BINARY | O_CREAT, 0600, 1024);
	if (fp == NULL) {
		acl_msg_error("%s, %s(%d): fopen file(%s) error(%s)",
			myname, __FILE__, __LINE__, path, acl_last_serror());
		return -1;
	}

	if (acl_file_ftruncate(fp, length) < 0) {
		acl_vstream_close(fp);
		return -1;
	}
	acl_vstream_close(fp);
	return 0;
}
#elif defined(ACL_UNIX)
int acl_file_ftruncate(ACL_VSTREAM *fp, acl_off_t length)
{
	ACL_FILE_HANDLE hf = ACL_VSTREAM_FILE(fp);

	return ftruncate(hf, length);
}

int acl_file_truncate(const char *path, acl_off_t length)
{
	return truncate(path, length);
}
#endif /* !ACL_MS_WINDOWS, ACL_UNIX */

int acl_vstream_fstat(ACL_VSTREAM *fp, struct acl_stat *buf)
{
	const char *myname = "acl_vstream_fstat";

	if (fp == NULL || buf == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return -1;
	} else if (fp->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s(%d): not a file stream", myname, __LINE__);
		return -1;
	}

	return acl_fstat(ACL_VSTREAM_FILE(fp), buf);
}

acl_int64 acl_vstream_fsize(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_fsize";

	if (fp == NULL) {
		acl_msg_error("%s(%d): fp null", myname, __LINE__);
		return -1;
	} else if (fp->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s(%d): not a file stream", myname, __LINE__);
		return -1;
	}
	return acl_file_fsize(ACL_VSTREAM_FILE(fp), fp, fp->context)
		+ fp->wbuf_dlen;
}

void acl_vstream_reset(ACL_VSTREAM *stream)
{
	if (stream) {
		stream->read_cnt = 0;
		stream->read_ptr = stream->read_buf;
		stream->flag     = ACL_VSTREAM_FLAG_RW;
		stream->total_read_cnt = 0;
		stream->total_write_cnt = 0;
		stream->sys_read_ready = 0;
		stream->wbuf_dlen = 0;
		stream->offset = 0;
		stream->nrefer = 0;
		stream->read_buf_len = 0;
		ACL_SAFE_STRNCPY(stream->errbuf, "OK", sizeof(stream->errbuf));
		acl_vstream_clean_close_handle(stream);
		if (stream->fdp != NULL)
			event_fdtable_reset(stream->fdp);
	}
}

void acl_vstream_free(ACL_VSTREAM *stream)
{
	if (stream->nrefer > 0) {
		/* 设置延迟释放标志位 */
		stream->flag |= ACL_VSTREAM_FLAG_DEFER_FREE;
		return;
	}

	if (stream->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = acl_array_size(stream->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i++) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				acl_array_index(stream->close_handle_lnk, i);
			if (close_handle == NULL)
				break;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在
			 * close_fn 里调用了删除回调函数的操作而造成对同一内
			 * 存的多次释放
			 */
			acl_array_delete_idx(stream->close_handle_lnk, i, NULL);
			close_handle->close_fn(stream, close_handle->context);
			acl_myfree(close_handle);
		}
		acl_array_destroy(stream->close_handle_lnk, NULL);
	}

	if (stream->fdp != NULL)
		event_fdtable_free(stream->fdp);
	if (stream->read_buf != NULL)
		acl_myfree(stream->read_buf);
	if (stream->wbuf != NULL)
		acl_myfree(stream->wbuf);

	if (stream->addr_peer && stream->addr_peer != __empty_string)
		acl_myfree(stream->addr_peer);
	if (stream->addr_local && stream->addr_local != __empty_string)
		acl_myfree(stream->addr_local);
	if (stream->sa_peer)
		acl_myfree(stream->sa_peer);
	if (stream->sa_local)
		acl_myfree(stream->sa_local);
	if (stream->path && stream->path != __empty_string)
		acl_myfree(stream->path);

	acl_myfree(stream);
}

int acl_vstream_close(ACL_VSTREAM *stream)
{
	const char *myname = "acl_vstream_close";
	int  ret = 0;

	if (stream->nrefer > 0) {
		/* 设置延迟释放标志位 */
		stream->flag |= ACL_VSTREAM_FLAG_DEFER_FREE;
		return 0;
	}

	if (stream->wbuf_dlen > 0)
		if (acl_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			acl_msg_error("%s: fflush stream error", myname);

	/* 必须在调用各个关闭回调函数之前将连接关闭，否则会影响 iocp 的事件引擎
	 * 的正常工作。在使用 iocp 作为事件引擎时，当流关闭时会调用 events_iocp.c
	 * 中的 stream_on_close，该函数会释放掉 fdp->event_read/fdp->event_write
	 * 两个对象，但当套接口未关闭时，这两个对象有可能会被 iocp 使用时，只有
	 * 当套接口关闭时，iocp 才不会使用这两个对象中的 IOCP_EVENT->overlapped 等
	 * 成员. ---2011.5.18, zsx
	 */
	/*
	 * 2011.5.18 的改动虽然解决了事件引擎为 iocp 的问题，但同时造成了 win32
	 * 窗口消息引擎的问题，虽然 win32 消息引擎的方式在关闭套接口之前会回调
	 * stream_on_close，而该回调要求套接口必须是打开的，既然二者出现了冲突，
	 * 则 iocp 的问题还是由 iocp 引擎本身去解决吧，即在 iocp 引擎的
	 * stream_on_close 中，在释放 fdp->event_read/fdp->event_write 之前关闭
	 * 套接口即可，在 acl_vstream_close 最后需要关闭套接口时只要根据句柄是否
	 * 有效来判断是否调用关闭过程. ---2011.5.19, zsx
	 */
	/*
	if (stream->read_buf != NULL)
		acl_myfree(stream->read_buf);
	if (ACL_VSTREAM_SOCK(stream) != ACL_SOCKET_INVALID && stream->close_fn)
		ret = stream->close_fn(ACL_VSTREAM_SOCK(stream));
	else if (ACL_VSTREAM_FILE(stream) != ACL_FILE_INVALID && stream->fclose_fn)
		ret = stream->fclose_fn(ACL_VSTREAM_FILE(stream));
	ACL_VSTREAM_SOCK(stream) = ACL_SOCKET_INVALID;
	ACL_VSTREAM_FILE(stream) = ACL_FILE_INVALID;
	*/

	if (stream->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = acl_array_size(stream->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i--) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				acl_array_index(stream->close_handle_lnk, i);
			if (close_handle == NULL)
				continue;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在
			 * close_fn 里调用了删除回调函数的操作而造成对同一内存
			 * 的多次释放
			 */
			acl_array_delete_idx(stream->close_handle_lnk, i, NULL);
			close_handle->close_fn(stream, close_handle->context);
			acl_myfree(close_handle);
		}
		acl_array_destroy(stream->close_handle_lnk, NULL);
	}

	if (ACL_VSTREAM_SOCK(stream) != ACL_SOCKET_INVALID && stream->close_fn)
		ret = stream->close_fn(ACL_VSTREAM_SOCK(stream));
	else if (ACL_VSTREAM_FILE(stream) != ACL_FILE_INVALID
		&& stream->fclose_fn)
	{
		ret = stream->fclose_fn(ACL_VSTREAM_FILE(stream));
	}

	if (stream->fdp != NULL)
		event_fdtable_free(stream->fdp);
	if (stream->read_buf != NULL)
		acl_myfree(stream->read_buf);
	if (stream->wbuf != NULL)
		acl_myfree(stream->wbuf);

	if (stream->addr_local && stream->addr_local != __empty_string)
		acl_myfree(stream->addr_local);
	if (stream->addr_peer && stream->addr_peer != __empty_string)
		acl_myfree(stream->addr_peer);
	if (stream->sa_peer)
		acl_myfree(stream->sa_peer);
	if (stream->sa_local)
		acl_myfree(stream->sa_local);
	if (stream->path && stream->path != __empty_string)
		acl_myfree(stream->path);

	acl_myfree(stream);
	return ret;
}

static void set_sock_addr(struct sockaddr_in *saddr, const char *addr)
{
	const char *myname = "set_sockaddr";
	char  buf[128], *ptr;
	int   port;

	snprintf(buf, sizeof(buf), "%s", addr);
	ptr = strchr(buf, ':');
	if (ptr == NULL) {
		acl_msg_error("%s, %s(%d): addr(%s) invalid",
			myname, __FILE__, __LINE__, addr);
		return;
	}
	*ptr++ = 0;
	port = atoi(ptr);

	saddr->sin_family = AF_INET;
	saddr->sin_port = htons(port);
	saddr->sin_addr.s_addr = inet_addr(buf);
}

void acl_vstream_set_local(ACL_VSTREAM *stream, const char *addr)
{
	if (stream->addr_local == __empty_string || stream->addr_local == NULL)
		stream->addr_local = acl_mystrdup(addr);
	else {
		acl_myfree(stream->addr_local);
		stream->addr_local = acl_mystrdup(addr);
	}

	if (stream->sa_local == NULL) {
		stream->sa_local_size = sizeof(struct sockaddr_in);
		stream->sa_local = (struct sockaddr_in*)
			acl_mycalloc(1, stream->sa_local_size);
	} else
		memset(stream->sa_local, 0, stream->sa_local_size);

	set_sock_addr(stream->sa_local, addr);
	stream->sa_local_len = stream->sa_local_size;
}

void acl_vstream_set_local_addr(ACL_VSTREAM *stream, const struct sockaddr_in *sa)
{
	int   port;
	char  ip[64], addr[64];

	if (stream->sa_local == NULL) {
		stream->sa_local_size = sizeof(struct sockaddr_in);
		stream->sa_local = (struct sockaddr_in*)
			acl_mymalloc(stream->sa_local_size);
	}

	memcpy(stream->sa_local, sa, stream->sa_local_size);
	stream->sa_local_len = stream->sa_local_size;

	ip[0] = 0;
	acl_inet_ntoa(sa->sin_addr, ip, sizeof(ip));
	port = ntohs(sa->sin_port);
	snprintf(addr, sizeof(addr), "%s:%d", ip, port);

	if (stream->addr_local == __empty_string || stream->addr_local == NULL)
		stream->addr_local = acl_mystrdup(addr);
	else {
		acl_myfree(stream->addr_local);
		stream->addr_local = acl_mystrdup(addr);
	}
}

void acl_vstream_set_peer(ACL_VSTREAM *stream, const char *addr)
{
	if (stream->addr_peer == __empty_string || stream->addr_peer == NULL)
		stream->addr_peer = acl_mystrdup(addr);
	else {
		acl_myfree(stream->addr_peer);
		stream->addr_peer = acl_mystrdup(addr);
	}

	if (stream->sa_peer == NULL) {
		stream->sa_peer_size = sizeof(struct sockaddr_in);
		stream->sa_peer = (struct sockaddr_in*)
			acl_mycalloc(1, stream->sa_peer_size);
	} else
		memset(stream->sa_peer, 0, stream->sa_peer_size);

	set_sock_addr(stream->sa_peer, addr);
	stream->sa_peer_len = stream->sa_peer_size;
}

void acl_vstream_set_peer_addr(ACL_VSTREAM *stream, const struct sockaddr_in *sa)
{
	int   port;
	char  ip[64], addr[64];

	if (stream->sa_peer == NULL) {
		stream->sa_peer_size = sizeof(struct sockaddr_in);
		stream->sa_peer = (struct sockaddr_in*)
			acl_mymalloc(stream->sa_peer_size);
	}

	memcpy(stream->sa_peer, sa, stream->sa_peer_size);
	stream->sa_peer_len = stream->sa_peer_size;

	ip[0] = 0;
	acl_inet_ntoa(sa->sin_addr, ip, sizeof(ip));
	port = ntohs(sa->sin_port);
	snprintf(addr, sizeof(addr), "%s:%d", ip, port);

	if (stream->addr_peer == __empty_string || stream->addr_peer == NULL)
		stream->addr_peer = acl_mystrdup(addr);
	else {
		acl_myfree(stream->addr_peer);
		stream->addr_peer = acl_mystrdup(addr);
	}
}

void acl_vstream_set_path(ACL_VSTREAM *stream, const char *path)
{
	if (stream->path == __empty_string || stream->path == NULL)
		stream->path = acl_mystrdup(path);
	else {
		acl_myfree(stream->path);
		stream->path = acl_mystrdup(path);
	}
}

void acl_vstream_call_close_handles(ACL_VSTREAM *stream)
{
	if (stream->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = acl_array_size(stream->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i--) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				acl_array_index(stream->close_handle_lnk, i);
			if (close_handle == NULL)
				continue;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在
			 * close_fn 里调用了删除回调函数的操作而造成对同一内存
			 * 的多次释放
			 */
			acl_array_delete_idx(stream->close_handle_lnk, i, NULL);
			close_handle->close_fn(stream, close_handle->context);
			acl_myfree(close_handle);
		}
		acl_array_clean(stream->close_handle_lnk, NULL);
	}
}

void acl_vstream_add_close_handle(ACL_VSTREAM *stream,
	void (*close_fn)(ACL_VSTREAM*, void*), void *context)
{
	const char *myname = "acl_vstream_add_close_handle";
	ACL_VSTREAM_CLOSE_HANDLE *close_handle;
	ACL_ITER  iter;

	if (stream == NULL)
		acl_msg_fatal("%s, %s(%d): stream null",
			myname, __FILE__, __LINE__);
	if (stream->close_handle_lnk == NULL)
		acl_msg_fatal("%s, %s(%d): close_handle_lnk null",
			myname, __FILE__, __LINE__);
	if (close_fn == NULL)
		acl_msg_fatal("%s, %s(%d): close_fn null",
			myname, __FILE__, __LINE__);

	acl_foreach(iter, stream->close_handle_lnk) {
		close_handle = (ACL_VSTREAM_CLOSE_HANDLE*) iter.data;
		if (close_handle->close_fn == close_fn
			&& close_handle->context == context)
		{
			return;
		}
	}

	close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
		acl_mycalloc(1, sizeof(ACL_VSTREAM_CLOSE_HANDLE));
	if (close_handle == NULL) {
		acl_msg_fatal("%s, %s(%d): calloc error=%s",
			myname, __FILE__, __LINE__, acl_last_serror());
	}
	close_handle->close_fn = close_fn;
	close_handle->context = context;

	if (acl_array_append(stream->close_handle_lnk, close_handle) < 0)
		acl_msg_fatal("%s, %s(%d): acl_array_append error=%s",
			myname, __FILE__, __LINE__, acl_last_serror());
}

void acl_vstream_delete_close_handle(ACL_VSTREAM *stream,
	void (*close_fn)(ACL_VSTREAM*, void*), void *context)
{
	const char *myname = "acl_vstream_delete_close_handle";
	ACL_VSTREAM_CLOSE_HANDLE *close_handle;
	int   i, n;

	if (stream == NULL) {
		acl_msg_error("%s(%d): stream null", myname, __LINE__);
		return;
	}
	if (stream->close_handle_lnk == NULL) {
		acl_msg_error("%s(%d): close_handle_lnk null",
			myname, __LINE__);
		return;
	}
	if (close_fn == NULL) {
		acl_msg_error("%s(%d): close_fn null", myname, __LINE__);
		return;
	}

	n = acl_array_size(stream->close_handle_lnk);
	if (n <= 0)
		return;

	/* 因为添加时是正序的, 所以在删除时是倒序的,
	 * 这样对动态数组的使用的效率才会比较高, 
	 * 避免了动态数组内部移动的情况
	 */
	for (i = n - 1; i >= 0; i--) {
		close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
			acl_array_index(stream->close_handle_lnk, i);
		if (close_handle == NULL)
			continue;
		if (close_handle->close_fn == close_fn
		    && close_handle->context == context) {
			acl_array_delete_idx(stream->close_handle_lnk, i, NULL);
			acl_myfree(close_handle);
			break;
		}
	}
}

void acl_vstream_clean_close_handle(ACL_VSTREAM *stream)
{
	int   i, n;
	ACL_VSTREAM_CLOSE_HANDLE *close_handle;

	if (stream == NULL || stream->close_handle_lnk == NULL)
		return;

	n = acl_array_size(stream->close_handle_lnk);
	/* 因为添加时是正序的, 所以在删除时是倒序的,
	 * 这样对动态数组的使用的效率才会比较高, 
	 * 避免了动态数组内部移动的情况
	 */
	for (i = n - 1; i >= 0; i++) {
		close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
			acl_array_index(stream->close_handle_lnk, i);
		acl_array_delete_idx(stream->close_handle_lnk, i, NULL);
		acl_myfree(close_handle);
	}

	acl_array_clean(stream->close_handle_lnk, NULL);
}

const char *acl_vstream_strerror(ACL_VSTREAM *stream)
{
	static char err[] = "input error";

	if (stream == NULL)
		return (err);

	return (stream->errbuf);
}
