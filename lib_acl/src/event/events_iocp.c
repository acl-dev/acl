#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_fifo.h"
#include "stdlib/acl_mystring.h"
#include "net/acl_sane_socket.h"
#include "event/acl_events.h"

#endif  /* ACL_PREPARE_COMPILE */

#include "events_define.h"

#ifdef ACL_EVENTS_STYLE_IOCP

#include "events_fdtable.h"
#include "events.h"
#include "events_iocp.h"

typedef BOOL (PASCAL FAR* LPFN_CONNECTEX) (
        IN   SOCKET s,
        IN   const struct sockaddr FAR *name,
        IN   int namelen,
        IN   PVOID lpSendBuffer OPTIONAL,
        IN   DWORD dwSendDataLength,
        OUT  LPDWORD lpdwBytesSent,
        IN   LPOVERLAPPED lpOverlapped
);

typedef struct EVENT_KERNEL {
	ACL_EVENT event;
	ACL_RING fdp_delay_list;
	int   event_fdslots;
	int   event_fd;
	HANDLE h_iocp;
} EVENT_KERNEL;

struct IOCP_EVENT {
	OVERLAPPED overlapped;
	int   type;
#define	IOCP_EVENT_READ		(1 << 0)
#define IOCP_EVENT_WRITE	(1 << 2)
#define IOCP_EVENT_DEAD		(1 << 3)

	ACL_EVENT_FDTABLE *fdp;

#define ACCEPT_ADDRESS_LENGTH ((sizeof(struct sockaddr_in) + 16))
	char  myAddrBlock[ACCEPT_ADDRESS_LENGTH * 2];
};

static void stream_on_close(ACL_VSTREAM *stream, void *arg)
{
	const char *myname = "stream_on_close";
	EVENT_KERNEL *ev = (EVENT_KERNEL*) arg;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	BOOL is_completed;

	if (fdp == NULL)
		acl_msg_fatal("%s(%d): fdp null, sockfd(%d)",
			myname, __LINE__, sockfd);

	if (fdp->h_iocp != NULL) {
		fdp->h_iocp = NULL;
		fdp->flag &= ~EVENT_FDTABLE_FLAG_IOCP;
	}

	/* windows xp 环境下，必须在关闭套接字之前调用此宏判断重叠 IO
	 * 是否处于 STATUS_PENDING 状态
	 */
	if (fdp->event_read != NULL)
		is_completed = HasOverlappedIoCompleted(
			&fdp->event_read->overlapped);
	else
		is_completed = FALSE;

	/* 必须在释放 fdp->event_read/fdp->event_write 前关闭套接口句柄 */
	if (ACL_VSTREAM_SOCK(stream) != ACL_SOCKET_INVALID
		&& stream->close_fn)
	{
		(void) stream->close_fn(ACL_VSTREAM_SOCK(stream));
	} else if (ACL_VSTREAM_FILE(stream) != ACL_FILE_INVALID
		&& stream->fclose_fn)
	{
		(void) stream->fclose_fn(ACL_VSTREAM_FILE(stream));
	}

	ACL_VSTREAM_SOCK(stream) = ACL_SOCKET_INVALID;
	ACL_VSTREAM_FILE(stream) = ACL_FILE_INVALID;

	if (fdp->event_read) {
		/* 如果完成端口处于未决状态，则不能释放重叠结构，需在主循环的
		 * GetQueuedCompletionStatus 调用后来释放
		 */
		if (is_completed)
			acl_myfree(fdp->event_read);
		else {
			fdp->event_read->type = IOCP_EVENT_DEAD;
			fdp->event_read->fdp = NULL;
		}
		fdp->event_read = NULL;
	}
	if (fdp->event_write) {
		/* 如果完成端口处于未决状态，则不能释放重叠结构，需在主循环的
		 * GetQueuedCompletionStatus 调用后来释放
		 */
		if (HasOverlappedIoCompleted(&fdp->event_write->overlapped))
			acl_myfree(fdp->event_write);
		else {
			fdp->event_write->type = IOCP_EVENT_DEAD;
			fdp->event_write->fdp = NULL;
		}

		fdp->event_write = NULL;
	}

	if ((fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_DELAY_OPER;
		acl_ring_detach(&fdp->delay_entry);
	}

	if (ev->event.maxfd == ACL_VSTREAM_SOCK(fdp->stream))
		ev->event.maxfd = ACL_SOCKET_INVALID;
	if (fdp->fdidx >= 0 && fdp->fdidx < --ev->event.fdcnt) {
		ev->event.fdtabs[fdp->fdidx] = ev->event.fdtabs[ev->event.fdcnt];
		ev->event.fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < ev->event.ready_cnt
		&& ev->event.ready[fdp->fdidx_ready] == fdp)
	{
		ev->event.ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
	event_fdtable_free(fdp);
	stream->fdp = NULL;
}

static ACL_EVENT_FDTABLE *read_enable(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);

	if (fdp == NULL) {
		fdp = event_fdtable_alloc();

		fdp->flag = EVENT_FDTABLE_FLAG_ADD_READ | EVENT_FDTABLE_FLAG_EXPT;
		fdp->stream = stream;
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		stream->fdp = (void *) fdp;

		/* 添加流关闭时的回调函数 */
		acl_vstream_add_close_handle(stream, stream_on_close, eventp);
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ)) {
		goto END;
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_READ)) {

		/* 停止禁止读监听过程 */

		acl_assert((fdp->flag & EVENT_FDTABLE_FLAG_READ));

		/* 重新启用读监听过程, 因为之前的过程是正在拆除读监听过程但
		 * 还没有正式拆除，所以只需要清除拆除标志位即可
		 */

		fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_READ;
	} else if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		fdp->flag |= EVENT_FDTABLE_FLAG_ADD_READ;
		if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
			acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
			fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		}
	}

END:
	if (fdp->fdidx == -1) {
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt++] = fdp;
	}

	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->r_callback != callback || fdp->r_context != context) {
		fdp->r_callback = callback;
		fdp->r_context = context;
	}

	if (timeout > 0) {
		fdp->r_timeout = ((acl_int64) timeout) * 1000000;
		fdp->r_ttl = eventp->present + fdp->r_timeout;
	} else {
		fdp->r_ttl = 0;
		fdp->r_timeout = 0;
	}

	return fdp;
}

static void event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	ACL_EVENT_FDTABLE *fdp = read_enable(eventp, stream, timeout,
			callback, context);
	fdp->listener = acl_is_listening_socket(ACL_VSTREAM_SOCK(stream));
}

static void event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	ACL_EVENT_FDTABLE *fdp = read_enable(eventp, stream, timeout,
			callback, context);
	fdp->listener = acl_is_listening_socket(ACL_VSTREAM_SOCK(stream));
}

static void event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);

	if (fdp == NULL) {
		fdp = event_fdtable_alloc();

		fdp->flag = EVENT_FDTABLE_FLAG_ADD_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		fdp->stream = stream;
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		stream->fdp = (void *) fdp;
		/* 添加流关闭时的回调函数 */
		acl_vstream_add_close_handle(stream, stream_on_close, eventp);
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE)) {
		goto END;
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_WRITE)) {

		acl_assert((fdp->flag & EVENT_FDTABLE_FLAG_WRITE));

		fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_WRITE;
	} else if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		fdp->flag |= EVENT_FDTABLE_FLAG_ADD_WRITE;
		if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
			acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
			fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
		}
	}

END:
	if (fdp->fdidx == -1) {
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt++] = fdp;
	}

	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->w_callback != callback || fdp->w_context != context) {
		fdp->w_callback = callback;
		fdp->w_context = context;
	}

	if (timeout > 0) {
		fdp->w_timeout = ((acl_int64) timeout) * 1000000;
		fdp->w_ttl = eventp->present + fdp->w_timeout;
	} else {
		fdp->w_ttl = 0;
		fdp->w_timeout = 0;
	}
}

/* event_disable_read - disable request for read events */

static void event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_read";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}
	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx(%d) invalid, fdcnt: %d",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream),
			fdp->fdidx, eventp->fdcnt);
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_READ)) {
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ)) {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_READ;
		goto DEL_READ_TAG;
	}

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream));
		return;
	}
	fdp->flag |= EVENT_FDTABLE_FLAG_DEL_READ;
	if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
	}

DEL_READ_TAG:

	fdp->r_ttl = 0;
	fdp->r_timeout = 0;
	fdp->r_callback = NULL;
	fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
		|| (fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE))
	{
		return;
	}

	if (eventp->maxfd == ACL_VSTREAM_SOCK(fdp->stream))
		eventp->maxfd = ACL_SOCKET_INVALID;

	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < eventp->ready_cnt
		&& eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
}

/* event_disable_write - disable request for write events */

static void event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_write";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}
	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx(%d) invalid",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream), fdp->fdidx);
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_WRITE)) {
		return;
	}
	if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE)) {
		fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_WRITE;
		goto DEL_WRITE_TAG;
	}
	if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		acl_msg_warn("%s(%d): sockfd(%d) not be set",
			myname, __LINE__, ACL_VSTREAM_SOCK(stream));
		return;
	}

	fdp->flag |= EVENT_FDTABLE_FLAG_DEL_WRITE;
	if (!(fdp->flag & EVENT_FDTABLE_FLAG_DELAY_OPER)) {
		acl_ring_append(&ev->fdp_delay_list, &fdp->delay_entry);
		fdp->flag |= EVENT_FDTABLE_FLAG_DELAY_OPER;
	}

DEL_WRITE_TAG:

	fdp->w_ttl = 0;
	fdp->w_timeout = 0;
	fdp->w_callback = NULL;
	fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
		|| (fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ))
	{
		return;
	}

	if (eventp->maxfd == ACL_VSTREAM_SOCK(stream))
		eventp->maxfd = ACL_SOCKET_INVALID;

	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < eventp->ready_cnt
		&& eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	event_disable_read(eventp, stream);
	event_disable_write(eventp, stream);
}

static void enable_listen(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "enable_listen";
	ACL_SOCKET sock;
	DWORD ReceiveLen = 0;

	sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		0, 0, WSA_FLAG_OVERLAPPED);

	memset(&fdp->event_read->overlapped, 0,
		sizeof(fdp->event_read->overlapped));

	fdp->stream->type |= ACL_VSTREAM_TYPE_LISTEN_IOCP;
	fdp->stream->iocp_sock = sock;

	if (AcceptEx(ACL_VSTREAM_SOCK(fdp->stream), sock,
		fdp->event_read->myAddrBlock, 0,
		ACCEPT_ADDRESS_LENGTH, ACCEPT_ADDRESS_LENGTH,
		&ReceiveLen, &fdp->event_read->overlapped) == FALSE
		&& acl_last_error() !=ERROR_IO_PENDING)
	{
		acl_msg_warn("%s(%d): AcceptEx error(%s)",
			myname, __LINE__, acl_last_serror());
	}
}

static void enable_read(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "enable_read";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(fdp->stream);
	DWORD recvBytes;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_READ;
	fdp->flag |= EVENT_FDTABLE_FLAG_READ;

	if (fdp->h_iocp == NULL) {
		fdp->h_iocp = CreateIoCompletionPort((HANDLE) sockfd,
			ev->h_iocp, (ULONG_PTR) fdp, 0);
		if (fdp->h_iocp != ev->h_iocp)
			acl_msg_fatal("%s(%d): CreateIoCompletionPort error(%s)",
				myname, __LINE__, acl_last_serror());
		fdp->flag |= EVENT_FDTABLE_FLAG_IOCP;
	}

	if (fdp->event_read == NULL) {
		fdp->event_read = (IOCP_EVENT*) acl_mymalloc(sizeof(IOCP_EVENT));
		fdp->event_read->fdp = fdp;
	} else if (fdp->event_read->type == IOCP_EVENT_READ) {
		return;
	}

	fdp->event_read->type = IOCP_EVENT_READ;

	if ((fdp->stream->type & ACL_VSTREAM_TYPE_LISTEN)) {
		enable_listen(ev, fdp);
		return;
	}

	memset(&fdp->event_read->overlapped, 0,
		sizeof(fdp->event_read->overlapped));

	if (ReadFile((HANDLE) sockfd, NULL, 0,
			&recvBytes, &fdp->event_read->overlapped) == FALSE
		&& acl_last_error() != ERROR_IO_PENDING)
	{
		acl_msg_warn("%s(%d): ReadFile error(%s)",
			myname, __LINE__, acl_last_serror());
	}
}

static void parse_addr(char *addr, unsigned short *port,
	char **local_ip, char **remote)
{
	const char *myname = "parse_addr";
	char *ptr;

	ptr = strchr(addr, ':');
	if (ptr == NULL)
		acl_msg_fatal("%s, %s(%d): invalid addr(%s)",
			__FILE__, myname, __LINE__, addr);

	*ptr++ = 0;
	*port = atoi(ptr);
	if (*port <= 0)
		acl_msg_fatal("%s, %s(%d): invalid port(%d)",
			__FILE__, myname, __LINE__, *port);

	ptr = strchr(addr, '@');
	if (ptr != NULL) {
		*ptr++ = 0;
		*local_ip = addr;
		*remote = ptr; 
	} else {
		*local_ip = NULL;
		*remote = addr;
	}

	if (strlen(*remote) == 0)
		acl_msg_fatal("%s, %s(%d): ip buf's length is 0",
			__FILE__, myname, __LINE__);
}

static void enable_connect(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "enable_connect";
	DWORD SentLen = 0;
	struct sockaddr_in addr;
	unsigned short port;
	char *local_ip, *remote, buf[256];
	ACL_SOCKET sock = ACL_VSTREAM_SOCK(fdp->stream);
	LPFN_CONNECTEX lpfnConnectEx = NULL;
	GUID  GuidConnectEx = WSAID_CONNECTEX;
	int   dwErr, dwBytes;
	static char *any_ip = "0.0.0.0";

	memset(&fdp->event_write->overlapped, 0,
		sizeof(fdp->event_write->overlapped));

	ACL_SAFE_STRNCPY(buf, ACL_VSTREAM_PEER(fdp->stream), sizeof(buf));
	parse_addr(buf, &port, &local_ip, &remote);

	if (!local_ip || !*local_ip)
		local_ip = any_ip;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(local_ip);
	addr.sin_port = htons(0);
	if (bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0) {
		acl_msg_fatal("%s(%d): bind local ip(%s) error(%s, %d), sock: %d",
			myname, __LINE__, local_ip, acl_last_serror(),
			acl_last_error(), (int) sock);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons((short) port);
	addr.sin_addr.s_addr = inet_addr(remote);

	dwErr = WSAIoctl(sock,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidConnectEx,
			sizeof(GuidConnectEx),
			&lpfnConnectEx,
			sizeof(lpfnConnectEx),
			&dwBytes,
			NULL,
			NULL);
	if(dwErr  ==  SOCKET_ERROR)
		acl_msg_fatal("%s(%d): WSAIoctl error(%s)",
			myname, __LINE__, acl_last_serror());

	if (lpfnConnectEx(sock,
			(const struct sockaddr *) &addr,
			sizeof(struct sockaddr),
			NULL,
			0,
			NULL,
			&fdp->event_write->overlapped) == FALSE
		&& acl_last_error() !=ERROR_IO_PENDING)
	{
		acl_msg_warn("%s(%d): ConnectEx error(%s), sock(%d)",
			myname, __LINE__, acl_last_serror(), sock);
	}
}

static void enable_write(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	const char *myname = "enable_write";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(fdp->stream);
	DWORD sendBytes;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_ADD_WRITE;
	fdp->flag |= EVENT_FDTABLE_FLAG_WRITE;

	if (fdp->h_iocp == NULL) {
		fdp->h_iocp = CreateIoCompletionPort((HANDLE) sockfd,
			ev->h_iocp, (ULONG_PTR) fdp, 0);
		if (fdp->h_iocp != ev->h_iocp)
			acl_msg_fatal("%s(%d): CreateIoCompletionPort error(%s)",
				myname, __LINE__, acl_last_serror());
		fdp->flag |= EVENT_FDTABLE_FLAG_IOCP;
	}

	if (fdp->event_write == NULL) {
		fdp->event_write = (IOCP_EVENT*) acl_mymalloc(sizeof(IOCP_EVENT));
		fdp->event_write->fdp = fdp;
	} else if (fdp->event_write->type == IOCP_EVENT_WRITE) {
		return;
	}

	fdp->event_write->type = IOCP_EVENT_WRITE;

	if ((fdp->stream->flag & ACL_VSTREAM_FLAG_CONNECTING)) {
		enable_connect(ev, fdp);
		fdp->stream->flag &= ~ACL_VSTREAM_FLAG_CONNECTING;
		return;
	}

	memset(&fdp->event_write->overlapped, 0,
		sizeof(fdp->event_write->overlapped));

	if (WriteFile((HANDLE) sockfd, NULL, 0,
			&sendBytes, &fdp->event_write->overlapped) == FALSE
		&& acl_last_error() != ERROR_IO_PENDING)
	{
		acl_msg_warn("%s(%d): WriteFile error(%s), sockfd(%d)",
			myname, __LINE__, acl_last_serror(), sockfd);
	}
}

static int disable_read(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	ACL_VSTREAM *stream = fdp->stream;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_READ;
	fdp->flag &= ~EVENT_FDTABLE_FLAG_READ;
	fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);
	return 1;
}

static int disable_write(EVENT_KERNEL *ev, ACL_EVENT_FDTABLE *fdp)
{
	ACL_VSTREAM *stream = fdp->stream;

	fdp->flag &= ~EVENT_FDTABLE_FLAG_DEL_WRITE;
	fdp->flag &= ~EVENT_FDTABLE_FLAG_WRITE;
	fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);
	return 1;
}

static void event_set_all(ACL_EVENT *eventp)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_FDTABLE *fdp;
	int   i;

	/* 优先处理添加读/写监控任务, 这样可以把 ADD 中间态转换成正式状态 */

	eventp->ready_cnt = 0;

	if (eventp->present - eventp->last_check >= eventp->check_inter
		|| eventp->read_ready > 0)
	{
		eventp->last_check = eventp->present;
		event_check_fds(eventp);
	}

	/* 处理任务项 */

	while (1) {
		ACL_RING *r = acl_ring_pop_head(&ev->fdp_delay_list);
		if (r == NULL)
			break;
		fdp = acl_ring_to_appl(r, ACL_EVENT_FDTABLE, delay_entry);

		if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_READ)) {
			enable_read(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_ADD_WRITE)) {
			enable_write(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_READ)) {
			disable_read(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_DEL_WRITE)) {
			disable_write(ev, fdp);
		}

		fdp->flag &= ~EVENT_FDTABLE_FLAG_DELAY_OPER;
	}

	for (i = 0; i < eventp->fdcnt; i++) {
		fdp = eventp->fdtabs[i];
		if ((fdp->event_type & (ACL_EVENT_XCPT | ACL_EVENT_RW_TIMEOUT)))
			continue;
		if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
			&& (fdp->event_read == NULL || fdp->event_read->type == 0))
		{
			enable_read(ev, fdp);
		}
		if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)
			&& (fdp->event_write == NULL || fdp->event_write->type == 0))
		{
			enable_write(ev, fdp);
		}
	}
}

static void event_loop(ACL_EVENT *eventp)
{
	const char *myname = "event_loop";
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;
	ACL_EVENT_NOTIFY_TIME timer_fn;
	void    *timer_arg;
	ACL_EVENT_TIMER *timer;
	int   delay;
	ACL_EVENT_FDTABLE *fdp;

	delay = (int) (eventp->delay_sec * 1000 + eventp->delay_usec / 1000);
	if (delay < 0)
		delay = 0; /* 0 milliseconds at least */

	SET_TIME(eventp->present);

	/*
	 * Find out when the next timer would go off. Timer requests are sorted.
	 * If any timer is scheduled, adjust the delay appropriately.
	 */
	if ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		acl_int64 n = (timer->when - eventp->present) / 1000;

		if (n <= 0)
			delay = 0;
		else if ((int) n < delay)
			delay = (int) n;
	}

	eventp->nested++;

	event_set_all(eventp);

	if (eventp->fdcnt == 0) {
		if (eventp->ready_cnt == 0)
			sleep(1);
		goto TAG_DONE;
	}

	if (eventp->ready_cnt > 0)
		delay = 0;

TAG_DONE:

	/*
	* Deliver timer events. Requests are sorted: we can stop when we reach
	* the future or the list end. Allow the application to update the timer
	* queue while it is being called back. To this end, we repeatedly pop
	* the first request off the timer queue before delivering the event to
	* the application.
	*/
	SET_TIME(eventp->present);
	while ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		if (timer->when > eventp->present)
			break;
		timer_fn  = timer->callback;
		timer_arg = timer->context;

		/* 如果定时器的时间间隔 > 0 且允许定时器被循环调用，
		 * 则再重设定时器
		 */
		if (timer->delay > 0 && timer->keep) {
			timer->ncount++;
			eventp->timer_request(eventp, timer->callback,
				timer->context, timer->delay, timer->keep);
		} else {
			acl_ring_detach(&timer->ring);  /* first this */
			timer->nrefer--;
			if (timer->nrefer != 0)
				acl_msg_fatal("%s(%d): nrefer(%d) != 0",
					myname, __LINE__, timer->nrefer);
			acl_myfree(timer);
		}
		timer_fn(ACL_EVENT_TIME, eventp, timer_arg);
	}

	for (;;) {
		BOOL isSuccess = FALSE;
		DWORD bytesTransferred = 0;
		DWORD iocpKey = 0;
		DWORD lastError = 0;
		IOCP_EVENT *iocp_event = NULL;

		isSuccess = GetQueuedCompletionStatus(ev->h_iocp,
			&bytesTransferred, (PULONG_PTR) &fdp,
			(OVERLAPPED**) &iocp_event, delay);

		if (!isSuccess) {

			if (iocp_event == NULL)
				break;
			if (iocp_event->type == IOCP_EVENT_DEAD)
				acl_myfree(iocp_event);
			else if (iocp_event->fdp == NULL) {
				acl_msg_warn("%s(%d): fdp null",
					myname, __LINE__);
				acl_myfree(iocp_event);
			} else if (iocp_event->fdp != fdp)
				acl_msg_fatal("%s(%d): invalid fdp",
					myname, __LINE__);
			else if (!(fdp->event_type & (ACL_EVENT_XCPT
				| ACL_EVENT_RW_TIMEOUT)))
			{
				fdp->event_type |= ACL_EVENT_XCPT;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt] = fdp;
				eventp->ready_cnt++;
			}
			continue;
		}

		acl_assert(fdp == iocp_event->fdp);

		if ((fdp->event_type & (ACL_EVENT_XCPT
			| ACL_EVENT_RW_TIMEOUT)))
		{
			continue;
		}

		if (iocp_event->type == IOCP_EVENT_READ) {
			acl_assert(fdp->event_read == iocp_event);
			iocp_event->type &= ~IOCP_EVENT_READ;
			if ((fdp->event_type & (ACL_EVENT_READ
				| ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_READ;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt] = fdp;
				eventp->ready_cnt++;
			}

			if (fdp->listener)
				fdp->event_type |= ACL_EVENT_ACCEPT;
			else
				fdp->stream->read_ready = 1;
		}
		if (iocp_event->type == IOCP_EVENT_WRITE) {
			acl_assert(fdp->event_write == iocp_event);
			iocp_event->type &= ~IOCP_EVENT_WRITE;
			if ((fdp->event_type & (ACL_EVENT_READ
				| ACL_EVENT_WRITE)) == 0)
			{
				fdp->event_type |= ACL_EVENT_WRITE;
				fdp->fdidx_ready = eventp->ready_cnt;
				eventp->ready[eventp->ready_cnt] = fdp;
				eventp->ready_cnt++;
			}
		}
		delay = 0;
	}

	if (eventp->ready_cnt > 0)
		event_fire(eventp);
	eventp->nested--;
}

static int event_isrset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	return fdp == NULL ? 0 : (fdp->flag & EVENT_FDTABLE_FLAG_READ);
}

static int event_iswset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	return fdp == NULL ? 0 : (fdp->flag & EVENT_FDTABLE_FLAG_WRITE);

}

static int event_isxset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	return fdp == NULL ? 0 : (fdp->flag & EVENT_FDTABLE_FLAG_EXPT);
}

static void event_free(ACL_EVENT *eventp)
{
	EVENT_KERNEL *ev = (EVENT_KERNEL *) eventp;

	CloseHandle(ev->h_iocp);
	acl_myfree(ev);
}

ACL_EVENT *event_new_iocp(int fdsize acl_unused)
{
	const char *myname = "event_new_iocp";
	ACL_EVENT *eventp;
	EVENT_KERNEL *ev;

	eventp = event_alloc(sizeof(EVENT_KERNEL));

	snprintf(eventp->name, sizeof(eventp->name), "events - %s", EVENT_NAME);
	eventp->event_mode           = ACL_EVENT_KERNEL;
	eventp->use_thread           = 0;
	eventp->loop_fn              = event_loop;
	eventp->free_fn              = event_free;
	eventp->enable_read_fn       = event_enable_read;
	eventp->enable_write_fn      = event_enable_write;
	eventp->enable_listen_fn     = event_enable_listen;
	eventp->disable_read_fn      = event_disable_read;
	eventp->disable_write_fn     = event_disable_write;
	eventp->disable_readwrite_fn = event_disable_readwrite;
	eventp->isrset_fn            = event_isrset;
	eventp->iswset_fn            = event_iswset;
	eventp->isxset_fn            = event_isxset;
	eventp->timer_request        = event_timer_request;
	eventp->timer_cancel         = event_timer_cancel;
	eventp->timer_keep           = event_timer_keep;
	eventp->timer_ifkeep         = event_timer_ifkeep;

	ev = (EVENT_KERNEL*) eventp;
	ev->h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (ev->h_iocp == NULL)
		acl_msg_fatal("%s(%d): create iocp error(%s)",
			myname, __LINE__, acl_last_serror());
	acl_ring_init(&ev->fdp_delay_list);
	return eventp;
}

#endif
