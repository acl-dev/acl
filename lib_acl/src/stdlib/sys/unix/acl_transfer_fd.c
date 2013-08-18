#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <stdio.h>
#include <stdlib.h>

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_transfer_fd.h"

int acl_read_fd(int fd, void *ptr, int nbytes, int *recv_fd)
{
#ifdef HAVE_MSGHDR_MSG_CONTROL
	char *myname = "acl_read_fd";
#endif
	struct msghdr msg;
	struct iovec iov[1];
	int n;
#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char   control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
#else
	int newfd;

	msg.msg_accrights = (caddr_t) &newfd;
	msg.msg_accrightslen = sizeof(int);
#endif

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;

	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	if ((n = recvmsg(fd, &msg, 0)) <= 0)
		return (n);

#ifdef HAVE_MSGHDR_MSG_CONTROL
	if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL
	    && cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		if (cmptr->cmsg_level != SOL_SOCKET)
			acl_msg_fatal("%s: control level != SOL_SOCKET", myname);
		if (cmptr->cmsg_type != SCM_RIGHTS)
			acl_msg_fatal("%s: control type != SCM_RIGHTS", myname);
		*recv_fd = *CMSG_DATA(cmptr);
/*
		*recv_fd = *((int *) CMSG_DATA(cmptr));
*/
	} else
		*recv_fd = -1;  /* descriptor was not passed */
#else
	if (msg.msg_accrightslen == sizeof(int))
		*recv_fd = newfd;
	else
		*recv_fd = -1; /* descriptor was not passed */
#endif
	
	return (n);
}

int acl_write_fd(int fd, void *ptr, int nbytes, int send_fd)
{
	struct msghdr msg;
	struct iovec  iov[1];

#ifdef HAVE_MSGHDR_MSG_CONTROL
	union {
		struct cmsghdr cm;
		char   control[CMSG_SPACE(sizeof(int))];
	} control_un;
	struct cmsghdr *cmptr;

	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*CMSG_DATA(cmptr) = send_fd;

/*
	*((int *) CMSG_DATA(cmptr)) = send_fd;
*/
#else
	msg.msg_accrights = (caddr_t) &send_fd;
	msg.msg_accrightslen = sizeof(int);
#endif

	msg.msg_name = NULL;
	msg.msg_namelen = 0;

	iov[0].iov_base = ptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;

	return (sendmsg(fd, &msg, 0));
}

#endif /* ACL_UNIX */
