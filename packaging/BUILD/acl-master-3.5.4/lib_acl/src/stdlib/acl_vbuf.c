#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System library. */

#include "stdlib/acl_define.h"

#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_vbuf.h"

#endif

/* vbuf_unget - implement at least one character pushback */

int acl_vbuf_unget(ACL_VBUF *bp, int ch)
{
	if ((ch & 0xff) != ch || -bp->cnt >= bp->len) {
		bp->flags |= ACL_VBUF_FLAG_ERR;
		return (ACL_VBUF_EOF);
	} else {
		bp->cnt--;
		bp->flags &= ~ACL_VBUF_FLAG_EOF;
		return ((*--bp->ptr = ch));
	}
}

/* vbuf_get - handle read buffer empty condition */

int acl_vbuf_get(ACL_VBUF *bp)
{
	return (bp->get_ready(bp) ? ACL_VBUF_EOF : ACL_VBUF_GET(bp));
}

/* vbuf_put - handle write buffer full condition */

int acl_vbuf_put(ACL_VBUF *bp, int ch)
{
	return (bp->put_ready(bp) ? ACL_VBUF_EOF : ACL_VBUF_PUT(bp, ch));
}

/* vbuf_read - bulk read from buffer */

int acl_vbuf_read(ACL_VBUF *bp, char *buf, int len)
{
    char   *cp;
	ssize_t n, count;

#if 0
	for (count = 0; count < len; count++)
		if ((buf[count] = ACL_VBUF_GET(bp)) < 0)
			break;
	return (count);
#else
	for (cp = buf, count = len; count > 0; cp += n, count -= n) {
		if (bp->cnt >= 0 && bp->get_ready(bp))
			break;
		n = (count < -bp->cnt ? count : -bp->cnt);
		memcpy(cp, bp->ptr, n);
		bp->ptr += n;
		bp->cnt += n;
	}
	return (int) (len - count);
#endif
}

/* vbuf_write - bulk write to buffer */

int acl_vbuf_write(ACL_VBUF *bp, const char *buf, int len)
{
	const char *cp;
	ssize_t n, count;

#if 0
	for (count = 0; count < len; count++)
		if (ACL_VBUF_PUT(bp, buf[count]) < 0)
			break;
	return (count);
#else
	for (cp = buf, count = len; count > 0; cp += n, count -= n) {
		if (bp->cnt <= 0 && bp->put_ready(bp) != 0)
			break;
		n = (count < bp->cnt ? count : bp->cnt);
		memcpy(bp->ptr, cp, n);
		bp->ptr += n;
		bp->cnt -= n;
	}
	return (int) (len - count);
#endif
}
