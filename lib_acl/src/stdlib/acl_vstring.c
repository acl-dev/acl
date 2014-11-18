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
#include "stdlib/acl_vbuf_print.h"
#include "stdlib/acl_vstring.h"

#endif

#include "charmap.h"

/* vstring_extend - variable-length string buffer extension policy */

static void vstring_extend(ACL_VBUF *bp, int incr)
{
	const char *myname = "vstring_extend";
	unsigned used = bp->ptr - bp->data;
	int     new_len;
	ACL_VSTRING *vp = (ACL_VSTRING *) bp->ctx;

	if (vp->maxlen > 0 && (int) ACL_VSTRING_LEN(vp) > vp->maxlen)
		acl_msg_warn("%s(%d), %s: reached the maxlen: %d",
			__FILE__, __LINE__, myname, vp->maxlen);

	/*
	 * Note: vp->vbuf.len is the current buffer size (both on entry and on
	 * exit of this routine). We round up the increment size to the buffer
	 * size to avoid silly little buffer increments. With really large
	 * strings we might want to abandon the length doubling strategy, and go
	 * to fixed increments.
	 */
	new_len = bp->len + (bp->len > incr ? bp->len : incr);
	if (vp->slice)
		bp->data = (unsigned char *) acl_slice_pool_realloc(
			__FILE__, __LINE__, vp->slice, bp->data, new_len);
	else
		bp->data = (unsigned char *) acl_myrealloc(bp->data, new_len);
	bp->len = new_len;
	bp->ptr = bp->data + used;
	bp->cnt = bp->len - used;
}

/* vstring_buf_get_ready - vbuf callback for read buffer empty condition */

static int vstring_buf_get_ready(ACL_VBUF *buf acl_unused)
{
	acl_msg_panic("vstring_buf_get: write-only buffer");
	return (0);
}

/* vstring_buf_put_ready - vbuf callback for write buffer full condition */

static int vstring_buf_put_ready(ACL_VBUF *bp)
{
	vstring_extend(bp, 0);
	return (0);
}

/* vstring_buf_space - vbuf callback to reserve space */

static int vstring_buf_space(ACL_VBUF *bp, int len)
{
	int     need;

	if (len < 0)
		acl_msg_panic("vstring_buf_space: bad length %d", len);
	if ((need = len - bp->cnt) > 0)
		vstring_extend(bp, need);
	return (0);
}

void acl_vstring_init(ACL_VSTRING *vp, size_t len)
{
	if (len < 1)
		acl_msg_panic("acl_vstring_alloc: bad input, vp null or len < 1");

	vp->vbuf.flags = 0;
	vp->vbuf.len = 0;
	vp->vbuf.data = (unsigned char *) acl_mymalloc(len);
	vp->vbuf.len = len;
	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space = vstring_buf_space;
	vp->vbuf.ctx = vp;
	vp->maxlen = 0;
	vp->slice = NULL;
}

void acl_vstring_free_buf(ACL_VSTRING *vp)
{
	if (vp->vbuf.data) {
		acl_myfree(vp->vbuf.data);
		vp->vbuf.data = NULL;
	}
}

/* acl_vstring_alloc - create variable-length string */

ACL_VSTRING *acl_vstring_alloc(size_t len)
{
	return (acl_vstring_alloc2(NULL, len));
}

ACL_VSTRING *acl_vstring_alloc2(ACL_SLICE_POOL *slice, size_t len)
{
	ACL_VSTRING *vp;

	if (len < 1)
		acl_msg_panic("acl_vstring_alloc: bad length %d", (int) len);
	if (slice) {
		vp = (ACL_VSTRING*) acl_slice_pool_alloc(__FILE__, __LINE__,
			slice, sizeof(*vp));
		vp->slice = slice;
	} else {
		vp = (ACL_VSTRING *) acl_mymalloc(sizeof(*vp));
		vp->slice = NULL;
	}
	vp->vbuf.flags = 0;
	vp->vbuf.len = 0;
	if (vp->slice)
		vp->vbuf.data = (unsigned char *) acl_slice_pool_alloc(
			__FILE__, __LINE__, vp->slice, len);
	else
		vp->vbuf.data = (unsigned char *) acl_mymalloc(len);
	vp->vbuf.len = len;
	ACL_VSTRING_RESET(vp);
	vp->vbuf.data[0] = 0;
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space = vstring_buf_space;
	vp->vbuf.ctx = vp;
	vp->maxlen = 0;
	return (vp);
}

/* acl_vstring_free - destroy variable-length string */

ACL_VSTRING *acl_vstring_free(ACL_VSTRING *vp)
{
	if (vp->vbuf.data) {
		if (vp->slice)
			acl_slice_pool_free(__FILE__, __LINE__, vp->vbuf.data);
		else
			acl_myfree(vp->vbuf.data);
	}
	if (vp->slice)
		acl_slice_pool_free(__FILE__, __LINE__, vp);
	else
		acl_myfree(vp);
	return (0);
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
		case ACL_VSTRING_CTL_MAXLEN:
			vp->maxlen = va_arg(ap, int);
			if (vp->maxlen < 0)
				acl_msg_panic("%s: bad max length %d",
					myname, vp->maxlen);
			break;
		}
	}
	va_end(ap);
}

/* acl_vstring_truncate - truncate string */

ACL_VSTRING *acl_vstring_truncate(ACL_VSTRING *vp, size_t len)
{
	if (len < ACL_VSTRING_LEN(vp)) {
		ACL_VSTRING_AT_OFFSET(vp, len);
		ACL_VSTRING_TERMINATE(vp);
	}
	return (vp);
}

/* acl_vstring_strcpy - copy string */

ACL_VSTRING *acl_vstring_strcpy(ACL_VSTRING *vp, const char *src)
{
	ACL_VSTRING_RESET(vp);

	while (*src) {
		ACL_VSTRING_ADDCH(vp, *src);
		src++;
	}
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_strncpy - copy string of limited length */

ACL_VSTRING *acl_vstring_strncpy(ACL_VSTRING *vp, const char *src, size_t len)
{
	ACL_VSTRING_RESET(vp);

	while (len-- > 0 && *src) {
		ACL_VSTRING_ADDCH(vp, *src);
		src++;
	}
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_strcat - append string */

ACL_VSTRING *acl_vstring_strcat(ACL_VSTRING *vp, const char *src)
{
	while (*src) {
		ACL_VSTRING_ADDCH(vp, *src);
		src++;
	}
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_strncat - append string of limited length */

ACL_VSTRING *acl_vstring_strncat(ACL_VSTRING *vp, const char *src, size_t len)
{
	while (len-- > 0 && *src) {
		ACL_VSTRING_ADDCH(vp, *src);
		src++;
	}
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_memcpy - copy buffer of limited length */

ACL_VSTRING *acl_vstring_memcpy(ACL_VSTRING *vp, const char *src, size_t len)
{
	ACL_VSTRING_RESET(vp);

	ACL_VSTRING_SPACE(vp, len);
	memcpy(acl_vstring_str(vp), src, len);
	ACL_VSTRING_AT_OFFSET(vp, len);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_memmove - move buffer of limited length */

ACL_VSTRING *acl_vstring_memmove(ACL_VSTRING *vp, const char *src, size_t len)
{
	if (src >= acl_vstring_str(vp)
		&& (src + len <= acl_vstring_str(vp) + ACL_VSTRING_SIZE(vp)))
	{
		/* 说明是同一内存区间的数据移动 */
		memmove(acl_vstring_str(vp), src, len);
		ACL_VSTRING_AT_OFFSET(vp, len);
		ACL_VSTRING_TERMINATE(vp);
		return (vp);
	} else {
		/* 说明不是同一内存区间的数据移动 */
		char *ptr = acl_mymalloc(len);

		memcpy(ptr, src, len);
		acl_myfree(vp->vbuf.data);
		vp->vbuf.data = (unsigned char *) ptr;
		vp->vbuf.len = len;
		ACL_VSTRING_AT_OFFSET(vp, len);
		ACL_VSTRING_TERMINATE(vp);
		vp->maxlen = 0;
		return (vp);
	}
}

/* acl_vstring_memcat - append buffer of limited length */

ACL_VSTRING *acl_vstring_memcat(ACL_VSTRING *vp, const char *src, size_t len)
{
	ACL_VSTRING_SPACE(vp, len);
	memcpy(acl_vstring_end(vp), src, len);
	len += ACL_VSTRING_LEN(vp);
	ACL_VSTRING_AT_OFFSET(vp, len);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
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
	return (NULL);
}

/* acl_vstring_strstr - locate byte in buffer */

char *acl_vstring_strstr(ACL_VSTRING *vp, const char *needle)
{
	unsigned char *cp, *startn = 0;
	const unsigned char *np = 0;

	if (vp == NULL || needle == NULL || *needle == 0)
		return (NULL);

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

	return (NULL);
}

/* acl_vstring_strcasestr - locate byte in buffer */

char *acl_vstring_strcasestr(ACL_VSTRING *vp, const char *needle)
{
	const unsigned char *cm = maptolower;
	unsigned char *cp, *startn = 0;
	const unsigned char *np = 0;

	if (vp == NULL || needle == NULL || *needle == 0)
		return (NULL);

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

	return (NULL);
}

/* acl_vstring_rstrstr - locate byte in buffer */

char *acl_vstring_rstrstr(ACL_VSTRING *vp, const char *needle)
{
	unsigned char *cp;
	const unsigned char *np = 0, *needle_end;

	if (vp == NULL || needle == NULL || *needle == 0)
		return (NULL);

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

	return (NULL);
}

/* acl_vstring_rstrcasestr - locate byte in buffer */

char *acl_vstring_rstrcasestr(ACL_VSTRING *vp, const char *needle)
{
	const unsigned char *cm = maptolower;
	unsigned char *cp;
	const unsigned char *np = 0, *needle_end;

	if (vp == NULL || needle == NULL || *needle == 0)
		return (NULL);

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
				return ((char *) cp);
		}
	}

	return (NULL);
}

/* acl_vstring_insert - insert text into string */

ACL_VSTRING *acl_vstring_insert(ACL_VSTRING *vp, size_t start,
	const char *buf, size_t len)
{
	const char *myname = "acl_vstring_insert";
	size_t new_len;

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
	ACL_VSTRING_SPACE(vp, len);
	memmove(acl_vstring_str(vp) + start + len,
		acl_vstring_str(vp) + start,
		ACL_VSTRING_LEN(vp) - start);
	memcpy(acl_vstring_str(vp) + start, buf, len);
	ACL_VSTRING_AT_OFFSET(vp, new_len);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_prepend - prepend text to string */

ACL_VSTRING *acl_vstring_prepend(ACL_VSTRING *vp, const char *buf, size_t len)
{
	ssize_t new_len;

	/*
	 * Move the existing content and copy the new content.
	 */
	new_len = ACL_VSTRING_LEN(vp) + len;
	ACL_VSTRING_SPACE(vp, len);
	memmove(acl_vstring_str(vp) + len, acl_vstring_str(vp), ACL_VSTRING_LEN(vp));
	memcpy(acl_vstring_str(vp), buf, len);
	ACL_VSTRING_AT_OFFSET(vp, new_len);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_export - VSTRING to bare string */

char   *acl_vstring_export(ACL_VSTRING *vp)
{
	char   *cp;

	cp = (char *) vp->vbuf.data;
	vp->vbuf.data = 0;
	acl_myfree(vp);
	return (cp);
}

/* acl_vstring_import - bare string to vstring */

ACL_VSTRING *acl_vstring_import(char *str)
{
	ACL_VSTRING *vp;
	int     len;

	vp = (ACL_VSTRING *) acl_mymalloc(sizeof(*vp));
	vp->slice = NULL;
	len = strlen(str);
	vp->vbuf.data = (unsigned char *) str;
	vp->vbuf.len = len + 1;
	ACL_VSTRING_AT_OFFSET(vp, len);
	vp->maxlen = 0;
	return (vp);
}

void acl_vstring_glue(ACL_VSTRING *vp, void *buf, size_t len)
{
	vp->vbuf.flags = 0;
	vp->vbuf.data = buf;
	vp->vbuf.len = len;
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
	return (vp);
}

/* acl_vstring_vsprintf - format string, vsprintf-like interface */

ACL_VSTRING *acl_vstring_vsprintf(ACL_VSTRING *vp, const char *format, va_list ap)
{
	ACL_VSTRING_RESET(vp);
	acl_vbuf_print(&vp->vbuf, format, ap);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_sprintf_append - append formatted string */

ACL_VSTRING *acl_vstring_sprintf_append(ACL_VSTRING *vp, const char *format,...)
{
	va_list ap;

	va_start(ap, format);
	vp = acl_vstring_vsprintf_append(vp, format, ap);
	va_end(ap);
	return (vp);
}

/* acl_vstring_vsprintf_append - append format string, vsprintf-like interface */

ACL_VSTRING *acl_vstring_vsprintf_append(ACL_VSTRING *vp, const char *format, va_list ap)
{
	acl_vbuf_print(&vp->vbuf, format, ap);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

/* acl_vstring_sprintf_prepend - format + prepend string, vsprintf-like interface */

ACL_VSTRING *acl_vstring_sprintf_prepend(ACL_VSTRING *vp, const char *format,...)
{
	va_list ap;
	ssize_t old_len = ACL_VSTRING_LEN(vp);
	ssize_t result_len;

	/* Construct: old|new|free */
	va_start(ap, format);
	vp = acl_vstring_vsprintf_append(vp, format, ap);
	va_end(ap);
	result_len = ACL_VSTRING_LEN(vp);

	/* Construct: old|new|old|free */
	ACL_VSTRING_SPACE(vp, old_len);  /* avoid dangling pointer */
	acl_vstring_memcat(vp, acl_vstring_str(vp), old_len);

	/* Construct: new|old|free */
	memmove(acl_vstring_str(vp), acl_vstring_str(vp) + old_len, result_len);
	ACL_VSTRING_AT_OFFSET(vp, result_len);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

const ACL_VSTRING *acl_buffer_gets_nonl(ACL_VSTRING *vp, const char **src, size_t dlen)
{
	const char *myname = "acl_buffer_gets_nonl";
	const char *ptr, *pend, *pbegin = *src;

	if (dlen <= 0) {
		acl_msg_warn("%s(%d): dlen(%d) invalid",
				myname, __LINE__, (int) dlen);
		return (NULL);
	}

	ptr = memchr(pbegin, '\n', dlen);
	if (ptr == NULL) {
		acl_vstring_memcat(vp, pbegin, dlen);
		ACL_VSTRING_TERMINATE(vp);
		*src += dlen;  /* 移动 *src 指针位置 */
		return (NULL);
	}
	*src = ptr + 1;  /* 移动 *src 指针位置 */
	pend = ptr;

	/* 去除多余的 \r\n */
	while (pend >= pbegin) {
		if (*pend != '\r' && *pend != '\n') {
			break;
		}
		pend--;
	}
	if (pend < pbegin) {
		/* 说明 data 中只包括 \r, \n */
		ACL_VSTRING_TERMINATE(vp);
		return (vp);
	}
	acl_vstring_memcat(vp, pbegin, pend - pbegin + 1);
	ACL_VSTRING_TERMINATE(vp);
	return (vp);
}

const ACL_VSTRING *acl_buffer_gets(ACL_VSTRING *vp, const char **src, size_t dlen)
{
	const char *myname = "acl_buffer_gets";
	const char *ptr;

	if (dlen <= 0) {
		acl_msg_warn("%s(%d): dlen(%d) invalid",
			myname, __LINE__, (int) dlen);
		return (NULL);
	}

	ptr = memchr(*src, '\n', dlen);
	if (ptr == NULL) {
		acl_vstring_memcat(vp, *src, dlen);
		ACL_VSTRING_TERMINATE(vp);
		*src += dlen;
		return (NULL);
	}

	acl_vstring_memcat(vp, *src, ptr - *src + 1);
	ACL_VSTRING_TERMINATE(vp);
	*src = ptr + 1;  /* 修改 *src 指针位置 */
	return (vp);
}
