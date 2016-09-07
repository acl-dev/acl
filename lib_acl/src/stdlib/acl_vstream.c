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

#ifdef  ACL_WINDOWS
# include <io.h>
#elif defined(ACL_UNIX)
# include <sys/types.h>
# include <sys/socket.h>
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
#include "net/acl_sane_socket.h"
#include "stdlib/acl_vstream.h"

#endif

#include "../event/events_fdtable.h"

static char __empty_string[] = "";

 /*
  * Initialization of the three pre-defined streams. Pre-allocate a static
  * I/O buffer for the standard error fp, so that the error handler can
  * produce a diagnostic even when memory allocation fails.
  */

static unsigned char __vstream_stdin_buf[ACL_VSTREAM_BUFSIZE];
static unsigned char __vstream_stdout_buf[ACL_VSTREAM_BUFSIZE];
static unsigned char __vstream_stderr_buf[ACL_VSTREAM_BUFSIZE];

static int read_char(ACL_VSTREAM *fp);

ACL_VSTREAM acl_vstream_fstd[] = {              
	{       
#ifdef ACL_UNIX
		{ STDIN_FILENO },               /* h_file */
#elif defined(ACL_WINDOWS)
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
		0,                              /* read_ready */
		0,                              /* total_read_cnt */
		0,                              /* total_write_cnt */
		NULL,                           /* ioctl_read_ctx */
		NULL,                           /* ioctl_write_ctx */
		NULL,                           /* fdp */
		ACL_VSTREAM_FLAG_READ,          /* flag */
		"\0",                           /* errbuf */
		0,                              /* errnum */
		0,                              /* rw_timeout */
		NULL,                           /* addr_local */
		NULL,                           /* addr_peer */
		NULL,                           /* sa_local */
		NULL,                           /* sa_peer */
		0,                              /* sa_local_size */
		0,                              /* sa_peer_size */
		0,                              /* sa_local_len */
		0,                              /* sa_peer_len */
		NULL,                           /* path */
		NULL,                           /* context */
		NULL,                           /* close_handle_lnk */
		read_char,                      /* sys_getc */
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
#ifdef ACL_WINDOWS
		NULL,                           /* hproc */
		ACL_SOCKET_INVALID,             /* iocp_sock */
#endif
		NULL,				/* objs_table */
	},

	{
#ifdef ACL_UNIX
		{ STDOUT_FILENO },              /* h_file */
#elif defined(ACL_WINDOWS)
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
		0,                              /* read_ready */
		0,                              /* total_read_cnt */
		0,                              /* total_write_cnt */
		NULL,                           /* ioctl_read_ctx */
		NULL,                           /* ioctl_write_ctx */
		NULL,                           /* fdp */
		ACL_VSTREAM_FLAG_WRITE,         /* flag */
		"\0",                           /* errbuf */
		0,                              /* errnum */
		0,                              /* rw_timeout */
		NULL,                           /* addr_local */
		NULL,                           /* addr_peer */
		NULL,                           /* sa_local */
		NULL,                           /* sa_peer */
		0,                              /* sa_local_size */
		0,                              /* sa_peer_size */
		0,                              /* sa_local_len */
		0,                              /* sa_peer_len */
		NULL,                           /* path */
		NULL,                           /* context */
		NULL,                           /* close_handle_lnk */
		read_char,                      /* sys_getc */
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
#ifdef ACL_WINDOWS
		NULL,                           /* hproc */
		ACL_SOCKET_INVALID,             /* iocp_sock */
#endif
		NULL,				/* objs_table */
	},
	{
#ifdef ACL_UNIX
		{ STDERR_FILENO },              /* h_file */
#elif defined(ACL_WINDOWS)
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
		0,                              /* read_ready */
		0,                              /* total_read_cnt */
		0,                              /* total_write_cnt */
		NULL,                           /* ioctl_read_ctx */
		NULL,                           /* ioctl_write_ctx */
		NULL,                           /* fdp */
		ACL_VSTREAM_FLAG_WRITE,         /* flag */
		"\0",                           /* errbuf */
		0,                              /* errnum */
		0,                              /* rw_timeout */
		NULL,                           /* addr_local */
		NULL,                           /* addr_peer */
		NULL,                           /* sa_local */
		NULL,                           /* sa_peer */
		0,                              /* sa_local_size */
		0,                              /* sa_peer_size */
		0,                              /* sa_local_len */
		0,                              /* sa_peer_len */
		NULL,                           /* path */
		NULL,                           /* context */
		NULL,                           /* close_handle_lnk */
		read_char,                      /* sys_getc */
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
#ifdef ACL_WINDOWS
		NULL,                           /* hproc */
		ACL_SOCKET_INVALID,             /* iocp_sock */
#endif
		NULL,				/* objs_table */
	},
};

#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str

void acl_vstream_init()
{
	static int __called = 0;

	if (__called)
		return;
	__called = 1;

#ifdef ACL_WINDOWS
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

static int sys_read(ACL_VSTREAM *in, void *buf, size_t size)
{
	const char *myname = "sys_read";
	int   read_cnt, nagain = 0;

	if (in->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(in) == ACL_FILE_INVALID) {
			in->read_ready = 0;
			return -1;
		}
	} else if (ACL_VSTREAM_SOCK(in) == ACL_SOCKET_INVALID) {
		in->read_ready = 0;
		return -1;
	}

#ifndef	SAFE_COPY
#define	SAFE_COPY(x, y) ACL_SAFE_STRNCPY((x), (y), sizeof((x)))
#endif

AGAIN:

	/* 清除系统错误号 */
	acl_set_error(0);

	if (in->type == ACL_VSTREAM_TYPE_FILE) {
		read_cnt = in->fread_fn(ACL_VSTREAM_FILE(in), buf, size,
			in->rw_timeout, in, in->context);
		if (in->read_cnt > 0)
			in->sys_offset += in->read_cnt;
	} else {
		read_cnt = in->read_fn(ACL_VSTREAM_SOCK(in), buf, size,
			in->rw_timeout, in, in->context);
	}

	if (read_cnt > 0) {
		in->flag &= ~ACL_VSTREAM_FLAG_BAD;
		in->errnum = 0;
		in->errbuf[0] = 0;
		in->total_read_cnt += read_cnt;

		return read_cnt;
	} else if (read_cnt == 0) {
		in->flag = ACL_VSTREAM_FLAG_EOF;
		in->errnum = 0;
		snprintf(in->errbuf, sizeof(in->errbuf), "closed by perr(%s)",
			acl_last_serror());

		return 0;
	}

	in->errnum = acl_last_error();

	if (in->errnum == ACL_EINTR) {
		if (nagain++ < 5)
			goto AGAIN;

#ifdef ACL_WINDOWS
		acl_msg_error("%s(%d), %s: nagain: %d too much, fd: %lld",
			__FILE__, __LINE__, myname, nagain,
			in->type == ACL_VSTREAM_TYPE_FILE ?
			(long long) ACL_VSTREAM_FILE(in) : ACL_VSTREAM_SOCK(in));
#else
		acl_msg_error("%s(%d), %s: nagain: %d too much, fd: %d",
			__FILE__, __LINE__, myname, nagain,
			in->type == ACL_VSTREAM_TYPE_FILE ?
			(int) ACL_VSTREAM_FILE(in) : ACL_VSTREAM_SOCK(in));
#endif
	} else if (in->errnum == ACL_ETIMEDOUT) {
		in->flag |= ACL_VSTREAM_FLAG_TIMEOUT;
		SAFE_COPY(in->errbuf, "read timeout");
	}
#if ACL_EWOULDBLOCK == ACL_EAGAIN
	else if (in->errnum != ACL_EWOULDBLOCK)
#else
	else if (in->errnum != ACL_EWOULDBLOCK && in->errnum != ACL_EAGAIN)
#endif
	{
		in->flag |= ACL_VSTREAM_FLAG_ERR;
		acl_strerror(in->errnum, in->errbuf, sizeof(in->errbuf));
	}

	return -1;
}

static int read_to_buffer(ACL_VSTREAM *fp, void *buf, size_t size)
{
	int n = sys_read(fp, buf, size);

	if (n <= 0)
		return -1;
	return n;
}

static int read_buffed(ACL_VSTREAM *fp)
{
	int  n;

	fp->read_ptr = fp->read_buf;
	n =  read_to_buffer(fp, fp->read_buf, (size_t) fp->read_buf_len);
	if (n >= 0)
		fp->read_cnt = n;
	else
		fp->read_cnt = 0;
	return n;
}

static int read_char(ACL_VSTREAM *fp)
{
	int n = read_buffed(fp);

	if (n >= 0)
		fp->read_cnt = n;
	else
		fp->read_cnt = 0;
	if (n <= 0)
		return ACL_VSTREAM_EOF;
	else
		return ACL_VSTREAM_GETC(fp);
}

int acl_vstream_getc(ACL_VSTREAM *fp)
{
	if (fp == NULL)
		return ACL_VSTREAM_EOF;
	if (fp->read_cnt <= 0 && read_buffed(fp) <= 0)
		return ACL_VSTREAM_EOF;

	fp->read_cnt--;
	fp->offset++;
	return *fp->read_ptr++;
}

int acl_vstream_nonb_readn(ACL_VSTREAM *fp, char *buf, int size)
{
	const char *myname = "acl_vstream_nonb_readn";
	int   n, nread, read_cnt;
	unsigned char *ptr;
	int   rw_timeout;
#ifdef	ACL_UNIX
	int   flags;
#endif

	if (fp == NULL || buf == NULL || size <= 0) {
		acl_msg_error("%s(%d), %s: fp %s, buf %s, size %d",
			__FILE__, __LINE__, myname, fp ? "not null" : "null",
			buf ? "not null" : "null", size);
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(%d) < 0, fd(%d)",
			myname, __FILE__, __LINE__,
			(int) fp->read_cnt, ACL_VSTREAM_SOCK(fp));
		return ACL_VSTREAM_EOF;
	}

	ptr = (unsigned char *) buf;
	nread = 0;

	if (fp->read_cnt > 0) {
		n = size > (int) fp->read_cnt ? (int) fp->read_cnt : size;
		read_cnt = acl_vstream_bfcp_some(fp, ptr, n);
		if (read_cnt <= 0) {
			acl_msg_error("%s, %s(%d): error, read_cnt=%d",
				myname, __FILE__, __LINE__, read_cnt);
			return ACL_VSTREAM_EOF;
		}
		size  -= read_cnt;
		ptr   += read_cnt;
		nread += read_cnt;
		if (size == 0)
			return read_cnt;
		else if (size < 0) {
			acl_msg_error("%s, %s(%d): error, size = %d",
				myname, __FILE__, __LINE__, size);
			return ACL_VSTREAM_EOF;
		}
	}

#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(fp), F_GETFL, 0);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			myname, __FILE__, __LINE__, acl_last_serror(),
			ACL_VSTREAM_SOCK(fp));
		return ACL_VSTREAM_EOF;
	}
	acl_non_blocking(ACL_VSTREAM_SOCK(fp), 1);
#elif defined(ACL_WINDOWS)
	if (fp->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(fp), 1);
#endif

	/* 先保留读写超时时间值，并将该流的超时值置为0，以免
	 * 启动读超时过程(select)。
	 */
	rw_timeout = fp->rw_timeout;
	fp->rw_timeout = 0;
	fp->errnum = 0;

	read_cnt = read_buffed(fp);

	fp->rw_timeout = rw_timeout;

	/* 恢复该套接字的原有标记位 */
#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(fp), F_SETFL, flags);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			myname, __FILE__, __LINE__,
			acl_last_serror(), ACL_VSTREAM_SOCK(fp));
		return ACL_VSTREAM_EOF;
	}

#elif	defined(ACL_WINDOWS)
	if (fp->is_nonblock == 0 && fp->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(fp), ACL_BLOCKING);
#endif

	if (read_cnt < 0) {
#ifdef	ACL_WINDOWS
		if (fp->errnum == ACL_EWOULDBLOCK)
#elif	defined(ACL_UNIX)
		if (fp->errnum == ACL_EWOULDBLOCK
			|| fp->errnum == ACL_EAGAIN)
#endif
		{
			return nread;
		}
		else if (nread == 0)
			return ACL_VSTREAM_EOF;
		else
			return nread;
	} else if (read_cnt == 0) {
		if (nread == 0)
			return ACL_VSTREAM_EOF;
		else
			return nread;
	}

	if (fp->read_cnt > 0) {
		n = size > (int) fp->read_cnt ? (int) fp->read_cnt : size;
		read_cnt = acl_vstream_bfcp_some(fp, ptr, n);
		if (read_cnt <= 0) {
			acl_msg_error("%s, %s(%d): error, read_cnt=%d",
				myname, __FILE__, __LINE__, read_cnt);
			return ACL_VSTREAM_EOF;
		}

		nread += read_cnt;
	}

	fp->rw_timeout = rw_timeout;

	return nread;
}

int acl_vstream_probe_status(ACL_VSTREAM *fp)
{
#ifdef	ACL_UNIX
	int   flags;
#endif
	int   ch;
	int   rw_timeout;

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(fp), F_GETFL, 0);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			__FUNCTION__, __FILE__, __LINE__,
			acl_last_serror(), ACL_VSTREAM_SOCK(fp));
		return -1;
	}
	acl_non_blocking(ACL_VSTREAM_SOCK(fp), 1);
#elif defined(ACL_WINDOWS)
	if (fp->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(fp), 1);
#endif

	rw_timeout = fp->rw_timeout;

	/* 先保留读写超时时间值，并将该流的超时值置为0，以免
	 * 启动读超时过程(select)。
	 */
	fp->rw_timeout = 0;
	fp->errnum = 0;

	ch = acl_vstream_getc(fp);

	fp->rw_timeout = rw_timeout;

	/* 恢复该套接字的原有标记位 */
#ifdef	ACL_UNIX
	flags = fcntl(ACL_VSTREAM_SOCK(fp), F_SETFL, flags);
	if (flags < 0) {
		acl_msg_error("%s, %s(%d): fcntl error(%s), fd=%d",
			__FUNCTION__, __FILE__, __LINE__,
			acl_last_serror(), ACL_VSTREAM_SOCK(fp));
		return -1;
	}

#elif	defined(ACL_WINDOWS)
	if (fp->is_nonblock == 0 && fp->type != ACL_VSTREAM_TYPE_FILE)
		acl_non_blocking(ACL_VSTREAM_SOCK(fp), ACL_BLOCKING);
#endif

	if (ch == ACL_VSTREAM_EOF) {
#ifdef	ACL_WINDOWS
		if (fp->errnum == ACL_EWOULDBLOCK)
#elif	defined(ACL_UNIX)
		if (fp->errnum == ACL_EWOULDBLOCK || fp->errnum == ACL_EAGAIN)
#endif
			return 0;
		else
			return -1;
	} else {
		/* 将读到的数据再放回原处:) */
		fp->read_cnt++;
		fp->read_ptr--;
		fp->offset--;
		if (fp->read_ptr < fp->read_buf)
			return -1;
		return 0;
	}
}

int acl_vstream_ungetc(ACL_VSTREAM *fp, int ch)
{
	unsigned char c;

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	c = (unsigned char) ch;
	(void) acl_vstream_unread(fp, &c, 1);
	return ch;
}

static void *__vstream_memmove(ACL_VSTREAM *fp, size_t n)
{
#if 1
	char *src, *dst, *dst_saved;

	if (fp->read_cnt == 0)
		return fp->read_buf;

	src = (char*) fp->read_ptr + fp->read_cnt - 1;
	dst_saved = dst = (char*) fp->read_ptr + n + fp->read_cnt - 1;

	/* 为了防止内存数据覆盖问题, 采用数据从尾部拷贝方式 */
	while (src >= (char*) fp->read_ptr)
		*dst-- = *src--;
	return dst_saved;
#else
	return memmove((char*) fp->read_ptr + n,
			fp->read_ptr, fp->read_cnt);
#endif
}

int acl_vstream_unread(ACL_VSTREAM *fp, const void *ptr, size_t length)
{
	size_t capacity;
	ssize_t k;

	if (fp == NULL || ptr == NULL || length == 0) {
		acl_msg_error("%s(%d), %s: fp %s, ptr %s, length %d",
			__FILE__, __LINE__, __FUNCTION__, fp ? "not null"
			: "null", ptr ? "not null" : "null", (int) length);
		return -1;
	}

	capacity = fp->read_ptr - fp->read_buf;
	k = (ssize_t) (capacity - length);

	/* 如果读缓冲中前部分空间不足, 则需要调整数据位置或扩充读缓冲区空间 */

	if (k < 0) {
		void *pbuf;
		size_t n, min_delta = 4096;

		n = (size_t) -k;

		/* 如果读缓冲区后部分空间够用, 则只需后移缓冲区中的数据 */

		if (fp->read_buf_len - fp->read_cnt > (acl_off_t) length) {
			if (fp->read_cnt > 0)
				__vstream_memmove(fp, n);

			memcpy(fp->read_buf, ptr, length);
			fp->read_ptr = fp->read_buf;
			fp->read_cnt += (int) length;
			fp->offset = 0;
			return (int) length;
		}

		/* 说明整个缓冲区的空间都不够用, 所以需要扩充缓冲区空间 */

		n = min_delta * ((n + min_delta - 1) / min_delta);
		acl_assert(n > 0);
		fp->read_buf_len += (int) n;
		pbuf = acl_mymalloc((size_t) fp->read_buf_len);

		memcpy(pbuf, ptr, length);
		if (fp->read_cnt > 0)
			memcpy((char*) pbuf + length, fp->read_ptr,
				(size_t) fp->read_cnt);
		acl_myfree(fp->read_buf);

		fp->read_buf = pbuf;
		fp->read_ptr = fp->read_buf;
		fp->read_cnt += (int) length;
		fp->offset = 0;
		return (int) length;
	}

	fp->read_ptr -= length;
	memcpy(fp->read_ptr, ptr, length);
	fp->read_cnt += (int) length;
	fp->offset -= length;
	return (int) length;
}

int acl_vstream_bfcp_some(ACL_VSTREAM *fp, void *vptr, size_t maxlen)
{
	const char *myname = "acl_vstream_bfcp_some";
	int   n;

	/* input params error */
	if (fp == NULL || vptr == NULL || maxlen == 0) {
		acl_msg_error("%s, %s(%d): input error, fp %s, vptr %s, "
			"maxlen %d", myname, __FILE__, __LINE__, fp ?
			"not null" : "null", vptr ? "not null" : "null",
			(int) maxlen);
		return ACL_VSTREAM_EOF;
	}

	/* internal fatal error */
	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	/* there is no any data in buf */
	if (fp->read_cnt == 0) {
		fp->read_ptr = fp->read_buf;
		return 0;
	}

	if (fp->read_ptr >= fp->read_buf + (int) fp->read_buf_len) {
		fp->read_cnt = 0;
		fp->read_ptr = fp->read_buf;
		return 0;
	}

	n = (int) fp->read_cnt > (int) maxlen
		? (int) maxlen : (int) fp->read_cnt;

	memcpy(vptr, fp->read_ptr, n);

	fp->read_cnt -= n;
	fp->read_ptr += n;
	fp->offset += n;

	return n;
}

int acl_vstream_readtags(ACL_VSTREAM *fp, void *vptr, size_t maxlen,
	const char *tag, size_t taglen)
{
	const char *myname = "acl_vstream_readtags";
	int   n, ch, matched = 0;
	unsigned char *ptr;
	const unsigned char *haystack;
	const unsigned char *needle, *needle_end;

	if (fp == NULL || vptr == NULL || maxlen <= 0
	    || tag == NULL || taglen <= 0)
	{
		acl_msg_error("%s(%d), %s: fp %s, vptr %s, maxlen %d, tag %s,"
			" taglen: %d", __FILE__, __LINE__, myname,
			fp ? "not null" : "null", vptr ? "not null" : "null",
			(int) maxlen, tag ? tag : "null", (int) taglen);
		return ACL_VSTREAM_EOF;
	}

	fp->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	needle_end = (const unsigned char *) tag;

	while(1) {
		taglen--;
		if (taglen == 0)
			break;
		needle_end++;
	}
	ptr = (unsigned char *) vptr;

	for (n = 1; n < (int) maxlen; n++) {
		/* left one byte for '\0' */

#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(fp);
#else
		ch = acl_vstream_getc(fp);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 1)  /* EOF, nodata read */
				return ACL_VSTREAM_EOF;

			break;  /* EOF, some data was read */
		}

		*ptr = ch;
		if (ch == *needle_end) {
			haystack = ptr - 1;
			needle = needle_end - 1;
			matched = 0;
			while(1) {
				/* 已经成功比较完毕(匹配) */
				if (needle < (const unsigned char *) tag) {
					matched = 1;
					break;
				}

				/* 原字符串用完而匹配串还没有比较完(不匹配) */
				if (haystack < (unsigned char *) vptr)
					break;
				/* 不相等(不匹配) */
				if (*haystack != *needle)
					break;
				haystack--;
				needle--;
			}
		}
		ptr++;
		if (matched) {
			fp->flag |= ACL_VSTREAM_FLAG_TAGYES;
			break;
		}
	}

	*ptr = 0;  /* null terminate like fgets() */

	return n;
}

int acl_vstream_gets(ACL_VSTREAM *fp, void *vptr, size_t maxlen)
{
	const char *myname = "acl_vstream_gets";
	int   n, ch;
	unsigned char *ptr;

	if (fp == NULL || vptr == NULL || maxlen <= 0) {
		acl_msg_error("%s(%d), %s: fp %s, vptr %s, maxlen %d",
			__FILE__, __LINE__, myname, fp ? "not null" : "null",
			vptr ? "not null" : "null", (int) maxlen);
		return ACL_VSTREAM_EOF;
	}

	fp->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	ptr = (unsigned char *) vptr;
	for (n = 1; n < (int) maxlen; n++) {
		/* left one byte for '\0' */

#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(fp);
#else
		ch = acl_vstream_getc(fp);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 1)  /* EOF, nodata read */
				return ACL_VSTREAM_EOF;

			break;  /* EOF, some data was read */
		}

		*ptr++ = ch;
		if (ch == '\n'){
			/* newline is stored, like fgets() */

			fp->flag |= ACL_VSTREAM_FLAG_TAGYES;
			break;
		}
	}

	/* null terminate like fgets() */
	*ptr = 0;

	return n;
}

int acl_vstream_gets_nonl(ACL_VSTREAM *fp, void *vptr, size_t maxlen)
{
	const char *myname = "acl_vstream_gets_nonl";
	int   n, ch;
	unsigned char *ptr;

	if (fp == NULL || vptr == NULL || maxlen <= 0) {
		acl_msg_error("%s(%d), %s: fp %s, vptr %s, maxlen %d",
			__FILE__, __LINE__, myname, fp ? "not null" : "null",
			vptr ? "not null" : "null", (int) maxlen);
		return ACL_VSTREAM_EOF;
	}

	fp->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	ptr = (unsigned char *) vptr;
	for (n = 1; n < (int) maxlen; n++) {
#ifdef	_USE_FAST_MACRO
		ch = ACL_VSTREAM_GETC(fp);
#else
		ch = acl_vstream_getc(fp);
#endif
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 1)  /* EOF, nodata read */
				return ACL_VSTREAM_EOF;

			break;  /* EOF, some data was read */
		}

		*ptr++ = ch;
		if (ch == '\n') {
			fp->flag |= ACL_VSTREAM_FLAG_TAGYES;
			break;  /* newline is stored, like fgets() */
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

int acl_vstream_readn(ACL_VSTREAM *fp, void *buf, size_t size)
{
	const char *myname = "acl_vstream_readn";
	size_t  size_saved = size;
	unsigned char *ptr;
	int   n;

	if (fp == NULL || buf == NULL || size == 0) {
		acl_msg_error("%s(%d): fp %s, buf %s, size %d",
			myname, __LINE__, fp ? "not null" : "null",
			buf ? "not null" : "null", (int) size);
		return ACL_VSTREAM_EOF;
	}

	ptr = (unsigned char*) buf;

	/* 如果缓冲区中有上次读残留数据时，则优先将其拷贝至目标缓冲区 */

	if (fp->read_cnt > 0) {
		n = acl_vstream_bfcp_some(fp, ptr, size);
		ptr += n;
		size -= n;
		if (size == 0)
			return (int) size_saved;
	}

	/* 为减少 read 次数，当输入缓冲区较小时，则自动启用双缓冲读方式 */

	if (size_saved  < (size_t) fp->read_buf_len / 4) {
		while (size > 0) {
			if (read_buffed(fp) <= 0)
				return ACL_VSTREAM_EOF;
			n = acl_vstream_bfcp_some(fp, ptr, size);
			ptr += n;
			size -= n;
		}
	}

	/* 否则，则直接将读到的数据存入缓冲区，从而避免大数据的二次拷贝 */
	else {
		while (size > 0) {
			n = read_to_buffer(fp, ptr, size);
			if (n <= 0)
				return ACL_VSTREAM_EOF;
			size -= n;
			ptr += n;
		}
	}

	return (int) size_saved;
}

int acl_vstream_read(ACL_VSTREAM *fp, void *buf, size_t size)
{
	const char *myname = "acl_vstream_read";

	if (fp == NULL || buf == NULL || size == 0) {
		acl_msg_error("%s(%d): fp: %s, buf: %s, size: %d",
			myname, __LINE__, fp ? "not null" : "null",
			buf ? "not null" : "null", (int) size);

		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt > 0)
		return acl_vstream_bfcp_some(fp, (unsigned char*) buf, size);

	/* fp->read_cnt == 0 */

	/* 当缓冲区较大时，则直接将数据读到该缓冲区从而避免大数据拷贝 */
	if (size >= (size_t) fp->read_buf_len / 4) {
		int n = read_to_buffer(fp, buf, size);
		return n <= 0 ? ACL_VSTREAM_EOF : n;
	}
	/* 否则将数据读到流缓冲区中，然后再拷贝，从而减少 read 次数 */
	else {
		int   read_cnt = read_buffed(fp);
		if (read_cnt <= 0)
			return ACL_VSTREAM_EOF;
		return acl_vstream_bfcp_some(fp, (unsigned char*) buf, size);
	}
}

static int bfgets_crlf_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready)
{
	const char *myname = "bfgets_crlf_peek";
	int   ch = 0;

	if (fp->read_cnt <= 0)   /* XXX: sanity check */
		return 0;

	while (fp->read_cnt > 0) {
		ch = *(fp->read_ptr);
		ACL_VSTRING_ADDCH(buf, ch);

		fp->read_ptr++;
		fp->read_cnt--;
		fp->offset++;

		/* when get '\n', set ready 1 */
		if (ch == '\n') {
			*ready = 1;
			fp->flag |= ACL_VSTREAM_FLAG_TAGYES;

			break;
		}

		/* when reached the max limit, set ready 1 */
		if (buf->maxlen > 0 && (int) LEN(buf) >= buf->maxlen) {
			*ready = 1;
			acl_msg_warn("%s(%d), %s: line too long: %d, %d",
				__FILE__, __LINE__, myname,
				(int) buf->maxlen, (int) LEN(buf));
			break;
		}
	}

	/* set '\0' teminated */
	ACL_VSTRING_TERMINATE(buf);

	return ch;
}

int acl_vstream_gets_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready)
{
	const char *myname = "acl_vstream_gets_peek";
	int   n;

	if (fp == NULL || buf == NULL || ready == NULL) {
		acl_msg_error("%s, %s(%d): fp %s, buf %s, ready: %s", myname,
			__FILE__, __LINE__, fp ? "not null" : "null", buf ?
			"not null" : "null", ready ? "not null" : "null");
		return ACL_VSTREAM_EOF;
	}

	fp->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	*ready = 0;
	n = (int) LEN(buf);

	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt > 0) {
		bfgets_crlf_peek(fp, buf, ready);
		if (*ready)
			return (int) LEN(buf) - n;
	}

	/* XXX: 调用者通过检查 *ready 值来判断是否读够数据, 系统IO读操作出错
	 * 或关闭时返回结束标记; 如果返回 ACL_VSTRING_EOF 则调用者应该通过
	 * 检查缓冲区长度来处理未被处理的数据
	 */

	if (fp->read_ready) {
		if (read_buffed(fp) <= 0) {
			n = (int) LEN(buf) - n;
			return n >= 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (fp->read_cnt > 0)
		bfgets_crlf_peek(fp, buf, ready);
	return (int) LEN(buf) - n;
}

static int bfgets_no_crlf_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready)
{
	int   ch = bfgets_crlf_peek(fp, buf, ready);

	if (ch == 0)
		return ch;

	if (ch == '\n') {
		int   n = (int) LEN(buf) - 1;

		while (n >= 0) {
			ch = acl_vstring_charat(buf, n);
			if (ch == '\r' || ch == '\n')
				n--;
			else
				break;
		}

		/* reset the offset position */
		acl_vstring_truncate(buf, n + 1);

		/* must be '\0' teminated */
		ACL_VSTRING_TERMINATE(buf);
	}

	return ch;
}

int acl_vstream_gets_nonl_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf, int *ready)
{
	const char *myname = "acl_vstream_gets_nonl_peek";
	int   n;

	if (fp == NULL || buf == NULL || ready == NULL) {
		acl_msg_fatal("%s, %s(%d): fp %s, buf %s, ready: %s", myname,
			__FILE__, __LINE__, fp ? "not null" : "null", buf ?
			"not null" : "null", ready ? "not null" : "null");
		return ACL_VSTREAM_EOF;
	}

	fp->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	*ready = 0;
	n = (int) LEN(buf);

	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt > 0) {
		bfgets_no_crlf_peek(fp, buf, ready);
		if (*ready)
			return (int) LEN(buf) - n;
	}

	/* XXX: 调用者通过检查 *ready 值来判断是否读够数据, 系统IO读操作出错
	 * 或关闭时返回结束标记; 如果返回 ACL_VSTRING_EOF 则调用者应该通过
	 * 检查缓冲区长度来处理未被处理的数据
	 */

	if (fp->read_ready) {
		if (read_buffed(fp) <= 0) {
			n = (int) LEN(buf) - n;

			return n >= 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (fp->read_cnt > 0)
		bfgets_no_crlf_peek(fp, buf, ready);

	return (int) LEN(buf) - n;
}

static int bfread_cnt_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf,
	int cnt, int *ready)
{
	int   n;

	/* XXX: sanity check */
	if (fp->read_cnt <= 0)
		return 0;
	n = (int) (fp->read_cnt > cnt ? cnt : fp->read_cnt);
	acl_vstring_memcat(buf, (char*) fp->read_ptr, n);
	fp->read_cnt -= n;
	fp->offset += n;
	fp->read_ptr += n;
	cnt -= n;
	if (cnt == 0)
		*ready = 1;

	ACL_VSTRING_TERMINATE(buf);  /* set '\0' teminated */
	return n;
}

int acl_vstream_readn_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf,
	int cnt, int *ready)
{
	const char *myname = "acl_vstream_readn_peek";
	int   cnt_saved = cnt;

	if (fp == NULL || buf == NULL || cnt <= 0 || ready == NULL)
		acl_msg_fatal("%s, %s(%d): invalid input, fp: %s, buf: %s, "
			"cnt: %d, ready: %s", myname, __FILE__, __LINE__, fp ?
			"not null" : "null", buf ? "not null" : "null", cnt,
			ready ? "not null" : "null");

	*ready = 0;
	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt > 0) {
		cnt -= bfread_cnt_peek(fp, buf, cnt, ready);
		if (*ready)
			return cnt_saved - cnt;
	}

	/* XXX: 调用者通过检查 *ready 值来判断是否读够数据, 系统IO读操作出错
	 * 或关闭时返回结束标记, 如果返回 ACL_VSTRING_EOF 则调用者应该通过
	 * 检查缓冲区长度来处理未被处理的数据
	 */

	if (fp->read_ready) {
		if (read_buffed(fp) <= 0) {
			int   n = cnt_saved - cnt;
			return n >= 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (fp->read_cnt > 0)
		cnt -= bfread_cnt_peek(fp, buf, cnt, ready);
	return cnt_saved - cnt;
}

static void bfread_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf)
{
	/* XXX: sanity check */
	if (fp->read_cnt <= 0)
		return;
	acl_vstring_memcat(buf, (char*) fp->read_ptr, (size_t) fp->read_cnt);
	fp->offset += fp->read_cnt;
	fp->read_ptr += fp->read_cnt;
	fp->read_cnt = 0;

	/* set '\0' teminated */
	ACL_VSTRING_TERMINATE(buf);
}

int acl_vstream_read_peek(ACL_VSTREAM *fp, ACL_VSTRING *buf)
{
	const char *myname = "acl_vstream_read_peek";
	int   n;

	if (fp == NULL || buf == NULL) {
		acl_msg_error("%s, %s(%d): fp %s, buf %s", myname, __FILE__,
			__LINE__, fp ? "not null" : "null",
			buf ? "not null" : "null");
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	n = (int) LEN(buf);

	if (fp->read_cnt > 0)
		bfread_peek(fp, buf);

	/* 系统IO读操作出错或关闭时返回结束标记, 如果返回 ACL_VSTRING_EOF
	 * 则调用者应该通过检查缓冲区长度来处理未被处理的数据
	 */

	if (fp->read_ready) {
		if (read_buffed(fp) <= 0) {
			n = (int) LEN(buf) - n;
			return n >= 0 ? n : ACL_VSTREAM_EOF;
		}
	}

	if (fp->read_cnt > 0)
		bfread_peek(fp, buf);
	return (int) LEN(buf) - n;
}

int acl_vstream_can_read(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_can_read";

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, myname);
		return ACL_VSTREAM_EOF;
	}

	if (fp->read_cnt < 0) {
		acl_msg_error("%s, %s(%d): read_cnt(=%d) < 0",
			myname, __FILE__, __LINE__, (int) fp->read_cnt);
		return ACL_VSTREAM_EOF;
	}

	if (fp->flag & (ACL_VSTREAM_FLAG_ERR | ACL_VSTREAM_FLAG_EOF))
		return ACL_VSTREAM_EOF;
	else if (fp->read_cnt > 0)
		return 1;
	else if (fp->read_ready == 0)
		return 0;
	else if ((fp->flag & ACL_VSTREAM_FLAG_PREREAD) != 0) {
		if (read_buffed(fp) <= 0)
			return ACL_VSTREAM_EOF;
		else
			return 1;
	}
	else
		return 1;
}

static int write_once(ACL_VSTREAM *fp, const void *vptr, int dlen)
{
	const char *myname = "write_once";
	int   n, neintr = 0;

	if (vptr == NULL || dlen <= 0) {
		if (vptr == NULL)
			acl_msg_error("%s, %s(%d): vptr null",
				myname, __FILE__, __LINE__);
		if (dlen <= 0)
			acl_msg_error("%s, %s(%d): dlen(%d) <= 0",
				myname, __FILE__, __LINE__, dlen);
		return ACL_VSTREAM_EOF;
	}

	if (fp->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(fp) == ACL_FILE_INVALID) {
			acl_msg_error("%s, %s(%d): h_file invalid",
				myname, __FILE__, __LINE__);
			fp->errnum = ACL_EINVAL;
			return ACL_VSTREAM_EOF;
		}
	} else if (ACL_VSTREAM_SOCK(fp) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s, %s(%d): sockfd invalid",
			myname, __FILE__, __LINE__);
		fp->errnum = ACL_EINVAL;
		return ACL_VSTREAM_EOF;
	}

TAG_AGAIN:

	/* 清除系统错误号 */
	acl_set_error(0);

	if (fp->type == ACL_VSTREAM_TYPE_FILE) {
		if ((fp->oflags & O_APPEND)) {
#ifdef ACL_WINDOWS
			fp->sys_offset = acl_lseek(
				ACL_VSTREAM_FILE(fp), 0, SEEK_END);
			if (fp->sys_offset < 0) {
				fp->errnum = acl_last_error();
				fp->flag |= ACL_VSTREAM_FLAG_ERR;
				return ACL_VSTREAM_EOF;
			}
#endif
		} else if ((fp->flag & ACL_VSTREAM_FLAG_CACHE_SEEK)
			&& fp->offset != fp->sys_offset)
		{

			fp->sys_offset = acl_lseek(ACL_VSTREAM_FILE(fp),
				fp->offset, SEEK_SET);
			if (fp->sys_offset == -1) {
				acl_msg_error("%s, %s(%d): lseek error(%s), "
					"offset(" ACL_FMT_I64D "), sys_offset("
					ACL_FMT_I64D ")", myname, __FILE__,
					__LINE__, acl_last_serror(),
					fp->offset, fp->sys_offset);

				fp->errnum = acl_last_error();
				fp->flag |= ACL_VSTREAM_FLAG_ERR;
				return ACL_VSTREAM_EOF;
			}
			fp->offset = fp->sys_offset;
		}

		n = fp->fwrite_fn(ACL_VSTREAM_FILE(fp), vptr, dlen,
			fp->rw_timeout, fp, fp->context);
		if (n > 0) {
			fp->sys_offset += n;
			fp->offset = fp->sys_offset;

			/* 防止缓冲区内的数据与实际不一致, 仅对文件IO有效 */
			fp->read_cnt = 0;
		}
	} else {
		n = fp->write_fn(ACL_VSTREAM_SOCK(fp), vptr, dlen,
			fp->rw_timeout, fp, fp->context);
	}

	if (n > 0) {
		fp->total_write_cnt += n;
		return n;
	}

	fp->errnum = acl_last_error();

	if (fp->errnum == ACL_EINTR) {
		if (++neintr >= 5) {
			fp->flag |= ACL_VSTREAM_FLAG_ERR;
			return ACL_VSTREAM_EOF;
		}

		goto TAG_AGAIN;
	}

#if ACL_EAGAIN == ACL_EWOULDBLOCK
	if (fp->errnum == ACL_EWOULDBLOCK)
#else
	if (fp->errnum == ACL_EAGAIN || fp->errnum == ACL_EWOULDBLOCK)
#endif
		acl_set_error(ACL_EAGAIN);
	else if (fp->errnum == ACL_ETIMEDOUT) {
		fp->flag |= ACL_VSTREAM_FLAG_TIMEOUT;
		SAFE_COPY(fp->errbuf, "write timeout");
	} else
		fp->flag |= ACL_VSTREAM_FLAG_ERR;

	return ACL_VSTREAM_EOF;
}

static int writev_once(ACL_VSTREAM *fp, const struct iovec *vec, int count)
{
	const char *myname = "writev_once";
	int   n = 0, neintr = 0;

	if (vec == NULL || count <= 0) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (fp->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(fp) == ACL_FILE_INVALID) {
			acl_msg_error("%s, %s(%d): h_file invalid",
				myname, __FILE__, __LINE__);
			fp->errnum = ACL_EINVAL;
			return ACL_VSTREAM_EOF;
		}
	} else if (ACL_VSTREAM_SOCK(fp) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s, %s(%d): sockfd invalid",
			myname, __FILE__, __LINE__);
		fp->errnum = ACL_EINVAL;
		return ACL_VSTREAM_EOF;
	}

TAG_AGAIN:

	if (fp->type == ACL_VSTREAM_TYPE_FILE) {
		if ((fp->oflags & O_APPEND)) {
#ifdef ACL_WINDOWS
			fp->sys_offset = acl_lseek(
				ACL_VSTREAM_FILE(fp), 0, SEEK_END);
			if (fp->sys_offset < 0) {
				fp->errnum = acl_last_error();
				fp->flag |= ACL_VSTREAM_FLAG_ERR;
				return ACL_VSTREAM_EOF;
			}
#endif
		} else if ((fp->flag & ACL_VSTREAM_FLAG_CACHE_SEEK)
			&& fp->offset != fp->sys_offset)
		{
			fp->sys_offset = acl_lseek(ACL_VSTREAM_FILE(fp),
					fp->offset, SEEK_SET);
			if (fp->sys_offset == -1) {
				acl_msg_error("%s, %s(%d): lseek error(%s), "
					"offset(" ACL_FMT_I64D "), sys_offset("
					ACL_FMT_I64D ")", myname, __FILE__,
					__LINE__, acl_last_serror(),
					fp->offset, fp->sys_offset);

				fp->errnum = acl_last_error();
				fp->flag |= ACL_VSTREAM_FLAG_ERR;
				return ACL_VSTREAM_EOF;
			}
		}

		if (fp->fwrite_fn == acl_file_write) {
			n = fp->fwritev_fn(ACL_VSTREAM_FILE(fp), vec, count,
				fp->rw_timeout, fp, fp->context);
		} else {
			int i, ret;

			for (i = 0; i < count; i++) {
				ret = write_once(fp, vec[i].iov_base,
					(int) vec[i].iov_len);
				if (ret == ACL_VSTREAM_EOF)
					return ret;
				n += ret;
				if (ret < (int) vec[i].iov_len)
					break;
			}
		}

		if (n > 0) {
			fp->sys_offset += n;
			fp->offset = fp->sys_offset;

			/* 防止缓冲区内的数据与实际不一致, 仅对文件IO有效 */
			fp->read_cnt = 0;
		}
	}

	/* 当写接口函数指针为系统默认的接口时，直接写入 */
	else if (fp->write_fn == acl_socket_write) {
		n = fp->writev_fn(ACL_VSTREAM_SOCK(fp), vec, count,
			fp->rw_timeout, fp, fp->context);
	}

	/* 否则，则模拟 writev 的调用过程 */
	else {
		int i, ret;

		for (i = 0; i < count; i++) {
			ret = write_once(fp, vec[i].iov_base,
				(int) vec[i].iov_len);
			if (ret == ACL_VSTREAM_EOF)
				return ret;
			n += ret;
			if (ret < (int) vec[i].iov_len)
				break;
		}
	}

	if (n >= 0) {
		fp->total_write_cnt += n;
		return n;
	}

	fp->errnum = acl_last_error();

	if (fp->errnum == ACL_EINTR) {
		if (++neintr >= 5)
			return ACL_VSTREAM_EOF;

		goto TAG_AGAIN;
	}

#if ACL_EAGAIN == ACL_EWOULDBLOCK
	if (fp->errnum == ACL_EAGAIN)
#else
	if (fp->errnum == ACL_EAGAIN || fp->errnum == ACL_EWOULDBLOCK)
#endif
		acl_set_error(ACL_EAGAIN);
	else
		fp->flag |= ACL_VSTREAM_FLAG_ERR;

	return ACL_VSTREAM_EOF;
}

int acl_vstream_write(ACL_VSTREAM *fp, const void *vptr, int dlen)
{
	const char *myname = "acl_vstream_write";

	if (fp == NULL || vptr == NULL || dlen <= 0) {
		acl_msg_error("%s(%d), %s: fp %s, vptr %s, dlen %d", __FILE__,
			__LINE__, myname, fp ? "not null" : "null",
			vptr ? "not null" : "null", dlen);
		return ACL_VSTREAM_EOF;
	}

	if (fp->wbuf_dlen > 0) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	return write_once(fp, vptr, dlen);
}

int acl_vstream_writev(ACL_VSTREAM *fp, const struct iovec *vec, int count)
{
	const char *myname = "acl_vstream_writev";

	if (fp == NULL || vec == NULL || count <= 0) {
		acl_msg_error("%s(%d), %s: fp %s, vec %s, count %d", __FILE__,
			__LINE__, myname, fp ? "not null" : "null",
			vec ? "not null" : "null", count);
		return ACL_VSTREAM_EOF;
	}

	if (fp->wbuf_dlen > 0) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	return writev_once(fp, vec, count);
}

int acl_vstream_writevn(ACL_VSTREAM *fp, const struct iovec *vec, int count)
{
	const char *myname = "acl_vstream_writevn";
	int   n, i, nskip, nwrite = 0;
	struct iovec *iv, *iv_saved;

	if (fp == NULL || count <= 0 || vec == NULL) {
		acl_msg_error("%s, %s(%d): fp %s, vec %s, count %d", myname,
			__FILE__, __LINE__, fp ? "not null" : "null",
			vec ? "not null" : "null", count);
		return ACL_VSTREAM_EOF;
	}

	if (fp->wbuf_dlen > 0) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}

	iv = (struct iovec*) acl_mycalloc(count, sizeof(struct iovec));
	iv_saved = iv;  /* saving the memory for freeing */

	for (i = 0; i < count; i++) {
		iv[i].iov_base = vec[i].iov_base;
		iv[i].iov_len = vec[i].iov_len;
	}

	while (1) {
		n = writev_once(fp, iv, count);
		if (n == ACL_VSTREAM_EOF) {
			acl_myfree(iv_saved);
			return ACL_VSTREAM_EOF;
		} else if (n == 0)
			continue;

		nwrite += n;
		nskip   = 0;

		for (i = 0; i < count; i++) {
			if (n >= (int) iv[i].iov_len) {
				/* fully written one vector item */
				n -= (int) iv[i].iov_len;
				nskip++;
			} else {
				/* partially written */
				iv[i].iov_base = (void *) ((unsigned char*)
					iv[i].iov_base + n);
				iv[i].iov_len -= n;
				break;
			}
		}

		if (i >= count) {
			acl_myfree(iv_saved);
			return nwrite;
		}

		count -= nskip;
		iv    += nskip;
	}
}

int acl_vstream_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap)
{
	const char *myname = "acl_vstream_vfprintf";
	ACL_VSTRING *buf;
	int   n;

	if (fp == NULL || fmt == NULL || *fmt == 0) {
		acl_msg_error("%s, %s(%d): fp %s, fmt %s",
			myname, __FILE__, __LINE__, fp ? "not null" : "null",
			fmt && *fmt ? "not null" : "null");
		return ACL_VSTREAM_EOF;
	}

	buf = acl_vstring_alloc(ACL_VSTREAM_BUFSIZE);
	if (buf == NULL) {
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
	}

	if (acl_vstring_vsprintf(buf, fmt, ap) == NULL)
		acl_msg_fatal("%s, %s(%d): vsprintf return null",
			myname, __FILE__, __LINE__);

	n = (int) LEN(buf);
	if (n <= 0)
		acl_msg_fatal("%s, %s(%d): len(%d) <= 0",
			myname, __FILE__, __LINE__, n);

	n = acl_vstream_writen(fp, STR(buf), n);

	acl_vstring_free(buf);
	return n;
}

int acl_vstream_fprintf(ACL_VSTREAM *fp, const char *fmt, ...)
{
	const char *myname = "acl_vstream_fprintf";
	va_list ap;
	int   n;

	if (fp == NULL || fmt == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	va_start(ap, fmt);
	n = acl_vstream_vfprintf(fp, fmt, ap);
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
		return ACL_VSTREAM_EOF;
	}

	return acl_vstream_fputs(s, ACL_VSTREAM_OUT);
}

static int loop_writen(ACL_VSTREAM *fp, const void *vptr, size_t size)
{
#if 0
	const char *myname = "loop_writen";
#endif
	const unsigned char *ptr = (const unsigned char *) vptr;
	int   once_dlen = 64 * 1024 * 1024;  /* xxx: 以 64KB 为单位写 */
	int   nleft = (int) size, n, len;
#if 0
	time_t begin, end;
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(fp);
#endif

	while (nleft > 0) {
		len = nleft > once_dlen ? once_dlen : nleft;
		n = write_once(fp, ptr, len);
		if (n < 0)
			return ACL_VSTREAM_EOF;

		nleft -= n;
		ptr   += n;

#if 0
		if (n == len || fp->writev_fn == NULL || fp->rw_timeout <= 0)
			continue;

		/* 对于套接口写操作，如果一次性写没有写完，可能是系统
		 * 写缓冲区满，需要检测超时写
		 */
		begin = time(NULL);

		if (acl_write_wait(fd, fp->rw_timeout) == 0)
			continue;

		end = time(NULL);
		acl_msg_error("%s(%d), %s: acl_write_wait error,"
			"size: %d, nleft: %d, peer: %s, fd: %d,"
			" timeout: %d, cost: %ld", __FILE__, __LINE__,
			myname, (int) size, nleft, ACL_VSTREAM_PEER(fp), fd,
			fp->rw_timeout, end - begin);
		return ACL_VSTREAM_EOF;
#endif
	}

	return (int) (ptr - (const unsigned char *) vptr);
}

int acl_vstream_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen)
{
	if (fp == NULL || vptr == NULL || dlen == 0) {
		acl_msg_error("%s(%d), %s: fp %s, vptr %s, dlen %d", __FILE__,
			__LINE__, __FUNCTION__, fp ? "not null" : "null",
			vptr ? "not null" : "null", (int) dlen);
		return ACL_VSTREAM_EOF;
	}

	if (fp->wbuf_dlen > 0) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}
	return loop_writen(fp, vptr, dlen);
}

int acl_vstream_buffed_writen(ACL_VSTREAM *fp, const void *vptr, size_t dlen)
{
	if (fp == NULL || vptr == NULL || dlen == 0) {
		acl_msg_error("%s(%d), %s: fp %s, vptr %s, dlen %d", __FILE__,
			__LINE__, __FUNCTION__, fp ? "not null" : "null",
			vptr ? "not null" : "null", (int) dlen);
		return ACL_VSTREAM_EOF;
	}

	if (fp->wbuf == NULL) {
		fp->wbuf_size = 8192;
		fp->wbuf = acl_mymalloc(fp->wbuf_size);
	}

	if (dlen >= (size_t) fp->wbuf_size) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
		else if (loop_writen(fp, vptr, dlen) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
		else
			return (int) dlen;
	} else if (dlen + (size_t) fp->wbuf_dlen >=
		(size_t) fp->wbuf_size)
	{
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			return ACL_VSTREAM_EOF;
	}

	memcpy(fp->wbuf + (size_t) fp->wbuf_dlen, vptr, dlen);
	fp->wbuf_dlen += (int) dlen;
	return (int) dlen;
}

int acl_vstream_buffed_vfprintf(ACL_VSTREAM *fp, const char *fmt, va_list ap)
{
	const char *myname = "acl_vstream_buffed_vfprintf";
	ACL_VSTRING *buf;
	int   n;

	if (fp == NULL || fmt == NULL || *fmt == 0) {
		acl_msg_error("%s, %s(%d): fp %s, fmt %s",
			myname, __FILE__, __LINE__, fp ? "not null" : "null",
			fmt && *fmt ? "not null" : "null");
		return ACL_VSTREAM_EOF;
	}

	buf = acl_vstring_alloc(ACL_VSTREAM_BUFSIZE);
	if (buf == NULL) {
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			myname, __FILE__, __LINE__, acl_last_serror());
	}

	if (acl_vstring_vsprintf(buf, fmt, ap) == NULL)
		acl_msg_fatal("%s, %s(%d): vsprintf return null",
			myname, __FILE__, __LINE__);

	n = (int) LEN(buf);
	if (n <= 0)
		acl_msg_fatal("%s, %s(%d): len(%d) <= 0",
			myname, __FILE__, __LINE__, n);

	n = acl_vstream_buffed_writen(fp, STR(buf), n);

	acl_vstring_free(buf);
	return n;
}

int acl_vstream_buffed_fprintf(ACL_VSTREAM *fp, const char *fmt, ...)
{
	const char *myname = "acl_vstream_buffed_fprintf";
	va_list ap;
	int   n;

	if (fp == NULL || fmt == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
			myname, __FILE__, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	va_start(ap, fmt);
	n = acl_vstream_buffed_vfprintf(fp, fmt, ap);
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
		acl_msg_error("%s(%d): not a file fp", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	}

	if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): fflush fp fp's buff error(%s)",
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

void acl_vstream_buffed_space(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_buffed_space";

	if (fp == NULL) {
		acl_msg_error("%s(%d): fp null", myname, __LINE__);
		return;
	}
	if (fp->wbuf == NULL) {
		fp->wbuf_size = 8192;
		fp->wbuf_dlen = 0;
		fp->wbuf = acl_mymalloc(fp->wbuf_size);
	}
}

int acl_vstream_fflush(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_fflush";
	int   n;

	if (fp == NULL) {
		acl_msg_error("%s(%d): fp null", myname, __LINE__);
		return ACL_VSTREAM_EOF;
	} else if (fp->wbuf == NULL || fp->wbuf_dlen <= 0)
		return 0;

	n = loop_writen(fp, fp->wbuf, fp->wbuf_dlen);
	if (n > 0) {
		fp->wbuf_dlen -= n;
		if (fp->wbuf_dlen < 0)
			acl_msg_fatal("%s(%d): wbuf_dlen(%d) < 0",
				myname, __LINE__, fp->wbuf_dlen);
	}
	return n;
}

int acl_vstream_peekfd(ACL_VSTREAM *fp)
{
#ifdef ACL_UNIX
	int   n;

	if (fp != NULL && ACL_VSTREAM_SOCK(fp) != ACL_SOCKET_INVALID) {
		n = acl_peekfd(ACL_VSTREAM_SOCK(fp));
		if (n < 0)
			return -1;
		n += ACL_VSTREAM_BFRD_CNT(fp);

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


static ACL_VSTREAM_RD_FN acl_socket_read_fn   = acl_socket_read;
static ACL_VSTREAM_WR_FN acl_socket_write_fn  = acl_socket_write;
static ACL_VSTREAM_WV_FN acl_socket_writev_fn = acl_socket_writev;
static int (*acl_socket_close_fn)(ACL_SOCKET) = acl_socket_close;

void acl_socket_read_hook(ACL_VSTREAM_RD_FN read_fn)
{
	if (read_fn)
		acl_socket_read_fn = read_fn;
}

void acl_socket_write_hook(ACL_VSTREAM_WR_FN write_fn)
{
	if (write_fn)
		acl_socket_write_fn = write_fn;
}

void acl_socket_writev_hook(ACL_VSTREAM_WV_FN writev_fn)
{
	if (writev_fn)
		acl_socket_writev_fn = writev_fn;
}

void acl_socket_close_hook(int (*close_fn)(ACL_SOCKET))
{
	if (close_fn)
		acl_socket_close_fn = close_fn;
}

/* 定义流的缓冲区的默认大小 */

#define ACL_VSTREAM_DEF_MAXLEN  8192

ACL_VSTREAM *acl_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
	size_t buflen, int rw_timeout, int fdtype)
{
	const char *myname = "acl_vstream_fdopen";
	ACL_VSTREAM *fp;

	fp = (ACL_VSTREAM *) acl_mycalloc(1, sizeof(ACL_VSTREAM));

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

#ifdef ACL_MACOSX
	if ((fdtype & ACL_VSTREAM_TYPE_LISTEN_INET)
	    || (fdtype & ACL_VSTREAM_TYPE_LISTEN_UNIX))
	{
		fdtype |= ACL_VSTREAM_TYPE_LISTEN;
	}
#endif

	if (fd != ACL_SOCKET_INVALID && acl_is_listening_socket(fd)) {
		int ret = acl_getsocktype(fd);
		if (ret == AF_INET)
			fdtype |= ACL_VSTREAM_TYPE_LISTEN_INET;
#ifndef ACL_WINDOWS
		else if (ret == AF_UNIX)
			fdtype |= ACL_VSTREAM_TYPE_LISTEN_UNIX;
#endif
		fdtype |= ACL_VSTREAM_TYPE_LISTEN;
	}

	fp->read_buf = (unsigned char *) acl_mymalloc(buflen + 1);

	if (fdtype == 0) {
		fdtype = ACL_VSTREAM_TYPE_SOCK;
		acl_msg_warn("%s(%d): fdtype(0), set to ACL_VSTREAM_TYPE_SOCK",
			myname, __LINE__);
	}

	fp->read_buf_len     = (int) buflen;
	fp->type             = fdtype;
	ACL_VSTREAM_SOCK(fp) = fd;
#ifdef ACL_WINDOWS
	fp->iocp_sock        = ACL_SOCKET_INVALID;
#endif
	fp->read_ptr         = fp->read_buf;
	fp->oflags           = oflags;
	ACL_SAFE_STRNCPY(fp->errbuf, "OK", sizeof(fp->errbuf));

	if (rw_timeout > 0)
		fp->rw_timeout = rw_timeout;

	fp->sys_getc = read_char;
	if (fdtype == ACL_VSTREAM_TYPE_FILE) {
		fp->fread_fn   = acl_file_read;
		fp->fwrite_fn  = acl_file_write;
		fp->fwritev_fn = acl_file_writev;
		fp->fclose_fn  = acl_file_close;
	} else {
		fp->read_fn    = acl_socket_read_fn;
		fp->write_fn   = acl_socket_write_fn;
		fp->writev_fn  = acl_socket_writev_fn;
		fp->close_fn   = acl_socket_close_fn;

		/* xxx: 对于带有读写超时的流，需要先将 socket 设为非阻塞模式，
		 * 否则在写大数据包时会造成阻塞，超时作用失效
		 */
		if (rw_timeout > 0 && acl_getsocktype(fd) >= 0)
			acl_non_blocking(fd, ACL_NON_BLOCKING);
	}

	fp->addr_local = __empty_string;
	fp->addr_peer = __empty_string;
	fp->path = __empty_string;

	fp->close_handle_lnk = acl_array_create(8);
	return fp;
}

ACL_VSTREAM *acl_vstream_clone(const ACL_VSTREAM *from)
{
	const char *myname = "acl_vstream_clone";
	ACL_VSTREAM *to;
	ACL_VSTREAM_CLOSE_HANDLE *handle_from, *handle_to;
	int   i, n;

	if (from == NULL)
		acl_msg_fatal("%s(%d), %s: from null",
			__FILE__, __LINE__, myname);

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
	to->close_handle_lnk = acl_array_create(8);

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

int acl_vstream_set_fdtype(ACL_VSTREAM *fp, int type)
{
	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (type == ACL_VSTREAM_TYPE_FILE) {
		fp->fread_fn  = acl_file_read;
		fp->fwrite_fn = acl_file_write;
		fp->fclose_fn = acl_file_close;
		return 0;
	} else if (type == ACL_VSTREAM_TYPE_SOCK) {
		fp->read_fn  = acl_socket_read;
		fp->write_fn = acl_socket_write;
		fp->close_fn = acl_socket_close;
		return 0;
	}

	return -1;
}
/* acl_vstream_fopen - open buffered file fp */

ACL_VSTREAM *acl_vstream_fopen(const char *path, unsigned int oflags,
	int mode, size_t buflen)
{
	ACL_VSTREAM *fp;
	ACL_FILE_HANDLE fh;

	/* for linux2.6 */
#ifdef  _LARGEFILE64_SOURCE
	oflags |= O_LARGEFILE;
#endif

#ifdef	ACL_WINDOWS
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
#ifdef	ACL_WINDOWS
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

	while (1) {
		ret = acl_vstream_read(fp, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			break;
		acl_vstring_memcat(vbuf, (char *) buf, ret);
	}

	if (size)
		*size = (ssize_t) LEN(vbuf);

	acl_vstream_close(fp);
	ACL_VSTRING_TERMINATE(vbuf);

	return acl_vstring_export(vbuf);
}

/* acl_vstream_ctl - fine control */

void acl_vstream_ctl(ACL_VSTREAM *fp, int name,...)
{
	const char *myname = "acl_vstream_ctl";
	va_list ap;
	int   n;
	char *ptr;

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, myname);
		return;
	}

	va_start(ap, name);
	for (; name != ACL_VSTREAM_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_VSTREAM_CTL_READ_FN:
			fp->read_fn = va_arg(ap, ACL_VSTREAM_RD_FN);
			break;
		case ACL_VSTREAM_CTL_WRITE_FN:
			fp->write_fn = va_arg(ap, ACL_VSTREAM_WR_FN);
			break;
		case ACL_VSTREAM_CTL_CONTEXT:
			fp->context = va_arg(ap, char *);
			break;
		case ACL_VSTREAM_CTL_PATH:
			ptr = va_arg(ap, char*);
			if (fp->addr_peer && fp->addr_peer
				!= __empty_string)
			{
				acl_myfree(fp->addr_peer);
				fp->addr_peer = NULL;
			}
			fp->addr_peer = acl_mystrdup(ptr);
			break;
		case ACL_VSTREAM_CTL_FD:
			ACL_VSTREAM_SOCK(fp) = va_arg(ap, ACL_SOCKET);
			break;
		case ACL_VSTREAM_CTL_TIMEOUT:
			fp->rw_timeout = va_arg(ap, int);
			break;
		case ACL_VSTREAM_CTL_CACHE_SEEK:
			n = va_arg(ap, int);
			if (n)
				fp->flag |= ACL_VSTREAM_FLAG_CACHE_SEEK;
			else
				fp->flag &= ~ACL_VSTREAM_FLAG_CACHE_SEEK;
			break;
		default:
			acl_msg_panic("%s, %s(%d): bad name %d",
				myname, __FILE__, __LINE__, name);
		}
	}
	va_end(ap);
}

acl_off_t acl_vstream_fseek2(ACL_VSTREAM *fp, acl_off_t offset, int whence)
{
	const char *myname = "acl_vstream_fseek2";
	acl_off_t n;

	if (fp == NULL || ACL_VSTREAM_FILE(fp) == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): input error",
			myname, __FILE__, __LINE__);
		return -1;
	}

	if (fp->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s, %s(%d): type(%d) not ACL_VSTREAM_TYPE_FILE",
			myname, __FILE__, __LINE__, fp->type);
		return -1;
	}

	if (fp->wbuf_dlen > 0) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF) {
			acl_msg_error("%s, %s(%d): acl_vstream_fflush error",
				myname, __FILE__, __LINE__);
			return -1;
		}
	}

	if ((fp->flag & ACL_VSTREAM_FLAG_CACHE_SEEK) == 0) {
		fp->read_cnt = 0;
		goto SYS_SEEK2;
	}

	/* 获得真正的当前文件指针位置 */
	n = acl_lseek(ACL_VSTREAM_FILE(fp), (acl_off_t) 0, SEEK_CUR);
	if (n < 0)
		return -1;

	if (whence == SEEK_CUR) {
		if (fp->read_cnt >= offset) {
			fp->read_cnt -= (int) offset;
			n = -fp->read_cnt;  /* 计算出真实的文件位置 */
			fp->read_cnt = 0;
		} else if (fp->read_cnt >= 0) {
			offset -= fp->read_cnt;
			n = offset;  /* 计算出真实的文件位置 */
			fp->read_cnt = 0;
		} else { /* fp->read_cnt < 0 ? */
			acl_msg_error("%s, %s(%d): invalud read_cnt = %d",
				myname, __FILE__, __LINE__,
				(int) fp->read_cnt);
			return -1;
		}
	} else {
		n = offset;
		fp->read_cnt = 0;
	}

SYS_SEEK2:
	/* 定位到合适的位置 */
	fp->sys_offset = acl_lseek(
		ACL_VSTREAM_FILE(fp), offset, whence);
	fp->offset = fp->sys_offset;
	return fp->offset;
}

acl_off_t acl_vstream_fseek(ACL_VSTREAM *fp, acl_off_t offset, int whence)
{
	const char *myname = "acl_vstream_fseek";
	acl_off_t n;

	if (fp == NULL || ACL_VSTREAM_FILE(fp) == ACL_FILE_INVALID) {
		acl_msg_error("%s, %s(%d): input error",
			myname, __FILE__, __LINE__);
		return -1;
	}

	if (fp->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s, %s(%d): type(%d) not ACL_VSTREAM_TYPE_FILE",
			myname, __FILE__, __LINE__, fp->type);
		return -1;
	}

	if (fp->wbuf_dlen > 0) {
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF) {
			acl_msg_error("%s, %s(%d): acl_vstream_fflush error",
				myname, __FILE__, __LINE__);
			return -1;
		}
	}

	if ((fp->flag & ACL_VSTREAM_FLAG_CACHE_SEEK) == 0) {
		fp->read_cnt = 0;
		goto SYS_SEEK;
	}

	if (whence == SEEK_CUR) {
		/* 相对当前流位置 fp->offset 开始偏移 offset 的位置 */

		/* 必须严格检验 */
		if (fp->offset + fp->read_cnt != fp->sys_offset) {
			acl_msg_error("%s, %s(%d): offset(" ACL_FMT_I64D
				") + read_cnt(%d) != sys_offset("
				ACL_FMT_I64D ")", myname, __FILE__, __LINE__,
				fp->offset, fp->read_cnt,
				fp->sys_offset);
			fp->read_cnt = 0;
			goto SYS_SEEK;
		}

		/* 是否在读缓冲区间内 */
		if (fp->read_cnt >= offset) {
			/* 因为要从 fp->offset 偏移 offset 个字节后, 读指针
			 * fp->read_ptr 依然在缓冲区内, 所以只需要移动读指针
			 * 且减少缓冲区字节数、增加 fp->offset 偏移量即可.
			 */
			fp->read_cnt -= (int) offset;
			fp->read_ptr += (int) offset;
			fp->offset += offset;
			return fp->offset;
		} else if (fp->read_cnt >= 0) {
			/* 因为要计算从当前流位置 fp->offset 开始偏移 offset
			 * 的位置,而且流中还存在一定的缓存数据(fp->read_cnt),
			 * 所以需要先从 fp->offset 开始移动 fp->read_cnt
			 * 个字节(移动出读缓冲区),然后再移动剩余的字节
			 * (即 offset - fp->read_cnt) 即可; 因为已经称出读缓
			 * 冲区，所以需要将 fp->read_cnt 置 0.
			 */
			offset -= fp->read_cnt;
			fp->read_cnt = 0;
		} else { /* fp->read_cnt < 0 ? */
			acl_msg_error("%s, %s(%d): invalud read_cnt = %d",
				myname, __FILE__, __LINE__,
				(int) fp->read_cnt);
			fp->read_cnt = 0;
		}
	} else if (whence == SEEK_SET) {
#if 0
		/* 获得真正的当前文件指针位置 */
		fp->sys_offset = acl_lseek(ACL_VSTREAM_FILE(fp),
			(off_t) 0, SEEK_CUR);
#endif
		/* 利用缓存的偏移位置 */

		if (fp->sys_offset < 0) {
			acl_msg_error("%s, %s(%d): seek n(" ACL_FMT_I64D
				") invalid", myname, __FILE__, __LINE__,
				fp->sys_offset);
			fp->read_cnt = 0;
			goto SYS_SEEK;
		}

		/* 必须严格检验 */
		if (fp->offset + fp->read_cnt != fp->sys_offset) {
			acl_msg_error("%s, %s(%d): offset(" ACL_FMT_I64D
				") + read_cnt(%d) != sys_offset("
				ACL_FMT_I64D ")", myname, __FILE__, __LINE__,
				fp->offset, fp->read_cnt,
				fp->sys_offset);
			fp->read_cnt = 0;
			goto SYS_SEEK;
		}

		/* 如果读数据指针经过移动，可以将其回移，因为缓冲区内数据
		 * 并未破坏，可以复用
		 */
		if (fp->read_ptr > fp->read_buf) {
			n = fp->read_ptr - fp->read_buf;
			fp->offset -= n;
			fp->read_ptr = fp->read_buf;
			fp->read_cnt += (int) n;
		}

		/* 判断请求的偏移位置是否在读缓存区间内 */
		if (offset >= fp->offset && offset <= fp->sys_offset) {
			n = offset - fp->offset;
			fp->read_cnt -= (int) n;
			fp->read_ptr += n;
			fp->offset += n;
			return fp->offset;
		}
		fp->read_cnt = 0;
	} else
		fp->read_cnt = 0;

SYS_SEEK:
	/* 调用系统调用定位位置 */
	fp->sys_offset = acl_lseek(ACL_VSTREAM_FILE(fp), offset, whence);
	fp->offset = fp->sys_offset;

	return fp->offset;
}

acl_off_t acl_vstream_ftell(ACL_VSTREAM *fp)
{
	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	/* 先定位当前位置，然后再减去读缓冲区里的数据长度 */
	fp->sys_offset = acl_lseek(ACL_VSTREAM_FILE(fp), 0, SEEK_CUR);
	fp->offset = fp->sys_offset;
	return fp->offset - fp->read_cnt;
}

#ifdef ACL_WINDOWS
int acl_file_ftruncate(ACL_VSTREAM *fp, acl_off_t length)
{
	const char *myname = "acl_file_ftruncate";
	ACL_FILE_HANDLE hf = ACL_VSTREAM_FILE(fp);

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	/* 参见：C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\crt\src
	 * osfinfo.c
	 * _open_osfhandle: 将ACL_WINDOWS API的文件句柄转换为标准C的文件句柄
	 * _get_osfhandle: 根据标准C文件句柄查询ACL_WINDOWS API文件句柄
	 * _free_osfhnd: 释放由 _open_osfhandle 打开的标准C文件句柄的资源，
	 *               但并不实际关闭该ACL_WINDOWS API句柄，所以还得要对其真实
	 *               ACL_WINDOWS API文件句柄进行关闭
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

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	return ftruncate(hf, length);
}

int acl_file_truncate(const char *path, acl_off_t length)
{
	return truncate(path, length);
}

#endif /* !ACL_WINDOWS, ACL_UNIX */

int acl_vstream_fstat(ACL_VSTREAM *fp, struct acl_stat *buf)
{
	const char *myname = "acl_vstream_fstat";

	if (fp == NULL || buf == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return -1;
	} else if (fp->type != ACL_VSTREAM_TYPE_FILE) {
		acl_msg_error("%s(%d): not a file fp", myname, __LINE__);
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
		acl_msg_error("%s(%d): not a file fp", myname, __LINE__);
		return -1;
	}
	return acl_file_fsize(ACL_VSTREAM_FILE(fp), fp, fp->context)
		+ fp->wbuf_dlen;
}

void acl_vstream_reset(ACL_VSTREAM *fp)
{
	if (fp) {
		fp->read_cnt = 0;
		fp->read_ptr = fp->read_buf;
		fp->flag     = ACL_VSTREAM_FLAG_RW;
		fp->total_read_cnt = 0;
		fp->total_write_cnt = 0;
		fp->read_ready = 0;
		fp->wbuf_dlen = 0;
		fp->offset = 0;
		fp->nrefer = 0;
		fp->read_buf_len = 0;
		ACL_SAFE_STRNCPY(fp->errbuf, "OK", sizeof(fp->errbuf));
		acl_vstream_clean_close_handle(fp);
		if (fp->fdp != NULL)
			event_fdtable_reset(fp->fdp);
	}
}

void acl_vstream_free(ACL_VSTREAM *fp)
{
	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (fp->nrefer > 0) {
		/* 设置延迟释放标志位 */
		fp->flag |= ACL_VSTREAM_FLAG_DEFER_FREE;
		return;
	}

	if (fp->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = acl_array_size(fp->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i++) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				acl_array_index(fp->close_handle_lnk, i);
			if (close_handle == NULL)
				break;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在
			 * close_fn 里调用了删除回调函数的操作而造成对同一内
			 * 存的多次释放
			 */
			acl_array_delete_idx(fp->close_handle_lnk, i, NULL);
			close_handle->close_fn(fp, close_handle->context);
			acl_myfree(close_handle);
		}
		acl_array_destroy(fp->close_handle_lnk, NULL);
	}

	if (fp->fdp != NULL)
		event_fdtable_free(fp->fdp);
	if (fp->read_buf != NULL && fp->read_buf != __vstream_stdin_buf
		&& fp->read_buf != __vstream_stdout_buf
		&& fp->read_buf != __vstream_stderr_buf)
	{
		acl_myfree(fp->read_buf);
	}
	if (fp->wbuf != NULL)
		acl_myfree(fp->wbuf);

	if (fp->addr_peer && fp->addr_peer != __empty_string)
		acl_myfree(fp->addr_peer);
	if (fp->addr_local && fp->addr_local != __empty_string)
		acl_myfree(fp->addr_local);
	if (fp->sa_peer)
		acl_myfree(fp->sa_peer);
	if (fp->sa_local)
		acl_myfree(fp->sa_local);
	if (fp->path && fp->path != __empty_string)
		acl_myfree(fp->path);

	if (fp != &acl_vstream_fstd[0] && fp != &acl_vstream_fstd[1]
		&& fp != &acl_vstream_fstd[2])
	{
		acl_myfree(fp);
	}
}

int acl_vstream_close(ACL_VSTREAM *fp)
{
	const char *myname = "acl_vstream_close";
	int  ret = 0;

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (fp->nrefer > 0) {
		/* 设置延迟释放标志位 */
		fp->flag |= ACL_VSTREAM_FLAG_DEFER_FREE;
		return 0;
	}

	if (fp->wbuf_dlen > 0)
		if (acl_vstream_fflush(fp) == ACL_VSTREAM_EOF)
			acl_msg_error("%s: fflush fp error", myname);

	/* 须在调用各个关闭回调函数之前将连接关闭，否则会影响 iocp 的事件引擎
	 * 正常工作。在使用 iocp 事件引擎时，当流关闭时会调用 events_iocp.c 中
	 * 的 stream_on_close，该函数会释放掉 fdp->event_read/fdp->event_write
	 * 两个对象，但当套接口未关闭时，这两个对象有可能会被 iocp 使用，当套
	 * 接口关闭时，iocp 才不会使用这两个对象中的 IOCP_EVENT->overlapped 等
	 * 成员. ---2011.5.18, zsx
	 */
	/*
	 * 2011.5.18 的改动虽解决了事件引擎为 iocp 的问题，但同时造成了 win32
	 * 窗口消息引擎的问题，虽然 win32 消息引擎的方式在关闭套接口之前会回调
	 * stream_on_close，该回调要求套接口必须是打开的，既然二者出现了冲突，
	 * 则 iocp 的问题还是由 iocp 引擎本身去解决吧，即在 iocp 引擎的
	 * stream_on_close 中，在释放 fdp->event_read/fdp->event_write 前关闭
	 * 套接口即可，在 acl_vstream_close 最后需要关闭套接口时只要根据句柄
	 * 是否有效来判断是否调用关闭过程. ---2011.5.19, zsx
	 */
	/*
	if (fp->read_buf != NULL)
		acl_myfree(fp->read_buf);
	if (ACL_VSTREAM_SOCK(fp) != ACL_SOCKET_INVALID && fp->close_fn)
		ret = fp->close_fn(ACL_VSTREAM_SOCK(fp));
	else if (ACL_VSTREAM_FILE(fp) != ACL_FILE_INVALID && fp->fclose_fn)
		ret = fp->fclose_fn(ACL_VSTREAM_FILE(fp));
	ACL_VSTREAM_SOCK(fp) = ACL_SOCKET_INVALID;
	ACL_VSTREAM_FILE(fp) = ACL_FILE_INVALID;
	*/

	if (fp->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = acl_array_size(fp->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i--) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				acl_array_index(fp->close_handle_lnk, i);
			if (close_handle == NULL)
				continue;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在
			 * close_fn 里调用了删除回调函数的操作而造成对同一内存
			 * 的多次释放
			 */
			acl_array_delete_idx(fp->close_handle_lnk, i, NULL);
			close_handle->close_fn(fp, close_handle->context);
			acl_myfree(close_handle);
		}
		acl_array_destroy(fp->close_handle_lnk, NULL);
	}

	if (fp->objs_table)
		acl_htable_free(fp->objs_table, NULL);

	if (ACL_VSTREAM_SOCK(fp) != ACL_SOCKET_INVALID && fp->close_fn)
		ret = fp->close_fn(ACL_VSTREAM_SOCK(fp));
	else if (ACL_VSTREAM_FILE(fp) != ACL_FILE_INVALID && fp->fclose_fn)
		ret = fp->fclose_fn(ACL_VSTREAM_FILE(fp));

	if (fp->fdp != NULL)
		event_fdtable_free(fp->fdp);
	if (fp->read_buf != NULL)
		acl_myfree(fp->read_buf);
	if (fp->wbuf != NULL)
		acl_myfree(fp->wbuf);

	if (fp->addr_local && fp->addr_local != __empty_string)
		acl_myfree(fp->addr_local);
	if (fp->addr_peer && fp->addr_peer != __empty_string)
		acl_myfree(fp->addr_peer);
	if (fp->sa_peer)
		acl_myfree(fp->sa_peer);
	if (fp->sa_local)
		acl_myfree(fp->sa_local);
	if (fp->path && fp->path != __empty_string)
		acl_myfree(fp->path);

	acl_myfree(fp);
	return ret;
}

static void set_sock_addr(struct sockaddr_in *saddr, const char *addr)
{
	char  buf[128], *ptr;
	int   port;

	snprintf(buf, sizeof(buf), "%s", addr);
	ptr = strchr(buf, ':');
	if (ptr == NULL)
		return;

	*ptr++ = 0;
	port = atoi(ptr);

	saddr->sin_family = AF_INET;
	saddr->sin_port = htons(port);
	saddr->sin_addr.s_addr = inet_addr(buf);
}

void acl_vstream_set_local(ACL_VSTREAM *fp, const char *addr)
{
	if (fp == NULL || addr == NULL) {
		acl_msg_error("%s(%d), %s: fp %s, addr %s",
			__FILE__, __LINE__, __FUNCTION__,
			fp ? "not null" : "null", addr ? addr : "null");
		return;
	}

	if (fp->addr_local == __empty_string || fp->addr_local == NULL)
		fp->addr_local = acl_mystrdup(addr);
	else {
		acl_myfree(fp->addr_local);
		fp->addr_local = acl_mystrdup(addr);
	}

	if (fp->sa_local == NULL) {
		fp->sa_local_size = sizeof(struct sockaddr_in);
		fp->sa_local = (struct sockaddr_in*)
			acl_mycalloc(1, fp->sa_local_size);
	} else
		memset(fp->sa_local, 0, fp->sa_local_size);

	set_sock_addr(fp->sa_local, addr);
	fp->sa_local_len = fp->sa_local_size;
}

void acl_vstream_set_local_addr(ACL_VSTREAM *fp, const struct sockaddr_in *sa)
{
	int   port;
	char  ip[64], addr[64];

	if (fp == NULL || sa == NULL) {
		acl_msg_error("%s(%d), %s: fp %s, sa %s", __FILE__, __LINE__,
			__FUNCTION__, fp ? "not null" : "null",
			sa ? "not null" : "null");
		return;
	}

	if (fp->sa_local == NULL) {
		fp->sa_local_size = sizeof(struct sockaddr_in);
		fp->sa_local = (struct sockaddr_in*)
			acl_mymalloc(fp->sa_local_size);
	}

	memcpy(fp->sa_local, sa, fp->sa_local_size);
	fp->sa_local_len = fp->sa_local_size;

	ip[0] = 0;
	acl_inet_ntoa(sa->sin_addr, ip, sizeof(ip));
	port = ntohs(sa->sin_port);
	snprintf(addr, sizeof(addr), "%s:%d", ip, port);

	if (fp->addr_local == __empty_string || fp->addr_local == NULL)
		fp->addr_local = acl_mystrdup(addr);
	else {
		acl_myfree(fp->addr_local);
		fp->addr_local = acl_mystrdup(addr);
	}
}

void acl_vstream_set_peer(ACL_VSTREAM *fp, const char *addr)
{
	if (fp == NULL || addr == NULL) {
		acl_msg_error("%s(%d), %s: fp %s, addr %s", __FILE__, __LINE__,
			__FUNCTION__, fp ? "not null" : "null",
			addr ? "not null" : "null");
		return;
	}

	if (fp->addr_peer == __empty_string || fp->addr_peer == NULL)
		fp->addr_peer = acl_mystrdup(addr);
	else {
		acl_myfree(fp->addr_peer);
		fp->addr_peer = acl_mystrdup(addr);
	}

	if (fp->sa_peer == NULL) {
		fp->sa_peer_size = sizeof(struct sockaddr_in);
		fp->sa_peer = (struct sockaddr_in*)
			acl_mycalloc(1, fp->sa_peer_size);
	} else
		memset(fp->sa_peer, 0, fp->sa_peer_size);

	set_sock_addr(fp->sa_peer, addr);
	fp->sa_peer_len = fp->sa_peer_size;
}

void acl_vstream_set_peer_addr(ACL_VSTREAM *fp, const struct sockaddr_in *sa)
{
	int   port;
	char  ip[64], addr[64];

	if (fp == NULL || sa == NULL) {
		acl_msg_error("%s(%d), %s: fp %s, sa %s", __FILE__, __LINE__,
			__FUNCTION__, fp ? "not null" : "null",
			sa ? "not null" : "null");
		return;
	}

	if (fp->sa_peer == NULL) {
		fp->sa_peer_size = sizeof(struct sockaddr_in);
		fp->sa_peer = (struct sockaddr_in*)
			acl_mymalloc(fp->sa_peer_size);
	}

	memcpy(fp->sa_peer, sa, fp->sa_peer_size);
	fp->sa_peer_len = fp->sa_peer_size;

	ip[0] = 0;
	acl_inet_ntoa(sa->sin_addr, ip, sizeof(ip));
	port = ntohs(sa->sin_port);
	snprintf(addr, sizeof(addr), "%s:%d", ip, port);

	if (fp->addr_peer == __empty_string || fp->addr_peer == NULL)
		fp->addr_peer = acl_mystrdup(addr);
	else {
		acl_myfree(fp->addr_peer);
		fp->addr_peer = acl_mystrdup(addr);
	}
}

void acl_vstream_set_path(ACL_VSTREAM *fp, const char *path)
{
	if (fp == NULL || path == NULL) {
		acl_msg_error("%s(%d), %s: fp %s, path %s", __FILE__, __LINE__,
			__FUNCTION__, fp ? "not null" : "null",
			path ? "not null" : "null");
		return;
	}

	if (fp->path == __empty_string || fp->path == NULL)
		fp->path = acl_mystrdup(path);
	else {
		acl_myfree(fp->path);
		fp->path = acl_mystrdup(path);
	}
}

void acl_vstream_call_close_handles(ACL_VSTREAM *fp)
{
	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return;
	}

	if (fp->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = acl_array_size(fp->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i--) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				acl_array_index(fp->close_handle_lnk, i);
			if (close_handle == NULL)
				continue;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在
			 * close_fn 里调用了删除回调函数的操作而造成对同一内存
			 * 的多次释放
			 */
			acl_array_delete_idx(fp->close_handle_lnk, i, NULL);
			close_handle->close_fn(fp, close_handle->context);
			acl_myfree(close_handle);
		}
		acl_array_clean(fp->close_handle_lnk, NULL);
	}
}

void acl_vstream_add_close_handle(ACL_VSTREAM *fp,
	void (*close_fn)(ACL_VSTREAM*, void*), void *context)
{
	const char *myname = "acl_vstream_add_close_handle";
	ACL_VSTREAM_CLOSE_HANDLE *close_handle;
	ACL_ITER  iter;

	if (fp == NULL) {
		acl_msg_error("%s, %s(%d): fp null",
			myname, __FILE__, __LINE__);
		return;
	}

	if (fp->close_handle_lnk == NULL)
		fp->close_handle_lnk = acl_array_create(8);

	if (close_fn == NULL)
		acl_msg_fatal("%s, %s(%d): close_fn null",
			myname, __FILE__, __LINE__);

	acl_foreach(iter, fp->close_handle_lnk) {
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

	if (acl_array_append(fp->close_handle_lnk, close_handle) < 0)
		acl_msg_fatal("%s, %s(%d): acl_array_append error=%s",
			myname, __FILE__, __LINE__, acl_last_serror());
}

void acl_vstream_delete_close_handle(ACL_VSTREAM *fp,
	void (*close_fn)(ACL_VSTREAM*, void*), void *context)
{
	const char *myname = "acl_vstream_delete_close_handle";
	ACL_VSTREAM_CLOSE_HANDLE *close_handle;
	int   i, n;

	if (fp == NULL) {
		acl_msg_error("%s(%d): fp null", myname, __LINE__);
		return;
	}
	if (fp->close_handle_lnk == NULL)
		return;

	if (close_fn == NULL) {
		acl_msg_error("%s(%d): close_fn null", myname, __LINE__);
		return;
	}

	n = acl_array_size(fp->close_handle_lnk);
	if (n <= 0)
		return;

	/* 因为添加时是正序的, 所以在删除时是倒序的,
	 * 这样对动态数组的使用的效率才会比较高, 
	 * 避免了动态数组内部移动的情况
	 */
	for (i = n - 1; i >= 0; i--) {
		close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
			acl_array_index(fp->close_handle_lnk, i);
		if (close_handle == NULL)
			continue;
		if (close_handle->close_fn == close_fn
		    && close_handle->context == context)
		{
			acl_array_delete_idx(fp->close_handle_lnk, i, NULL);
			acl_myfree(close_handle);
			break;
		}
	}
}

void acl_vstream_clean_close_handle(ACL_VSTREAM *fp)
{
	int   i, n;
	ACL_VSTREAM_CLOSE_HANDLE *close_handle;

	if (fp == NULL || fp->close_handle_lnk == NULL)
		return;

	n = acl_array_size(fp->close_handle_lnk);
	/* 因为添加时是正序的, 所以在删除时是倒序的,
	 * 这样对动态数组的使用的效率才会比较高, 
	 * 避免了动态数组内部移动的情况
	 */
	for (i = n - 1; i >= 0; i++) {
		close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
			acl_array_index(fp->close_handle_lnk, i);
		acl_array_delete_idx(fp->close_handle_lnk, i, NULL);
		acl_myfree(close_handle);
	}

	acl_array_clean(fp->close_handle_lnk, NULL);
}

const char *acl_vstream_strerror(ACL_VSTREAM *fp)
{
	static char err[] = "input error";

	if (fp == NULL) {
		acl_msg_error("%s(%d), %s: fp null",
			__FILE__, __LINE__, __FUNCTION__);
		return err;
	}

	return fp->errbuf;
}

int acl_vstream_add_object(ACL_VSTREAM *fp, const char *key, void *obj)
{
	if (fp == NULL || key == NULL || *key == 0 || obj == NULL) {
		acl_msg_error("%s(%d), %s: fp %s, key %s, obj %s",
			__FILE__, __LINE__, __FUNCTION__,
			fp ? "not null" : "null", key && *key ? key : "null",
			obj ? "not null" : "null");
		return -1;
	}

	if (fp->objs_table == NULL)
		fp->objs_table = acl_htable_create(5, ACL_HTABLE_FLAG_KEY_LOWER);

	acl_htable_enter(fp->objs_table, key, obj);
	return 0;
}

int acl_vstream_del_object(ACL_VSTREAM *fp, const char *key)
{
	if (fp == NULL || fp->objs_table == NULL || key == NULL || *key == 0) {
		acl_msg_error("%s(%d), %s: fp %s, key %s", __FILE__, __LINE__,
			__FUNCTION__, fp ? "not null" : "null",
			key && *key ? key : "null");
		return -1;
	}

	return acl_htable_delete(fp->objs_table, key, NULL);
}

void *acl_vstream_get_object(ACL_VSTREAM *fp, const char *key)
{
	if (fp == NULL || fp->objs_table == NULL || key == NULL || *key == 0) {
		acl_msg_error("%s(%d), %s: fp %s, key %s", __FILE__, __LINE__,
			__FUNCTION__, fp ? "not null" : "null",
			key && *key ? key : "null");
		return NULL;
	}

	return acl_htable_find(fp->objs_table, key);
}
