#ifndef ACL_VBUF_INCLUDE_H
#define ACL_VBUF_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"

typedef struct ACL_VBUF ACL_VBUF;
typedef int (*ACL_VBUF_GET_READY_FN) (ACL_VBUF *);
typedef int (*ACL_VBUF_PUT_READY_FN) (ACL_VBUF *);
typedef int (*ACL_VBUF_SPACE_FN) (ACL_VBUF *, ssize_t);

struct ACL_VBUF {
    unsigned char *data;		/* variable-length buffer */
    unsigned char *ptr;			/* read/write position */
    ssize_t len;			/* buffer length */
    ssize_t cnt;			/* bytes left to read/write */
    unsigned flags;			/* status, see below */
    ACL_FILE_HANDLE fd;
#if defined(_WIN32) || defined(_WIN64)
    ACL_FILE_HANDLE hmap;
#endif
    ACL_VBUF_GET_READY_FN get_ready;	/* read buffer empty action */
    ACL_VBUF_PUT_READY_FN put_ready;	/* write buffer full action */
    ACL_VBUF_SPACE_FN space;		/* request for buffer space */
    /* void   *ctx; */
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

#define acl_vbuf_error(v)	((v)->flags & ACL_VBUF_FLAG_BAD)
#define acl_vbuf_eof(v)		((v)->flags & ACL_VBUF_FLAG_EOF)
#define acl_vbuf_timeout(v)	((v)->flags & ACL_VBUF_FLAG_TIMEOUT)
#define acl_vbuf_clearerr(v)	((v)->flags &= ~ACL_VBUF_FLAG_BAD)

 /*
  * Buffer I/O-like operations and results.
  */
#define ACL_VBUF_GET(v) ((v)->cnt < 0 ? ++(v)->cnt, \
	(int) *(v)->ptr++ : acl_vbuf_get(v))

#define ACL_VBUF_PUT(v,c) ((v)->cnt > 0 ? --(v)->cnt, \
	(int) (*(v)->ptr++ = (c)) : acl_vbuf_put((v),(c)))

#define ACL_VBUF_SPACE(v,n) ((v)->space((v),(n)))

#define	ACL_VBUF_CHARAT(v, offset) ((int) (v).data[offset])

#define ACL_VBUF_EOF		(-1)		/* no more space or data */

ACL_API int acl_vbuf_get(ACL_VBUF *);
ACL_API int acl_vbuf_put(ACL_VBUF *, int);
ACL_API int acl_vbuf_unget(ACL_VBUF *, int);
ACL_API int acl_vbuf_read(ACL_VBUF *, char *, int);
ACL_API int acl_vbuf_write(ACL_VBUF *, const char *, int);

#ifdef  __cplusplus
}
#endif

#endif
