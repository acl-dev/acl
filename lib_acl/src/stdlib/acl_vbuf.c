#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System library. */

#include "stdlib/acl_define.h"

#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_vstring.h"
#include "stdlib/acl_vbuf.h"

#endif

int acl_vbuf_space(ACL_VBUF *bp, ssize_t len)
{
	return acl_vstring_space(bp, len);
}

/* vbuf_put - handle write buffer full condition */

int acl_vbuf_put(ACL_VBUF *bp, int ch)
{
	return acl_vstring_put_ready(bp) ? ACL_VBUF_EOF : ACL_VBUF_PUT(bp, ch);
}

/* vbuf_write - bulk write to buffer */

int acl_vbuf_write(ACL_VBUF *bp, const char *buf, int len)
{
	const char *cp;
	ssize_t n, count, dlen;

	for (cp = buf, count = len; count > 0; cp += n, count -= n) {
		if (bp->ptr >= bp->data + bp->len
			 && acl_vstring_put_ready(bp) != 0) {
			break;
		}
		dlen = bp->ptr - bp->data;
		n = count < dlen ? count : dlen;
		memcpy(bp->ptr, cp, n);
		bp->ptr += n;
	}
	return (int) (len - count);
}
