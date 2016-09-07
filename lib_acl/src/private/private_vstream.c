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
# include <sys/stat.h>
# include <unistd.h>
#else
# error "unknown OS type"
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_array.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "net/acl_connect.h"
#include "net/acl_sane_socket.h"

#endif

#include "private_array.h"
#include "private_vstream.h"

#define	MAX_ADDR_SIZE	256

static int __sys_getc(ACL_VSTREAM *stream);

static int  __read_wait(ACL_SOCKET fd, int timeout)
{
	fd_set  read_fds;
	fd_set  except_fds;
	struct timeval tv;
	struct timeval *tp;

#ifdef	ACL_UNIX
	/*
	 * Sanity checks.
	 */
	acl_assert(FD_SETSIZE > (unsigned) fd);
#endif

	/*
	 * Use select() so we do not depend on alarm() and on signal() handlers.
	 * Restart the select when interrupted by some signal. Some select()
	 * implementations reduce the time to wait when interrupted, which is
	 * exactly what we want.
	 */
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	FD_ZERO(&except_fds);
	FD_SET(fd, &except_fds);
	if (timeout >= 0) {
		tv.tv_usec = 0;
		tv.tv_sec = timeout;
		tp = &tv;
	} else
		tp = 0;

	for (;;) {
		switch (select((int) fd + 1, &read_fds, (fd_set *) 0,
			&except_fds, tp))
		{
		case -1:
			if (acl_last_error() != ACL_EINTR)
				return -1;
			continue;
		case 0:
			acl_set_error(ACL_ETIMEDOUT);
			return (-1);
		default:
			return (0);
		}
	}
}

static int __vstream_sys_read(ACL_VSTREAM *stream)
{
	if (stream == NULL)
		return (-1);

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID)
			return (-1);
	} else if (ACL_VSTREAM_SOCK(stream) == ACL_SOCKET_INVALID)
		return (-1);

AGAIN:
	if (stream->rw_timeout > 0 && __read_wait(ACL_VSTREAM_SOCK(stream),
		stream->rw_timeout) < 0)
	{
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

		return (-1);
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
		return (-1);
	} else if (stream->read_cnt == 0) { /* closed by peer */
		stream->flag = ACL_VSTREAM_FLAG_EOF;
		stream->errnum = 0;
		snprintf(stream->errbuf, sizeof(stream->errbuf),
			"closed by peer(%s)", acl_last_serror());

		return (0);
	}

	stream->read_ptr = stream->read_buf;
	stream->flag &= ~ACL_VSTREAM_FLAG_BAD;
	stream->errnum = 0;
	stream->errbuf[0] = 0;
	stream->total_read_cnt += stream->read_cnt;

	return ((int) stream->read_cnt);
}

static int __sys_getc(ACL_VSTREAM *stream)
{
	stream->read_cnt = __vstream_sys_read(stream);
	if (stream->read_cnt <= 0)
		return (ACL_VSTREAM_EOF);
	else
		return (ACL_VSTREAM_GETC(stream));
}

int private_vstream_getc(ACL_VSTREAM *stream)
{
	if (stream == NULL)
		return (ACL_VSTREAM_EOF);
	if (stream->read_cnt <= 0) {
		if (__vstream_sys_read(stream) <= 0)
			return (ACL_VSTREAM_EOF);
	}

	stream->read_cnt--;
	stream->offset++;
	return (*stream->read_ptr++);
}

int private_vstream_ungetc(ACL_VSTREAM *stream, int ch)
{
	unsigned char c;

	c = (unsigned char) ch;
	(void) acl_vstream_unread(stream, &c, 1);
	return (ch);
}

static int vstream_bfcp_some(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n;

	/* input params error */
	acl_assert(stream && vptr && maxlen > 0);

	/* internal fatal error */
	acl_assert(stream->read_cnt >= 0);

	/* there is no any data in buf */
	if (stream->read_cnt == 0) {
		stream->read_ptr = stream->read_buf;
		return (0);
	}

	if (stream->read_ptr >= stream->read_buf + (int) stream->read_buf_len) {
		stream->read_cnt = 0;
		stream->read_ptr = stream->read_buf;
		return (0);
	}

	n = (int) stream->read_cnt > (int) maxlen
		? (int) maxlen : (int) stream->read_cnt;

	memcpy(vptr, stream->read_ptr, n);

	stream->read_cnt -= n;
	stream->read_ptr += n;
	stream->offset += n;

	return (n);
}

int private_vstream_gets(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n, ch;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || maxlen <= 0)
		return (ACL_VSTREAM_EOF);

	stream->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	ptr = (unsigned char *) vptr;
	for (n = 1; n < (int) maxlen; n++) {  /* left one byte for '\0' */
		ch = private_vstream_getc(stream);
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 1)
				return (ACL_VSTREAM_EOF);/* EOF, nodata read */
			break;		/* EOF, some data was read */
		} else {
			*ptr++ = ch;
			if (ch == '\n'){	/* newline is stored, like fgets() */
				stream->flag |= ACL_VSTREAM_FLAG_TAGYES;
				break;
			}
		}
	}

	*ptr = 0;				/* null terminate like fgets() */
	return (n);
}

int private_vstream_gets_nonl(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n, ch;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || maxlen <= 0)
		return (ACL_VSTREAM_EOF);

	stream->flag &= ~ACL_VSTREAM_FLAG_TAGYES;

	ptr = (unsigned char *) vptr;
	for (n = 1; n < (int) maxlen; n++) {
		ch = private_vstream_getc(stream);
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 1)
				return (ACL_VSTREAM_EOF);  /* EOF, nodata read */
			else
				break;		/* EOF, some data was read */
		} else {
			*ptr++ = ch;
			if (ch == '\n') {
				stream->flag |= ACL_VSTREAM_FLAG_TAGYES;
				break;	/* newline is stored, like fgets() */
			}
		}
	}

	*ptr = 0;				/* null terminate like fgets() */
	ptr--;
	while (ptr >= (unsigned char *) vptr) {
		if (*ptr == '\r' || *ptr == '\n') {
			*ptr-- = 0;
			n--;
			continue;
		}
		break;
	}
	return (n);
}

int private_vstream_readn(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   n, ch;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || (int) maxlen <= 0)
		return (ACL_VSTREAM_EOF);

	ptr = (unsigned char *) vptr;
	for (n = 0; n < (int) maxlen; n++) {
		ch = private_vstream_getc(stream);
		if (ch == ACL_VSTREAM_EOF) {
			if (n == 0)
				return (ACL_VSTREAM_EOF); /* EOF, nodata read */
			else
				break;		/* EOF, some data was read */
		} else {
			*ptr++ = ch;
		}
	}

	if (n != (int) maxlen) {
		snprintf(stream->errbuf, sizeof(stream->errbuf),
			"nread=%d, nneed=%d, errmsg=not read the needed data",
			n, (int) maxlen);
		stream->flag |= ACL_VSTREAM_FLAG_RDSHORT;

		return (ACL_VSTREAM_EOF);
	}

	return (n);
}

int private_vstream_read(ACL_VSTREAM *stream, void *vptr, size_t maxlen)
{
	int   read_cnt;
	unsigned char *ptr;

	if (stream == NULL || vptr == NULL || (int) maxlen <= 0)
		return (ACL_VSTREAM_EOF);

	acl_assert(stream->read_cnt >= 0);
	ptr = (unsigned char *) vptr;
	if (stream->read_cnt > 0) {
		read_cnt = vstream_bfcp_some(stream, ptr, maxlen);
		return (read_cnt);
	}

	/* stream->read_cnt == 0 */

	/* there is no data in buf, so need to read data from system */
	read_cnt = __vstream_sys_read(stream);
	if (read_cnt < 0)
		return (ACL_VSTREAM_EOF);
	else if (read_cnt == 0)
		return (ACL_VSTREAM_EOF);

	read_cnt = vstream_bfcp_some(stream, ptr, maxlen);
	return (read_cnt);
}

static int __vstream_sys_write(ACL_VSTREAM *stream, const void *vptr, int dlen)
{
	int   n, neintr = 0;

	acl_assert(stream && vptr && dlen > 0);

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if (ACL_VSTREAM_FILE(stream) == ACL_FILE_INVALID)
			return (ACL_VSTREAM_EOF);
	} else if (ACL_VSTREAM_SOCK(stream) == ACL_SOCKET_INVALID)
		return (ACL_VSTREAM_EOF);

TAG_AGAIN:

	if (stream->type == ACL_VSTREAM_TYPE_FILE) {
		if ((stream->oflags & O_APPEND)) {
#ifdef ACL_WINDOWS
			stream->sys_offset = acl_lseek(
				ACL_VSTREAM_FILE(stream), 0, SEEK_END);
			if (stream->sys_offset < 0)
				return (ACL_VSTREAM_EOF);
#endif
		} else if ((stream->flag & ACL_VSTREAM_FLAG_CACHE_SEEK)
			&& stream->offset != stream->sys_offset)
		{
			stream->sys_offset = acl_lseek(ACL_VSTREAM_FILE(stream),
				stream->offset, SEEK_SET);
			if (stream->sys_offset == -1)
				return (ACL_VSTREAM_EOF);
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
				return (ACL_VSTREAM_EOF);

			goto TAG_AGAIN;
		}

		if (acl_last_error() == ACL_EAGAIN
			|| acl_last_error() == ACL_EWOULDBLOCK)
		{
			acl_set_error(ACL_EAGAIN);
		}

		return (ACL_VSTREAM_EOF);
	}

	stream->total_write_cnt += n;
	return (n);
}

static int __loop_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen)
{
	const unsigned char *ptr;
	int   n;

	ptr   = (const unsigned char *) vptr;
	while (dlen > 0) {
		n = __vstream_sys_write(stream, ptr, (int) dlen);
		if (n <= 0) {
			if (acl_last_error() == ACL_EINTR
				|| acl_last_error() == ACL_EAGAIN)
			{
				continue;
			}
			return (ACL_VSTREAM_EOF);
		}

		dlen  -= n;
		ptr   += n;
	}

	return (int) (ptr - (const unsigned char *) vptr);
}

int private_vstream_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen)
{
	acl_assert(stream && vptr && dlen > 0);

	if (stream->wbuf_dlen > 0) {
		if (private_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return (ACL_VSTREAM_EOF);
	}
	return (__loop_writen(stream, vptr, dlen));
}

int private_vstream_write(ACL_VSTREAM *stream, const void *vptr, size_t dlen)
{
	return (__vstream_sys_write(stream, vptr, (int) dlen));
}

int private_vstream_buffed_writen(ACL_VSTREAM *stream,
	const void *vptr, size_t dlen)
{
	acl_assert(stream && vptr && dlen > 0);

	if (stream->wbuf == NULL) {
		stream->wbuf_size = 8192;
		stream->wbuf = malloc(stream->wbuf_size);
	}

	if (dlen >= (size_t) stream->wbuf_size) {
		if (private_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return (ACL_VSTREAM_EOF);
		else if (__loop_writen(stream, vptr, dlen) == ACL_VSTREAM_EOF)
			return (ACL_VSTREAM_EOF);
		else
			return (int) (dlen);
	} else if (dlen + (size_t) stream->wbuf_dlen >=
		(size_t) stream->wbuf_size)
	{
		if (private_vstream_fflush(stream) == ACL_VSTREAM_EOF)
			return (ACL_VSTREAM_EOF);
	}

	memcpy(stream->wbuf + (size_t) stream->wbuf_dlen, vptr, dlen);
	stream->wbuf_dlen += (int) dlen;
	return (int) (dlen);
}

int private_vstream_fflush(ACL_VSTREAM *stream)
{
	unsigned char *ptr;
	int   n;

	acl_assert(stream);
	if (stream->wbuf == NULL || stream->wbuf_dlen == 0)
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
			return (ACL_VSTREAM_EOF);
		}

		stream->wbuf_dlen -= n;
		ptr += n;
	}

	acl_assert(stream->wbuf_dlen >= 0);

	return (int) (ptr - stream->wbuf);
}

ACL_VSTREAM *private_vstream_fhopen(ACL_FILE_HANDLE fh, unsigned int oflags)
{
	ACL_VSTREAM *fp;

	acl_assert(fh != ACL_FILE_INVALID);

	fp = private_vstream_fdopen(ACL_SOCKET_INVALID, oflags,
		4096, 0, ACL_VSTREAM_TYPE_FILE);
	if (fp == NULL)
		return (NULL);

	fp->fd.h_file = fh;
	return (fp);
}


/* 定义流的缓冲区的默认大小 */

#define ACL_VSTREAM_DEF_MAXLEN  8192

ACL_VSTREAM *private_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
	size_t buflen, int rw_timeo, int fdtype)
{
	ACL_VSTREAM *stream = NULL;

	stream = (ACL_VSTREAM *) calloc(1, sizeof(ACL_VSTREAM));
	acl_assert(stream);

	if (buflen < ACL_VSTREAM_DEF_MAXLEN)
		buflen = ACL_VSTREAM_DEF_MAXLEN;

	/* XXX: 只有非监听流才需要有读缓冲区 */

	if ((fdtype & ACL_VSTREAM_TYPE_LISTEN_INET)
	    || (fdtype & ACL_VSTREAM_TYPE_LISTEN_UNIX))
	{
		fdtype |= ACL_VSTREAM_TYPE_LISTEN;
		stream->read_buf = NULL;
	} else {
		stream->read_buf = (unsigned char *) malloc(buflen + 1);
		acl_assert(stream->read_buf != NULL);
	}

	if (fdtype == 0)
		fdtype = ACL_VSTREAM_TYPE_SOCK;

	stream->read_buf_len     = (int) buflen;
	stream->type             = fdtype;
	ACL_VSTREAM_SOCK(stream) = fd;
#ifdef ACL_WINDOWS
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

	stream->addr_peer = NULL;
	stream->addr_local = NULL;
	stream->path = NULL;

	stream->close_handle_lnk = private_array_create(5);
	if (stream->close_handle_lnk == NULL) {
		free(stream->read_buf);
		free(stream);
		return (NULL);
	}

	return (stream);
}

ACL_VSTREAM *private_vstream_fopen(const char *path, unsigned int oflags,
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
		return (NULL);

	fp = private_vstream_fdopen(ACL_SOCKET_INVALID,
		oflags, buflen, 0, ACL_VSTREAM_TYPE_FILE);
	if (fp == NULL)
		return (NULL);

	fp->fd.h_file = fh;
	snprintf(fp->addr_peer, MAX_ADDR_SIZE, "%s", path);
	return (fp);
}

void private_vstream_ctl(ACL_VSTREAM *stream, int name,...)
{
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
			snprintf(stream->addr_peer, MAX_ADDR_SIZE, "%s", ptr);
			break;
		case ACL_VSTREAM_CTL_FD:
			ACL_VSTREAM_SOCK(stream) = va_arg(ap, ACL_SOCKET);
			break;
		case ACL_VSTREAM_CTL_TIMEOUT:
			stream->rw_timeout = va_arg(ap, int);
			break;
		case ACL_VSTREAM_CTL_CACHE_SEEK:
			n = va_arg(ap, int);
			if (n) {
				stream->flag |= ACL_VSTREAM_FLAG_CACHE_SEEK;
			} else {
				stream->flag &= ~ACL_VSTREAM_FLAG_CACHE_SEEK;
			}
			break;
		default:
			acl_assert(0);
			break;
		}
	}
	va_end(ap);
}

ACL_VSTREAM *private_vstream_connect(const char *addr,
	int conn_timeout, int rw_timeout)
{
	return (private_vstream_connect_ex(addr, ACL_BLOCKING,
			conn_timeout, rw_timeout, 8192, NULL));
}

ACL_VSTREAM *private_vstream_connect_ex(const char *addr, int block_mode,
	int conn_timeout, int rw_timeout, int rw_bufsize, int *he_errorp)
{
	ACL_VSTREAM *stream;
	ACL_SOCKET fd;
	char *ptr;

	acl_assert(addr && *addr);
	ptr = strchr(addr, ':');
	if (ptr)
		fd = acl_inet_connect_ex(addr, ACL_BLOCKING,
			conn_timeout, he_errorp);
#ifdef	ACL_WINDOWS
	else
		return (NULL);
#elif defined(ACL_UNIX)
	else
		fd = acl_unix_connect(addr, block_mode, conn_timeout);
#else
	else
		return (NULL);
#endif

	if (fd == ACL_SOCKET_INVALID)
		return (NULL);
	stream = private_vstream_fdopen(fd, ACL_VSTREAM_FLAG_RW, rw_bufsize,
			rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	acl_assert(stream);

	if (acl_getpeername(ACL_VSTREAM_SOCK(stream), stream->addr_peer,
		MAX_ADDR_SIZE) < 0)
	{
		snprintf(stream->addr_peer, MAX_ADDR_SIZE, "%s", addr);
	}

	return (stream);
}

void private_vstream_free(ACL_VSTREAM *stream)
{
	if (stream->nrefer > 0) {
		/* 设置延迟释放标志位 */
		stream->flag |= ACL_VSTREAM_FLAG_DEFER_FREE;
		return;
	}

	if (stream->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = private_array_size(stream->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i++) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				private_array_index(stream->close_handle_lnk, i);
			if (close_handle == NULL)
				break;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在 close_fn
			 * 里调用了删除回调函数的操作而造成对同一内存的多次释放
			 */
			private_array_delete(stream->close_handle_lnk, i, NULL);
			close_handle->close_fn(stream, close_handle->context);
			free(close_handle);
		}
		private_array_destroy(stream->close_handle_lnk, NULL);
	}

	if (stream->read_buf != NULL)
		free(stream->read_buf);

	ACL_VSTREAM_SOCK(stream) = ACL_SOCKET_INVALID;
	ACL_VSTREAM_FILE(stream) = ACL_FILE_INVALID;
	free(stream->addr_peer);
	free(stream->addr_local);
	free(stream);
}

int private_vstream_close(ACL_VSTREAM *stream)
{
	int  ret = 0;

	if (stream->nrefer > 0) {
		/* 设置延迟释放标志位 */
		stream->flag |= ACL_VSTREAM_FLAG_DEFER_FREE;
		return (0);
	}

	if (stream->wbuf_dlen > 0)
		(void) private_vstream_fflush(stream);

	if (stream->close_handle_lnk != NULL) {
		ACL_VSTREAM_CLOSE_HANDLE *close_handle;
		int   i, n = private_array_size(stream->close_handle_lnk);

		/* 因为添加时是正序的, 所以在删除时是倒序的,
		 * 这样对动态数组的使用的效率才会比较高, 
		 * 避免了动态数组内部移动的情况
		 */
		for (i = n - 1; i >= 0; i--) {
			close_handle = (ACL_VSTREAM_CLOSE_HANDLE *)
				private_array_index(stream->close_handle_lnk, i);
			if (close_handle == NULL)
				continue;
			if (close_handle->close_fn == NULL)
				continue;
			/* 只所将此调用放在 close_fn 前面，是为了防止有人误在 close_fn
			 * 里调用了删除回调函数的操作而造成对同一内存的多次释放
			 */
			private_array_delete(stream->close_handle_lnk, i, NULL);
			close_handle->close_fn(stream, close_handle->context);
			free(close_handle);
		}
		private_array_destroy(stream->close_handle_lnk, NULL);
	}

	if (ACL_VSTREAM_SOCK(stream) != ACL_SOCKET_INVALID && stream->close_fn)
		ret = stream->close_fn(ACL_VSTREAM_SOCK(stream));
	else if (ACL_VSTREAM_FILE(stream) != ACL_FILE_INVALID && stream->fclose_fn)
		ret = stream->fclose_fn(ACL_VSTREAM_FILE(stream));
	if (stream->read_buf != NULL)
		free(stream->read_buf);
	if (stream->wbuf != NULL)
		free(stream->wbuf);
	free(stream);
	return (ret);
}
