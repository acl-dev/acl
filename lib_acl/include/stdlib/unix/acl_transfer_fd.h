#ifndef ACL_TRANSFER_FD_INCLUDE_H
#define ACL_TRANSFER_FD_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "../acl_define.h"

#if defined(ACL_UNIX)

#include <sys/types.h>
#include <sys/socket.h>

#ifdef SUNOS5
#undef	HAVE_MSGHDR_MSG_CONTROL
#else
#define	HAVE_MSGHDR_MSG_CONTROL
#endif

#ifndef CMSG_LEN
#define CMSG_LEN(size) (sizeof(struct cmsghdr) + (size))
#endif

#ifndef CMSG_SPACE
#define CMSG_SPACE(size) (sizeof(struct cmsghdr) + (size))
#endif

int acl_read_fd(int fd, void *ptr, int nbytes, int *recv_fd);
int acl_write_fd(int fd, void *ptr, int nbytes, int send_fd);

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif

