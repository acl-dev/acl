#ifndef	__PRIVATE_VSTREAM_INCLUDE_H__
#define	__PRIVATE_VSTREAM_INCLUDE_H__

#include "stdlib/acl_vstream.h"

int private_vstream_getc(ACL_VSTREAM *stream);
int private_vstream_ungetc(ACL_VSTREAM *stream, int ch);
int private_vstream_gets(ACL_VSTREAM *stream, void *vptr, size_t maxlen);
int private_vstream_gets_nonl(ACL_VSTREAM *stream, void *vptr, size_t maxlen);
int private_vstream_readn(ACL_VSTREAM *stream, void *vptr, size_t maxlen);
int private_vstream_read(ACL_VSTREAM *stream, void *vptr, size_t maxlen);
int private_vstream_write(ACL_VSTREAM *stream, const void *vptr, size_t dlen);
int private_vstream_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen);
int private_vstream_buffed_writen(ACL_VSTREAM *stream, const void *vptr, size_t dlen);
int private_vstream_fflush(ACL_VSTREAM *stream);
ACL_VSTREAM *private_vstream_fhopen(ACL_FILE_HANDLE fh, unsigned int oflags);
ACL_VSTREAM *private_vstream_fdopen(ACL_SOCKET fd, unsigned int oflags,
	size_t buflen, int rw_timeo, int fdtype);
ACL_VSTREAM *private_vstream_fopen(const char *path, unsigned int oflags, int mode, size_t buflen);
ACL_VSTREAM *private_vstream_connect(const char *addr, int conn_timeout, int rw_timeout);
ACL_VSTREAM *private_vstream_connect_ex(const char *addr, int block_mode,
	int conn_timeout, int rw_timeout, int rw_bufsize, int *he_errorp);
void private_vstream_ctl(ACL_VSTREAM *stream, int name,...);
void private_vstream_free(ACL_VSTREAM *stream);
int private_vstream_close(ACL_VSTREAM *stream);

#endif
