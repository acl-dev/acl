#ifndef	__FIBER_IO_INCLUDE_H__
#define	__FIBER_IO_INCLUDE_H__

#include "fiber/fiber_define.h"
#include "../event.h"

#ifdef __cplusplus
extern "C" {
#endif

// in fiber_read.c
ssize_t fiber_recvmsg(FILE_EVENT *fe, struct msghdr *msg, int flags);
ssize_t fiber_recv(FILE_EVENT *fe, void *buf, size_t len, int flags);
ssize_t fiber_recvfrom(FILE_EVENT *fe, void *buf, size_t len,
	int flags, struct sockaddr *src_addr, socklen_t *addrlen);

// in fiber_write.c
ssize_t fiber_send(FILE_EVENT *fe, const void *buf, size_t len, int flags);
ssize_t fiber_sendto(FILE_EVENT *fe, const void *buf, size_t len,
	int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t fiber_sendmsg(FILE_EVENT *fe, const struct msghdr *msg, int flags);

#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)  || defined(MINGW) // SYS_UNIX

// in fiber_read.c
int fiber_iocp_read(FILE_EVENT *fe, char *buf, int len);

ssize_t fiber_read(FILE_EVENT *fe,  void *buf, size_t count);
ssize_t fiber_readv(FILE_EVENT *fe, const struct iovec *iov, int iovcnt);

// in fiber_write.c
int fiber_iocp_write(FILE_EVENT *fe, const char *buf, int len);

ssize_t fiber_write(FILE_EVENT *fe, const void *buf, size_t count);
ssize_t fiber_writev(FILE_EVENT *fe, const struct iovec *iov, int iovcnt);
# if defined(__USE_LARGEFILE64) && !defined(DISABLE_HOOK_IO)
ssize_t fiber_sendfile64(socket_t out_fd, int in_fd, off64_t *offset, size_t count);
# endif

# ifdef HAS_IO_URING
// in file.c
extern int file_close(EVENT *ev, FILE_EVENT *fe);

# define	CANCEL_NONE	0
# define	CANCEL_IO_READ	1
# define	CANCEL_IO_WRITE	2
extern int file_cancel(EVENT *ev, FILE_EVENT *fe, int iotype);
extern ssize_t file_sendfile(socket_t out_fd, int in_fd, off64_t *off, size_t cnt);
# endif // HAS_IO_URING

#endif  // SYS_UNIX

#ifdef __cplusplus
}
#endif

#endif
