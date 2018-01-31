#include "stdafx.h"
#include "common.h"

#ifdef HAS_IOCP

#include "event.h"
#include "event_iocp.h"

typedef BOOL (PASCAL FAR* LPFN_CONNECTEX) (
        IN   SOCKET s,
        IN   const struct sockaddr FAR *name,
        IN   int namelen,
        IN   PVOID lpSendBuffer OPTIONAL,
        IN   DWORD dwSendDataLength,
        OUT  LPDWORD lpdwBytesSent,
        IN   LPOVERLAPPED lpOverlapped
);

typedef struct EVENT_IOCP {
	EVENT  event;
	FILE_EVENT **files;
	int    size;
	int    count;
	HANDLE h_iocp;
} EVENT_IOCP;

struct IOCP_EVENT {
	OVERLAPPED overlapped;
	int   type;
#define	IOCP_EVENT_READ		(1 << 0)
#define IOCP_EVENT_WRITE	(1 << 2)
#define IOCP_EVENT_DEAD		(1 << 3)

	FILE_EVENT *fe;

#define ACCEPT_ADDRESS_LENGTH ((sizeof(struct sockaddr_in) + 16))
	char  myAddrBlock[ACCEPT_ADDRESS_LENGTH * 2];
};

static int iocp_close_sock(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	BOOL ok;

	if (fe->h_iocp != NULL) {
		fe->h_iocp = NULL;
	}

	/* windows xp 环境下，必须在关闭套接字之前调用此宏判断重叠 IO
	 * 是否处于 STATUS_PENDING 状态
	 */
	if (fe->reader != NULL) {
		ok = HasOverlappedIoCompleted(&fe->reader->overlapped);
	} else {
		ok = FALSE;
	}

	/* 必须在释放 fe->reader/fe->writer 前关闭套接口句柄 */
	closesocket(fe->fd);

	/* 将 fd 置为无效值，以便通知调用者该句柄已经关闭 */
	fe->fd = INVALID_SOCKET;

	if (fe->reader) {
		/* 如果完成端口处于未决状态，则不能释放重叠结构，需在主循环的
		 * GetQueuedCompletionStatus 调用后来释放
		 */
		if (ok) {
			free(fe->reader);
		} else {
			fe->reader->type = IOCP_EVENT_DEAD;
			fe->reader->fe   = NULL;
		}
		fe->reader = NULL;
	}

	if (fe->writer) {
		/* 如果完成端口处于未决状态，则不能释放重叠结构，需在主循环的
		 * GetQueuedCompletionStatus 调用后来释放
		 */
		if (HasOverlappedIoCompleted(&fe->writer->overlapped)) {
			free(fe->writer);
		} else {
			fe->writer->type = IOCP_EVENT_DEAD;
			fe->writer->fe   = NULL;
		}

		fe->writer = NULL;
	}

	return 1;
}

static void iocp_check(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (fe->id == -1) {
		assert(ev->count < ev->size);
		fe->id = ev->count++;
		ev->files[fe->id] = fe;
		ev->event.fdcount++;
	} else {
		assert(fe->id >= 0 && fe->id < ev->count);
		assert(ev->files[fe->id] = fe);
	}

	if (fe->h_iocp == NULL) {
		fe->h_iocp = CreateIoCompletionPort((HANDLE) fe->fd,
			ev->h_iocp, (ULONG_PTR) fe, 0);
		if (fe->h_iocp != ev->h_iocp) {
			msg_fatal("%s(%d): CreateIoCompletionPort error(%s)",
				__FUNCTION__, __LINE__, last_serror());
		}
	}
}

static int iocp_add_listen(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	DWORD    ReceiveLen = 0;
	socket_t sock;
	BOOL     ret;

	sock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		0, 0, WSA_FLAG_OVERLAPPED);

	fe->iocp_sock = sock;
	ret = AcceptEx(fe->fd,
		sock,
		fe->reader->myAddrBlock,
		0,
		ACCEPT_ADDRESS_LENGTH,
		ACCEPT_ADDRESS_LENGTH,
		&ReceiveLen,
		&fe->reader->overlapped);
	if (ret == FALSE && acl_fiber_last_error() != ERROR_IO_PENDING) {
		msg_warn("%s(%d): AcceptEx error(%s)",
			__FUNCTION__, __LINE__, last_serror());
		return -1;
	} else {
		fe->mask |= EVENT_READ;
		return 0;
	}
}

static int iocp_add_read(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	DWORD recvBytes;
	BOOL  ret;

	iocp_check(ev, fe);

	if (fe->reader == NULL) {
		fe->reader       = (IOCP_EVENT*) malloc(sizeof(IOCP_EVENT));
		fe->reader->fe   = fe;
	}

	fe->reader->type = IOCP_EVENT_READ;
	memset(&fe->reader->overlapped, 0, sizeof(fe->reader->overlapped));

	if (is_listen_socket(fe->fd)) {
		return iocp_add_listen(ev, fe);
	}

	ret = ReadFile((HANDLE) fe->fd, NULL, 0, &recvBytes,
		&fe->reader->overlapped);
	if (ret == FALSE && acl_fiber_last_error() != ERROR_IO_PENDING) {
		msg_warn("%s(%d): ReadFile error(%s)",
			__FUNCTION__, __LINE__, last_serror());
		return -1;
	} else {
		printf("read info %s\r\n", last_serror());
		fe->mask |= EVENT_READ;
		return 0;
	}
}

static int iocp_add_connect(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	DWORD SentLen = 0;
	struct sockaddr_in addr;
	LPFN_CONNECTEX lpfnConnectEx = NULL;
	GUID  GuidConnectEx = WSAID_CONNECTEX;
	int   dwErr, dwBytes;
	BOOL  ret;
	static const char *any_ip = "0.0.0.0";

	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(any_ip);
	addr.sin_port        = htons(0);

	if (bind(fe->fd, (struct sockaddr *) &addr,
		sizeof(struct sockaddr)) < 0) {
		msg_fatal("%s(%d): bind local ip(%s) error(%s, %d), sock: %u",
			__FUNCTION__, __LINE__, any_ip, last_serror(),
			acl_fiber_last_error(), (unsigned) fe->fd);
	}

	dwErr = WSAIoctl(fe->fd,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidConnectEx,
			sizeof(GuidConnectEx),
			&lpfnConnectEx,
			sizeof(lpfnConnectEx),
			&dwBytes,
			NULL,
			NULL);
	if(dwErr  ==  SOCKET_ERROR) {
		msg_fatal("%s(%d): WSAIoctl error(%s)",
			__FUNCTION__, __LINE__, last_serror());
	}

	ret = lpfnConnectEx(fe->fd,
			(const struct sockaddr *) &fe->peer_addr,
			sizeof(struct sockaddr),
			NULL,
			0,
			NULL,
			&fe->writer->overlapped);
	if (ret == FALSE && acl_fiber_last_error() != ERROR_IO_PENDING) {
		msg_warn("%s(%d): ConnectEx error(%s), sock(%u)",
			__FUNCTION__, __LINE__, last_serror(), fe->fd);
		return -1;
	} else {
		fe->mask |= EVENT_WRITE;
		return 0;
	}
}

static int iocp_add_write(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	DWORD sendBytes;
	BOOL  ret;

	iocp_check(ev, fe);

	if (fe->writer == NULL) {
		fe->writer     = (IOCP_EVENT*) malloc(sizeof(IOCP_EVENT));
		fe->writer->fe = fe;
	}

	fe->writer->type = IOCP_EVENT_WRITE;
	memset(&fe->writer->overlapped, 0, sizeof(fe->writer->overlapped));

	if (fe->status & STATUS_CONNECTING) {
		return iocp_add_connect(ev, fe);
	}

	ret = WriteFile((HANDLE) fe->fd, NULL, 0, &sendBytes,
		&fe->writer->overlapped);
	if (ret == FALSE && acl_fiber_last_error() != ERROR_IO_PENDING) {
		msg_warn("%s(%d): WriteFile error(%s)",
			__FUNCTION__, __LINE__, last_serror());
		return -1;
	} else {
		fe->mask |= EVENT_WRITE;
		return 0;
	}
}

static void iocp_remove(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (fe->id < --ev->count) {
		ev->files[fe->id]     = ev->files[ev->count];
		ev->files[fe->id]->id = fe->id;
	}

	fe->id = -1;
	ev->event.fdcount--;
}

static int iocp_del_read(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_READ)) {
		return 0;
	}

	assert(fe->id >= 0 && fe->id < ev->count);
	fe->mask &= ~EVENT_READ;
	fe->reader->type &= ~IOCP_EVENT_READ;

	if (fe->mask == 0) {
		iocp_remove(ev, fe);
	}
	return 0;
}

static int iocp_del_write(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_WRITE)) {
		return 0;
	}

	assert(fe->id >= 0 && fe->id < ev->count);
	fe->mask &= ~EVENT_WRITE;
	fe->writer->type &= ~IOCP_EVENT_WRITE;

	if (fe->mask == 0) {
		iocp_remove(ev, fe);
	}
	return 0;
}

static void iocp_set_all(EVENT_IOCP *ev)
{
	int i, n = 0;

	for (i = 0; i < ev->count; i++) {
		FILE_EVENT *fe = ev->files[i];
		if (!fe->reader || !(fe->reader->type & IOCP_EVENT_READ)) {
			iocp_add_read(ev, fe);
			n++;
		}
		if (!fe->writer || !(fe->writer->type & IOCP_EVENT_WRITE)) {
			iocp_add_write(ev, fe);
			n++;
		}
	}
}

static int iocp_wait(EVENT *ev, int timeout)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) ev;
	ARRAY *readers = array_create(100);
	ARRAY *writers = array_create(100);
	ITER   iter;

	iocp_set_all(ei);

	for (;;) {
		DWORD bytesTransferred;
		IOCP_EVENT *event = NULL;
		FILE_EVENT *fe = NULL;
		BOOL isSuccess = GetQueuedCompletionStatus(ei->h_iocp,
			&bytesTransferred, (PULONG_PTR) &fe,
			(OVERLAPPED**) &event, timeout);

		if (!isSuccess) {
			if (event == NULL) {
				break;
			}

			if (event->type == IOCP_EVENT_DEAD) {
				free(event);
			} else if (event->fe == NULL) {
				msg_warn("%s(%d): fe null",
					__FUNCTION__, __LINE__);
				free(event);
			} else if (event->fe != fe) {
				msg_fatal("%s(%d): invalid fe",
					__FUNCTION__, __LINE__);
			}

			continue;
		}

		assert(fe == event->fe);

		if ((event->type & IOCP_EVENT_READ) && fe->r_proc) {
			assert(fe->reader == event);
			//iocp_del_read(ei, fe);
			array_append(readers, fe);
		}

		if ((event->type & IOCP_EVENT_WRITE) && fe->w_proc) {
			assert(fe->writer == event);
			//iocp_del_write(ei, fe);
			array_append(writers, fe);
		}

		timeout = 0;
	}

	foreach(iter, readers) {
		FILE_EVENT *fe = (FILE_EVENT *) iter.data;
		fe->r_proc(ev, fe);
	}

	foreach(iter, writers) {
		FILE_EVENT *fe = (FILE_EVENT *) iter.data;
		fe->w_proc(ev, fe);
	}

	array_free(readers, NULL);
	array_free(writers, NULL);

	return 0;
}

static void iocp_free(EVENT *ev)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) ev;

	if (ei->h_iocp) {
		CloseHandle(ei->h_iocp);
	}
	free(ei->files);
	free(ei);
}

static int iocp_checkfd(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	(void) ev;
	return getsocktype(fe->fd) == -1 ? -1 : 0;
}

static int iocp_handle(EVENT *ev)
{
	return -1;
}

static const char *iocp_name(void)
{
	return "iocp";
}

EVENT *event_iocp_create(int size)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) calloc(1, sizeof(EVENT_IOCP));

	ei->h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (ei->h_iocp == NULL) {
		msg_fatal("%s(%d): create iocp error(%s)",
			__FUNCTION__, __LINE__, last_serror());
	}

	ei->files = (FILE_EVENT**) calloc(size, sizeof(FILE_EVENT*));
	ei->size  = size;
	ei->count = 0;

	ei->event.name   = iocp_name;
	ei->event.handle = iocp_handle;
	ei->event.free   = iocp_free;

	ei->event.event_wait = iocp_wait;
	ei->event.checkfd    = (event_oper *) iocp_checkfd;
	ei->event.add_read   = (event_oper *) iocp_add_read;
	ei->event.add_write  = (event_oper *) iocp_add_write;
	ei->event.del_read   = (event_oper *) iocp_del_read;
	ei->event.del_write  = (event_oper *) iocp_del_write;
	ei->event.close_sock = (event_oper *) iocp_close_sock;

	return (EVENT *) ei;
}

#endif
