#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System libraries. */

#include "stdlib/acl_define.h"

#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_vbuf_print.h"
#include "stdlib/acl_vstring.h"

#ifdef ACL_UNIX
#include <sys/mman.h>
#endif

#endif

#include "charmap.h"

#define MAX_PREALLOC	(1024*1024)

/* vstring_extend - variable-length string buffer extension policy */

static int vstring_extend(ACL_VBUF *bp, ssize_t incr)
{
	const char *myname = "vstring_extend";
	ssize_t used = (ssize_t) (bp->ptr - bp->data), new_len;
	ACL_VSTRING *vp = (ACL_VSTRING *) bp;

	if (vp->maxlen > 0 && (ssize_t) ACL_VSTRING_LEN(vp) >= vp->maxlen) {
		ACL_VSTRING_AT_OFFSET(vp, vp->maxlen - 1);
		ACL_VSTRING_TERMINATE(vp);
		acl_msg_warn("%s(%d), %s: overflow maxlen: %ld, %ld",
			__FILE__, __LINE__, myname, (long) vp->maxlen,
			(long) ACL_VSTRING_LEN(vp));
		bp->flags |= ACL_VBUF_FLAG_EOF;
		return ACL_VBUF_EOF;
	}

#ifdef ACL_WINDOWS
	if (bp->fd == ACL_FILE_INVALID && (bp->flags & ACL_VBUF_FLAG_FIXED))
#else
	if (bp->fd < 0 && (bp->flags & ACL_VBUF_FLAG_FIXED))
#endif
	{
		acl_msg_warn("%s(%d), %s: can't extend fixed buffer",
			__FILE__, __LINE__, myname);
		return ACL_VBUF_EOF;
	}

	/*
	 * Note: vp->vbuf.len is the current buffer size (both on entry and on
	 * exit of this routine). We round up the increment size to the buffer
	 * size to avoid silly little buffer increments. With really large
	 * strings we might want to abandon the length doubling strategy, and
	 * go to fixed increments.
	 */
#ifdef INCR_NO_DOUBLE
	/* below come from redis-server/sds.c/sdsMakeRoomFor, which can
	 * avoid memory double growing too large --- 2015.2.2, zsx
	 */
	new_len = bp->len + incr;
	if (new_len < MAX_PREALLOC)
		new_len *= 2;
	else
		new_len += MAX_PREALLOC;
#else
	new_len = bp->len + (bp->len > incr ? bp->len : incr);
#endif

	if (vp->maxlen > 0 && new_len > vp->maxlen)
		new_len = vp->maxlen;

	if (vp->slice)
		bp->data = (unsigned char *) acl_slice_pool_realloc(
			__FILE__, __LINE__, vp->slice, bp->data, new_len);
	else if (vp->dbuf) {
		const unsigned char *data = bp->data;
		bp->data = (unsigned char *) acl_dbuf_pool_alloc(
			vp->dbuf, new_len);
		memcpy(bp->data, data, used);
		acl_dbuf_pool_free(vp->dbuf, data);
	} else if (bp->fd != ACL_FILE_INVALID) {
#ifdef ACL_UNIX
		acl_off_t off = new_len - 1;
		if (acl_lseek(bp->fd, off, SEEK_SET) != (acl_off_t) off)
			acl_msg_fatal("lseek failed: %s, off: %lld",
				acl_last_serror(), off);
		if (acl_file_write(bp->fd, "\0", 1, 0, NULL, NULL)
			== ACL_VSTREAM_EOF)
		{
			acl_msg_fatal("write error: %s", acl_last_serror());
		}
#endif
	} else
		bp->data = (unsigned char *) acl_myrealloc(bp->data, new_len);

	bp->len = new_len;
	bp->ptr = bp->data + used;
	bp->cnt = bp->len - used;

	return 0;
}

/* vstring_buf_get_ready - vbuf callback for read buffer empty condition */

static int vstring_buf_get_ready(ACL_VBUF *buf acl_unused)
{
	acl_msg_panic("vstring_buf_get: write-only buffer");
	return 0;
}

/* vstring_buf_put_ready - vbuf callback for write buffer full condition */

static int vstring_buf_put_ready(ACL_VBUF *bp)
{
	return vstring_extend(bp, 0);
}

/* vstring_buf_space - vbuf callback to reserve space */

static int vstring_buf_space(ACL_VBUF *bp, ssize_t len)
{
	ssize_t need;

	if (len < 0)
		acl_msg_panic("vstring_buf_space: bad length %ld", (long) len);
	if ((need = len - bp->cnt) > 0)
		return vstring_extend(bp, need);
	else
		return 0;
}

void acl_vstring_init(ACL_VSTRING *vp, size_t len)
{
	if (len < 1)
		acl_msg_panic("acl_vstring_alloc: bad input, len < 1");

	vp->slice = NULL;
	vp->dbuf = NULL;
	vp->vbuf.data = (unsigned char *) acl_mymalloc(len);

	vp->vbuf.flags = 0;
	vp->vbuf.len = (int) len;
	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space = vstring_buf_space;
/*	vp->vbuf.ctx = NULL; */
	vp->maxlen  = 0;
	vp->slice   = NULL;
	vp->vbuf.fd = ACL_FILE_INVALID;
}

void acl_vstring_free_buf(ACL_VSTRING *vp)
{
	if (vp->vbuf.data == NULL)
		return;

	if (vp->slice)
		acl_slice_pool_free(__FILE__, __LINE__, vp->vbuf.data);
#ifdef ACL_UNIX
	if (vp->vbuf.fd != ACL_FILE_INVALID) {
		if (vp->maxlen > 0 && munmap(vp->vbuf.data, vp->maxlen) < 0)
			acl_msg_error("%s(%d), %s: munmap error %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_last_serror());
	}
#elif defined(_WIN32) || defined(_WIN64)
	if (vp->vbuf.fd != ACL_FILE_INVALID && vp->vbuf.hmap != NULL) {
		UnmapViewOfFile(vp->vbuf.data);
		CloseHandle(vp->vbuf.hmap);
	}
#endif
	else if (vp->dbuf == NULL)
		acl_myfree(vp->vbuf.data);
	vp->vbuf.data = NULL;
}

/* acl_vstring_alloc - create variable-length string */

ACL_VSTRING *acl_vstring_alloc(size_t len)
{
	return acl_vstring_slice_alloc(NULL, len);
}

ACL_VSTRING *acl_vstring_slice_alloc(ACL_SLICE_POOL *slice, size_t len)
{
	ACL_VSTRING *vp;

	if (len < 1)
		len = 64;

	if (slice) {
		vp = (ACL_VSTRING*) acl_slice_pool_alloc(__FILE__, __LINE__,
			slice, sizeof(*vp));
		vp->slice = slice;
		vp->dbuf = NULL;
		vp->vbuf.data = (unsigned char *) acl_slice_pool_alloc(
			__FILE__, __LINE__, slice, len);
	} else {
		vp = (ACL_VSTRING *) acl_mymalloc(sizeof(*vp));
		vp->slice = NULL;
		vp->dbuf = NULL;
		vp->vbuf.data = (unsigned char *) acl_mymalloc(len);
	}

#if defined(_WIN32) || defined(_WIN64)
	vp->vbuf.hmap  = NULL;
#endif
	vp->vbuf.flags = 0;
	vp->vbuf.len = (int) len;
	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space     = vstring_buf_space;
/*	vp->vbuf.ctx = NULL; */
	vp->maxlen  = 0;
	vp->vbuf.fd = ACL_FILE_INVALID;

	return vp;
}

ACL_VSTRING *acl_vstring_dbuf_alloc(ACL_DBUF_POOL *dbuf, size_t len)
{
	ACL_VSTRING *vp;

	if (len < 1)
		len = 64;

	if (dbuf) {
		vp = (ACL_VSTRING*) acl_dbuf_pool_alloc(dbuf, sizeof(*vp));
		vp->dbuf = dbuf;
		vp->slice = NULL;
		vp->vbuf.data = (unsigned char *)
			acl_dbuf_pool_alloc(dbuf, len);
	} else {
		vp = (ACL_VSTRING *) acl_mymalloc(sizeof(*vp));
		vp->slice = NULL;
		vp->dbuf = NULL;
		vp->vbuf.data = (unsigned char *) acl_mymalloc(len);
	}

#if defined(_WIN32) || defined(_WIN64)
	vp->vbuf.hmap  = NULL;
#endif
	vp->vbuf.flags = 0;
	vp->vbuf.len = (int) len;
	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space     = vstring_buf_space;
/*	vp->vbuf.ctx = NULL; */
	vp->maxlen  = 0;
	vp->vbuf.fd = ACL_FILE_INVALID;

	return vp;
}

static void mmap_buf_init(ACL_VSTRING *vp)
{
#ifdef ACL_UNIX
	if (acl_lseek(vp->vbuf.fd, vp->vbuf.len, SEEK_SET) != vp->vbuf.len)
		acl_msg_fatal("lseek failed: %s, off: %ld",
			acl_last_serror(), (long) vp->vbuf.len);
	if (acl_file_write(vp->vbuf.fd, "\0", 1, 0, NULL, NULL) == ACL_VSTREAM_EOF)
		acl_msg_fatal("write error: %s", acl_last_serror());
#endif

#ifdef ACL_UNIX
	vp->vbuf.data = (unsigned char*) mmap(NULL, vp->maxlen,
			PROT_READ | PROT_WRITE, MAP_SHARED, vp->vbuf.fd, 0);
	if ((void *) vp->vbuf.data == MAP_FAILED)
		acl_msg_fatal("mmap error: %s", acl_last_serror());
#elif defined(_WIN32) || defined(_WIN64)
	vp->vbuf.hmap = CreateFileMapping(vp->vbuf.fd, NULL, PAGE_READWRITE, 0,
		(DWORD) vp->maxlen, NULL);
	if (vp->vbuf.hmap == NULL)
		acl_msg_fatal("CreateFileMapping: %s", acl_last_serror());
	vp->vbuf.data = (unsigned char *) MapViewOfFile(vp->vbuf.hmap,
		FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);
	if (vp->vbuf.data == NULL)
		acl_msg_fatal("MapViewOfFile error: %s", acl_last_serror());
#else
	acl_msg_fatal("%s: not supported yet!", __FUNCTION__);
#endif
}

ACL_VSTRING *acl_vstring_mmap_alloc(ACL_FILE_HANDLE fd,
	ssize_t max_len, ssize_t init_len)
{
	const char *myname = "acl_vstring_mmap_alloc";
	ACL_VSTRING *vp;

	if (init_len < 1)
		acl_msg_panic("%s: bad length %ld", myname, (long) init_len);

	if (max_len < init_len)
		max_len = init_len;

	vp = (ACL_VSTRING *) acl_mymalloc(sizeof(*vp));

	vp->vbuf.fd = fd;
	vp->slice   = NULL;
	vp->dbuf = NULL;

	vp->vbuf.flags = 0;
	vp->vbuf.len = init_len;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space = vstring_buf_space;
/*	vp->vbuf.ctx = NULL; */
	vp->maxlen = max_len;

	mmap_buf_init(vp);

	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	return vp;
}

/* acl_vstring_free - destroy variable-length string */

void acl_vstring_free(ACL_VSTRING *vp)
{
	acl_vstring_free_buf(vp);

	if (vp->slice)
		acl_slice_pool_free(__FILE__, __LINE__, vp);
#ifdef ACL_UNIX
	else if (vp->vbuf.fd != ACL_FILE_INVALID)
		acl_myfree(vp);
#elif defined(_WIN32) || defined(_WIN64)
	else if (vp->vbuf.hmap != NULL)
		acl_myfree(vp);
#endif
	else if (vp->dbuf == NULL)
		acl_myfree(vp);
}

/* acl_vstring_ctl - modify memory management policy */

void  acl_vstring_ctl(ACL_VSTRING *vp,...)
{
	const char *myname = "acl_vstring_ctl";
	va_list ap;
	int     code;

	va_start(ap, vp);
	while ((code = va_arg(ap, int)) != ACL_VSTRING_CTL_END) {
		switch (code) {
		default:
			acl_msg_panic("%s: unknown code: %d", myname, code);
			break;
		case ACL_VSTRING_CTL_MAXLEN:
			vp->maxlen = va_arg(ap, int);
			if (vp->maxlen < 0)
				acl_msg_panic("%s: bad max length %ld",
					myname, (long) vp->maxlen);
			break;
		}
	}
	va_end(ap);
}

/* acl_vstring_truncate - truncate string */

ACL_VSTRING *acl_vstring_truncate(ACL_VSTRING *vp, size_t len)
{
	if (len < ACL_VSTRING_LEN(vp)) {
		ACL_VSTRING_AT_OFFSET(vp, (int) len);
		ACL_VSTRING_TERMINATE(vp);
	}
	return vp;
}

/* acl_vstring_strcpy - copy string */

ACL_VSTRING *acl_vstring_strcpy(ACL_VSTRING *vp, const char *src)
{
	return acl_vstring_memcpy(vp, src, strlen(src));
}

/* acl_vstring_strncpy - copy string of limited length */

ACL_VSTRING *acl_vstring_strncpy(ACL_VSTRING *vp, const char *src, size_t len)
{
	size_t n = strlen(src);

	if (n > len)
		n = len;
	return acl_vstring_memcpy(vp, src, n);
}

/* acl_vstring_strcat - append string */

ACL_VSTRING *acl_vstring_strcat(ACL_VSTRING *vp, const char *src)
{
	return acl_vstring_memcat(vp, src, strlen(src));
}

/* acl_vstring_strncat - append string of limited length */

ACL_VSTRING *acl_vstring_strncat(ACL_VSTRING *vp, const char *src, size_t len)
{
	size_t n = strlen(src);

	if (n > len)
		n = len;
	return acl_vstring_memcat(vp, src, n);
}

/* acl_vstring_memcpy - copy buffer of limited length */

ACL_VSTRING *acl_vstring_memcpy(ACL_VSTRING *vp, const char *src, size_t len)
{
	ACL_VSTRING_RESET(vp);

	if (len > 0) {
		ssize_t n;

		ACL_VSTRING_SPACE(vp, (ssize_t) len);

		n = acl_vstring_avail(vp);

		if ((size_t) n >= len)
			n = (ssize_t) len;
		else
			acl_msg_warn("%s(%d): space not enough, avail: %ld, "
				"len: %ld", __FUNCTION__, __LINE__,
				(long) n, (long) len);

		if (n > 0) {
			memcpy(acl_vstring_str(vp), src, n);
			ACL_VSTRING_AT_OFFSET(vp, n);
		} else
			acl_msg_warn("%s(%d): no space, avail: 0, len: %ld",
				__FUNCTION__, __LINE__, (long) len);
	}

	ACL_VSTRING_TERMINATE(vp);
	return vp;
}

/* acl_vstring_memmove - move buffer of limited length */

ACL_VSTRING *acl_vstring_memmove(ACL_VSTRING *vp, const char *src, size_t len)
{
	if (len == 0)
		return vp;

	if (src >= acl_vstring_str(vp)
		&& (src + len <= acl_vstring_str(vp) + ACL_VSTRING_SIZE(vp)))
	{
		/* 说明是同一内存区间的数据移动 */
		memmove(acl_vstring_str(vp), src, len);
		ACL_VSTRING_AT_OFFSET(vp, (int) len);
		ACL_VSTRING_TERMINATE(vp);
		return vp;
	}

	/* 说明不是同一内存区间的数据移动 */

	acl_vstring_free_buf(vp);

	vp->vbuf.len = (ssize_t) len;

	if (vp->slice != NULL)
		vp->vbuf.data = (unsigned char *) acl_slice_pool_alloc(
			__FILE__, __LINE__, vp->slice, len);
	else if (vp->dbuf != NULL)
		vp->vbuf.data = (unsigned char *)
			acl_dbuf_pool_alloc(vp->dbuf, len);
#ifdef ACL_UNIX
	else if (vp->vbuf.fd != ACL_FILE_INVALID) {
		if (len > (size_t) vp->maxlen)
			vp->maxlen = (ssize_t) len;
		mmap_buf_init(vp);
	}
#elif defined(_WIN32) || defined(_WIN64)
	else if (vp->vbuf.fd != ACL_FILE_INVALID && vp->vbuf.hmap != NULL) {
		if (len > (size_t) vp->maxlen)
			vp->maxlen = (ssize_t) len;
		mmap_buf_init(vp);
	}
#endif
	else
		vp->vbuf.data = acl_mymalloc(len);

	memcpy(vp->vbuf.data, src, len);
	ACL_VSTRING_AT_OFFSET(vp, (ssize_t) len);
	ACL_VSTRING_TERMINATE(vp);

	return vp;
}

/* acl_vstring_memcat - append buffer of limited length */

ACL_VSTRING *acl_vstring_memcat(ACL_VSTRING *vp, const char *src, size_t len)
{
	if (len > 0) {
		ssize_t n;

		ACL_VSTRING_SPACE(vp, (ssize_t) len);

		n = acl_vstring_avail(vp);

		if ((size_t) n >= len)
			n = (ssize_t) len;
		else
			acl_msg_warn("%s(%d): space not enough, avail: %ld, "
				"len: %ld", __FUNCTION__, __LINE__,
				(long) n, (long) len);

		if (n > 0 ) {
			memcpy(acl_vstring_end(vp), src, n);
			n += (ssize_t) ACL_VSTRING_LEN(vp);
			ACL_VSTRING_AT_OFFSET(vp, n);
		} else
			acl_msg_warn("%s(%d): no space, avail: 0, len: %ld",
				__FUNCTION__, __LINE__, (long) len);
	}

	ACL_VSTRING_TERMINATE(vp);
	return vp;
}

/* acl_vstring_memchr - locate byte in buffer */

char *acl_vstring_memchr(ACL_VSTRING *vp, int ch)
{
	unsigned char *cp;

	for (cp = (unsigned char *) acl_vstring_str(vp);
		cp < (unsigned char *) acl_vstring_end(vp); cp++) {
		if (*cp == ch)
			return ((char *) cp);
	}
	return NULL;
}

/* acl_vstring_strstr - locate byte in buffer */

char *acl_vstring_strstr(ACL_VSTRING *vp, const char *needle)
{
	unsigned char *cp, *startn = 0;
	const unsigned char *np = 0;

	if (vp == NULL || needle == NULL || *needle == 0)
		return NULL;

	for (cp = (unsigned char *) acl_vstring_str(vp);
		cp < (unsigned char *) acl_vstring_end(vp); cp++) {
		if (np) {
			if (*cp == *np) {
				if (!*++np)
					return ((char *) startn);
			} else
				np = 0;
		}
		if (!np && *cp == *((const unsigned char *)needle)) {
			np = (const unsigned char *) needle + 1;
			if (*np == 0)
				return ((char *) cp);
			startn = cp;
		}
	}

	return NULL;
}

/* acl_vstring_strcasestr - locate byte in buffer */

char *acl_vstring_strcasestr(ACL_VSTRING *vp, const char *needle)
{
	const unsigned char *cm = maptolower;
	unsigned char *cp, *startn = 0;
	const unsigned char *np = 0;

	if (vp == NULL || needle == NULL || *needle == 0)
		return NULL;

	for (cp = (unsigned char *) acl_vstring_str(vp);
		cp < (unsigned char *) acl_vstring_end(vp); cp++) {
		if (np) {
			if (cm[*cp] == cm[*np]) {
				if (!*++np)
					return ((char *) startn);
			} else
				np = 0;
		}
		if (!np && cm[*cp] == cm[*((const unsigned char *)needle)]) {
			np = (const unsigned char *) needle + 1;
			if (*np == 0)
				return ((char *) cp);
			startn = cp;
		}
	}

	return NULL;
}

/* acl_vstring_rstrstr - locate byte in buffer */

char *acl_vstring_rstrstr(ACL_VSTRING *vp, const char *needle)
{
	unsigned char *cp;
	const unsigned char *np = 0, *needle_end;

	if (vp == NULL || needle == NULL || *needle == 0)
		return NULL;

	needle_end = (const unsigned char *) needle + strlen(needle) - 1;

	for (cp = (unsigned char *) acl_vstring_end(vp) - 1;
		cp >= (unsigned char *) acl_vstring_str(vp); cp--) {
		if (np) {
			if (*cp == *np) {
				if (--np < (const unsigned char *) needle)
					return ((char *) cp);
			} else
				np = 0;
		}
		if (!np && *cp == *needle_end) {
			np = needle_end - 1;
			if (np < (const unsigned char *) needle)
				return ((char *) cp);
		}
	}

	return NULL;
}

/* acl_vstring_rstrcasestr - locate byte in buffer */

char *acl_vstring_rstrcasestr(ACL_VSTRING *vp, const char *needle)
{
	const unsigned char *cm = maptolower;
	unsigned char *cp;
	const unsigned char *np = 0, *needle_end;

	if (vp == NULL || needle == NULL || *needle == 0)
		return NULL;

	needle_end = (const unsigned char *) needle + strlen(needle) - 1;

	for (cp = (unsigned char *) acl_vstring_end(vp) - 1;
		cp >= (unsigned char *) acl_vstring_str(vp); cp--) {
		if (np) {
			if (*cp == cm[*np]) {
				if (--np < (const unsigned char *) needle)
					return ((char *) cp);
			} else
				np = 0;
		}
		if (!np && *cp == cm[*needle_end]) {
			np = needle_end - 1;
			if (np < (const unsigned char *) needle)
				return (char *) cp;
		}
	}

	return NULL;
}

/* acl_vstring_insert - insert text into string */

ACL_VSTRING *acl_vstring_insert(ACL_VSTRING *vp, size_t start,
	const char *buf, size_t len)
{
	const char *myname = "acl_vstring_insert";
	size_t new_len, n;

	/*
	 * Sanity check.
	 */
	if (start >= ACL_VSTRING_LEN(vp))
		acl_msg_panic("%s(%d): bad start %ld",
			myname, __LINE__, (long) start);

	/*
	 * Move the existing content and copy the new content.
	 */
	new_len = ACL_VSTRING_LEN(vp) + len;
	ACL_VSTRING_SPACE(vp, (ssize_t) len);

	n = acl_vstring_avail(vp);
	if (len > (size_t) n)
		len = n;

	if (len > 0) {
		memmove(acl_vstring_str(vp) + start + len,
			acl_vstring_str(vp) + start,
			ACL_VSTRING_LEN(vp) - start);
		memcpy(acl_vstring_str(vp) + start, buf, len);
		ACL_VSTRING_AT_OFFSET(vp, (int) new_len);
		ACL_VSTRING_TERMINATE(vp);
	}

	return vp;
}

/* acl_vstring_prepend - prepend text to string */

ACL_VSTRING *acl_vstring_prepend(ACL_VSTRING *vp, const char *buf, size_t len)
{
	ssize_t new_len, n;

	/*
	 * Move the existing content and copy the new content.
	 */
	new_len = (ssize_t) (ACL_VSTRING_LEN(vp) + len);
	ACL_VSTRING_SPACE(vp, (int) len);

	n = acl_vstring_avail(vp);
	if (len > (size_t) n)
		len = (size_t) n;

	if (len > 0) {
		memmove(acl_vstring_str(vp) + len, acl_vstring_str(vp),
			ACL_VSTRING_LEN(vp));
		memcpy(acl_vstring_str(vp), buf, len);
		ACL_VSTRING_AT_OFFSET(vp, new_len);
		ACL_VSTRING_TERMINATE(vp);
	}

	return vp;
}

/* acl_vstring_export - VSTRING to bare string */

char   *acl_vstring_export(ACL_VSTRING *vp)
{
	char   *cp;

	cp = (char *) vp->vbuf.data;
	vp->vbuf.data = 0;
	acl_myfree(vp);
	return cp;
}

/* acl_vstring_import - bare string to vstring */

ACL_VSTRING *acl_vstring_import(char *str)
{
	ACL_VSTRING *vp;
	int     len;

	vp = (ACL_VSTRING *) acl_mymalloc(sizeof(*vp));
	vp->slice = NULL;
	len = (int) strlen(str);
	vp->vbuf.data = (unsigned char *) str;
	vp->vbuf.len = len + 1;
	ACL_VSTRING_AT_OFFSET(vp, len);
	vp->maxlen = 0;
	return vp;
}

void acl_vstring_glue(ACL_VSTRING *vp, void *buf, size_t len)
{
	vp->vbuf.flags = 0;
	vp->vbuf.data = buf;
	vp->vbuf.len = (int) len;
	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space = vstring_buf_space;
	vp->maxlen = 0;
}

/* acl_vstring_charat -- get the position char */

char acl_vstring_charat(ACL_VSTRING *vp, size_t offset)
{
	const char *myname = "acl_vstring_charat";

	if (vp == NULL)
		acl_msg_fatal("%s(%d): invalid input", myname, __LINE__);
	if (offset >= ACL_VSTRING_LEN(vp))
		acl_msg_fatal("%s(%d): offset(%d) >= strlen(%d)",
			myname, __LINE__, (int) offset,
			(int) ACL_VSTRING_LEN(vp));
	return (ACL_VBUF_CHARAT(vp->vbuf, offset));
}

/* acl_vstring_sprintf - formatted string */

ACL_VSTRING *acl_vstring_sprintf(ACL_VSTRING *vp, const char *format,...)
{
	va_list ap;

	va_start(ap, format);
	vp = acl_vstring_vsprintf(vp, format, ap);
	va_end(ap);
	return vp;
}

/* acl_vstring_vsprintf - format string, vsprintf-like interface */

ACL_VSTRING *acl_vstring_vsprintf(ACL_VSTRING *vp, const char *format, va_list ap)
{
	ACL_VSTRING_RESET(vp);
	acl_vbuf_print(&vp->vbuf, format, ap);
	ACL_VSTRING_TERMINATE(vp);
	return vp;
}

/* acl_vstring_sprintf_append - append formatted string */

ACL_VSTRING *acl_vstring_sprintf_append(ACL_VSTRING *vp, const char *format,...)
{
	va_list ap;

	va_start(ap, format);
	vp = acl_vstring_vsprintf_append(vp, format, ap);
	va_end(ap);
	return vp;
}

/* acl_vstring_vsprintf_append - append format string, vsprintf-like interface */

ACL_VSTRING *acl_vstring_vsprintf_append(ACL_VSTRING *vp, const char *format, va_list ap)
{
	acl_vbuf_print(&vp->vbuf, format, ap);
	ACL_VSTRING_TERMINATE(vp);
	return vp;
}

/* acl_vstring_sprintf_prepend - format + prepend string, vsprintf-like interface */

ACL_VSTRING *acl_vstring_sprintf_prepend(ACL_VSTRING *vp, const char *format,...)
{
	va_list ap;
	ssize_t old_len = (ssize_t) ACL_VSTRING_LEN(vp);
	ssize_t result_len;

	/* Construct: old|new|free */
	va_start(ap, format);
	vp = acl_vstring_vsprintf_append(vp, format, ap);
	va_end(ap);
	result_len = (ssize_t) ACL_VSTRING_LEN(vp);

	/* Construct: old|new|old|free */
	ACL_VSTRING_SPACE(vp, old_len);  /* avoid dangling pointer */
	acl_vstring_memcat(vp, acl_vstring_str(vp), old_len);

	/* Construct: new|old|free */
	memmove(acl_vstring_str(vp), acl_vstring_str(vp) + old_len, result_len);
	ACL_VSTRING_AT_OFFSET(vp, result_len);
	ACL_VSTRING_TERMINATE(vp);
	return vp;
}

const ACL_VSTRING *acl_buffer_gets_nonl(ACL_VSTRING *vp, const char **src, size_t dlen)
{
	const char *myname = "acl_buffer_gets_nonl";
	const char *ptr, *pend, *pbegin = *src;

	if (dlen <= 0) {
		acl_msg_warn("%s(%d): dlen(%d) invalid",
			myname, __LINE__, (int) dlen);
		return NULL;
	}

	ptr = memchr(pbegin, '\n', dlen);
	if (ptr == NULL) {
		acl_vstring_memcat(vp, pbegin, dlen);
		ACL_VSTRING_TERMINATE(vp);
		*src += dlen;  /* 移动 *src 指针位置 */
		return NULL;
	}
	*src = ptr + 1;  /* 移动 *src 指针位置 */
	pend = ptr;

	/* 去除多余的 \r\n */
	while (pend >= pbegin) {
		if (*pend != '\r' && *pend != '\n')
			break;
		pend--;
	}
	if (pend < pbegin) {
		/* 说明 data 中只包括 \r, \n */
		ACL_VSTRING_TERMINATE(vp);
		return vp;
	}
	acl_vstring_memcat(vp, pbegin, pend - pbegin + 1);
	ACL_VSTRING_TERMINATE(vp);
	return vp;
}

const ACL_VSTRING *acl_buffer_gets(ACL_VSTRING *vp, const char **src, size_t dlen)
{
	const char *myname = "acl_buffer_gets";
	const char *ptr;

	if (dlen <= 0) {
		acl_msg_warn("%s(%d): dlen(%d) invalid",
			myname, __LINE__, (int) dlen);
		return NULL;
	}

	ptr = memchr(*src, '\n', dlen);
	if (ptr == NULL) {
		acl_vstring_memcat(vp, *src, dlen);
		ACL_VSTRING_TERMINATE(vp);
		*src += dlen;
		return NULL;
	}

	acl_vstring_memcat(vp, *src, ptr - *src + 1);
	ACL_VSTRING_TERMINATE(vp);
	*src = ptr + 1;  /* 修改 *src 指针位置 */
	return vp;
}
