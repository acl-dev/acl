#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vbuf_print.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_allocator.h"

#endif

#include "allocator.h"

static const char label[] = "vstring";

static ACL_ALLOCATOR *__var_allocator;
static int __len = 1024;

static void vstring_extend(ACL_VBUF *bp, int incr)
{
	unsigned used = (unsigned int) (bp->ptr - bp->data);
	long     new_len;

	/*
	 * Note: vp->vbuf.len is the current buffer size (both on entry and on
	 * exit of this routine). We round up the increment size to the buffer
	 * size to avoid silly little buffer increments. With really large
	 * strings we might want to abandon the length doubling strategy,
	 * and go to fixed increments.
	 */
	new_len = (long) (bp->len + (bp->len > incr ? bp->len : incr));
	/*
	 * bp->data = (unsigned char *) acl_myrealloc((char *) bp->data,
	 * 	new_len);
	 */
	bp->data = acl_allocator_membuf_alloc(__FILE__, __LINE__,
		__var_allocator, new_len);
	bp->len = new_len;
	bp->ptr = bp->data + used;
	bp->cnt = bp->len - used;
}

static int vstring_buf_get_ready(ACL_VBUF *buf acl_unused)
{
	acl_msg_panic("vstring_buf_get: write-only buffer");
	return (0);
}

static int vstring_buf_put_ready(ACL_VBUF *bp)
{
	vstring_extend(bp, 0);
	return (0);
}

static int vstring_buf_space(ACL_VBUF *bp, ssize_t len)
{
	ssize_t need;

	if (len < 0)
		acl_msg_panic("vstring_buf_space: bad length %ld", (long) len);
	if ((need = len - bp->cnt) > 0)
		vstring_extend(bp, (int) need);
	return (0);
}

static void after_alloc_fn(void *obj, void *pool_ctx acl_unused)
{
	ACL_VSTRING *vp = (ACL_VSTRING *) obj;
	char *buf;

	buf = acl_allocator_membuf_alloc(__FILE__, __LINE__,
		__var_allocator, __len);
	acl_vstring_glue(vp, buf, __len);
	vp->vbuf.get_ready = vstring_buf_get_ready;
	vp->vbuf.put_ready = vstring_buf_put_ready;
	vp->vbuf.space = vstring_buf_space;
}

static void before_free_fn(void *obj, void *pool_ctx acl_unused)
{
	ACL_VSTRING *vp = (ACL_VSTRING *) obj;

	acl_allocator_membuf_free(__FILE__, __LINE__,
		__var_allocator, vp->vbuf.data);
	vp->vbuf.data = NULL;
}

void vstring_pool_create(ACL_ALLOCATOR *allocator)
{
	__var_allocator = allocator;

	acl_allocator_pool_add(allocator, label,  sizeof(ACL_VSTRING),
		ACL_MEM_TYPE_VSTRING, after_alloc_fn, before_free_fn, NULL);
}
