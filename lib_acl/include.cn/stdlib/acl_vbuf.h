#ifndef ACL_VBUF_INCLUDE_H
#define ACL_VBUF_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

#include "acl_dbuf_pool.h"
#include "acl_slice.h"

typedef struct ACL_VBUF ACL_VBUF;
typedef int (*ACL_VBUF_GET_READY_FN) (ACL_VBUF *);
typedef int (*ACL_VBUF_PUT_READY_FN) (ACL_VBUF *);
typedef int (*ACL_VBUF_SPACE_FN) (ACL_VBUF *, ssize_t);

struct ACL_VBUF {
	unsigned char *data;		/* variable-length buffer */
	unsigned char *ptr;		/* read/write position */
	ssize_t len;			/* buffer length */
	unsigned flags;			/* status, see below */

	ACL_FILE_HANDLE fd;
#if defined(_WIN32) || defined(_WIN64)
	ACL_FILE_HANDLE hmap;
#endif

	union {
		ACL_SLICE_POOL *slice;
		ACL_DBUF_POOL  *dbuf;
	} alloc;

#if 0
	ACL_VBUF_GET_READY_FN get_ready; /* read buffer empty action */
	ACL_VBUF_PUT_READY_FN put_ready; /* write buffer full action */
	ACL_VBUF_SPACE_FN space;         /* request for buffer space */
#endif
};

 /*
  * Typically, an application will embed a VBUF structure into a larger
  * structure that also contains application-specific members. This approach
  * gives us the best of both worlds. The application can still use the
  * generic VBUF primitives for reading and writing VBUFs. The macro below
  * transforms a pointer from VBUF structure to the structure that contains
  * it.
  */
#define ACL_VBUF_TO_APPL(vbuf_ptr,app_type,vbuf_member) \
    ((app_type *) (((char *) (vbuf_ptr)) - offsetof(app_type,vbuf_member)))

 /*
  * Buffer status management.
  */
#define	ACL_VBUF_FLAG_ERR	(1<<0)		/* some I/O error */
#define ACL_VBUF_FLAG_EOF	(1<<1)		/* end of data */
#define ACL_VBUF_FLAG_TIMEOUT	(1<<2)		/* timeout error */
#define ACL_VBUF_FLAG_BAD \
	(ACL_VBUF_FLAG_ERR | ACL_VBUF_FLAG_EOF | ACL_VBUF_FLAG_TIMEOUT)
#define ACL_VBUF_FLAG_FIXED	(1<<3)		/* fixed-size buffer */
#define	ACL_VBUF_FLAG_SLICE	(1<<4)		/* use slice allocator */
#define	ACL_VBUF_FLAG_DBUF	(1<<5)		/* use dbuf allocator */
#define	ACL_VBUF_FLAG_MMAP	(1<<6)		/* use file mmap allocator */

#define acl_vbuf_error(v)	((v)->flags & ACL_VBUF_FLAG_BAD)
#define acl_vbuf_eof(v)		((v)->flags & ACL_VBUF_FLAG_EOF)
#define acl_vbuf_timeout(v)	((v)->flags & ACL_VBUF_FLAG_TIMEOUT)
#define acl_vbuf_clearerr(v)	((v)->flags &= ~ACL_VBUF_FLAG_BAD)

#define ACL_VBUF_PUT(v,c) ((v)->ptr < (v)->data + (v)->len ? \
	(int) (*(v)->ptr++ = (c)) : acl_vbuf_put((v),(c)))

ACL_API int acl_vbuf_space(struct ACL_VBUF *bp, ssize_t len);
#define	ACL_VBUF_SPACE acl_vbuf_space

#define ACL_VBUF_TERM(v) ((v)->ptr < (v)->data + (v)->len ? \
	*(v)->ptr = 0 : ACL_VBUF_SPACE((v), 1), *(v)->ptr = 0)

#define	ACL_VBUF_CHARAT(v, offset) ((int) (v).data[offset])

#define ACL_VBUF_EOF		(-1)		/* no more space or data */

ACL_API int acl_vbuf_put(ACL_VBUF *, int);
ACL_API int acl_vbuf_write(ACL_VBUF *, const char *, int);

#ifdef  __cplusplus
}
#endif

#endif
