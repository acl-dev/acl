#include "stdafx.h"
#include "common.h"

#ifdef HAS_IOCP

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Kernel32.lib")
#pragma comment(lib, "Mswsock.lib")

#include "../hook/hook.h"
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
	ARRAY *events;
} EVENT_IOCP;

struct IOCP_EVENT {
	OVERLAPPED overlapped;
	int   type;
#define	IOCP_EVENT_READ		(1 << 0)
#define IOCP_EVENT_WRITE	(1 << 2)
#define IOCP_EVENT_DEAD		(1 << 3)
#define	IOCP_EVENT_POLLR	(1 << 4)
#define	IOCP_EVENT_POLLW	(1 << 4)
	int   refer;
	FILE_EVENT *fe;
	event_proc *proc;

#define ACCEPT_ADDRESS_LENGTH ((sizeof(struct sockaddr_in) + 16))
	char  myAddrBlock[ACCEPT_ADDRESS_LENGTH * 2];
};

static void iocp_event_save(EVENT_IOCP *ei, IOCP_EVENT *event,
	FILE_EVENT *fe, DWORD trans);

static void iocp_remove(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (fe->id < --ev->count) {
		ev->files[fe->id]     = ev->files[ev->count];
		ev->files[fe->id]->id = fe->id;
	}

	fe->id = -1;
	ev->event.fdcount--;
}

static int iocp_close_sock(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (fe->h_iocp != NULL) {
		fe->h_iocp = NULL;
	}

	if (fe->id >= 0) {
		iocp_remove(ev, fe);
	}

	/* must close socket before releasing fe->reader/fe->writer */
	if (fe->fd != INVALID_SOCKET) {
		// because closesocket API has been hooked, so we should use the
		// real system API to close the socket.
		//closesocket(fe->fd);
		(*sys_close)(fe->fd);

		/* set fd INVALID_SOCKET notifying the caller the socket be closed*/
		//fe->fd = INVALID_SOCKET;
	}

	/* On Windows XP, must check if the OVERLAPPED IO is in STATUS_PENDING
	 * status before the socket being closed.
	 */

	if (fe->reader) {
		/*
		 * If the IOCP Port isn't in completed status, the OVERLAPPED
		 * object should not be released, which should be released in
		 * the GetQueuedCompletionStatus process.
		 */
		if (HasOverlappedIoCompleted(&fe->reader->overlapped)) {
			if (fe->reader->refer == 0) {
				mem_free(fe->reader);
			} else {
				fe->reader->fe = NULL;
			}
		} else {
			fe->reader->type = IOCP_EVENT_DEAD;
			fe->reader->fe   = NULL;
		}
		fe->reader = NULL;
	}

	if (fe->writer) {
		/* If the IOCP Port is in incompleted status, the OVERLAPPED
		 * object shouldn't be released, which should be released in
		 * the GetQueuedCompletionStatus process.
		 */
		if (HasOverlappedIoCompleted(&fe->writer->overlapped)) {
			if (fe->writer->refer == 0) {
				mem_free(fe->writer);
			} else {
				fe->writer->fe = NULL;
			}
		} else {
			fe->writer->type = IOCP_EVENT_DEAD;
			fe->writer->fe   = NULL;
		}

		fe->writer = NULL;
	}

	if (fe->poller_read) {
		if (fe->poller_read->refer == 0) {
			mem_free(fe->poller_read);
		} else {
			fe->poller_read->fe = NULL;
		}
	}

	if (fe->poller_write) {
		if (fe->poller_write->refer == 0) {
			mem_free(fe->poller_write);
		} else {
			fe->poller_write->fe = NULL;
		}
	}

	//printf("------------fdcount=%d------------\r\n", ev->event.fdcount);
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
		if (fe->id < 0 || fe->id > ev->count) {
			assert(fe->id >= 0 && fe->id < ev->count);
		}
		if (ev->files[fe->id] != fe) {
			assert(ev->files[fe->id] == fe);
		}
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

	if (ret == TRUE) {
		fe->mask |= EVENT_READ;
		return 1;
	} else if (acl_fiber_last_error() == ERROR_IO_PENDING) {
		fe->mask |= EVENT_READ;
		return 0;
	} else {
		msg_warn("%s(%d): AcceptEx error(%s)",
			__FUNCTION__, __LINE__, last_serror());
		fe->mask |= EVENT_ERR;
		assert(fe->reader);
		array_append(ev->events, fe->reader);
		return 1;
	}
}

static int iocp_add_read(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	int ret;
	WSABUF wsaData;
	DWORD  flags = 0, len = 0;
	IOCP_EVENT *event;
	int is_listener = is_listen_socket(fe->fd);

	iocp_check(ev, fe);

	/* Check if the fe has been set STATUS_POLLING in io.c/poll.c/socket.c,
	 * and will set poller_write or writer IOCP_EVENT.
	 */
	if (IS_POLLING(fe)) {
		if (fe->poller_read == NULL) {
			fe->poller_read = (IOCP_EVENT*) mem_calloc(1, sizeof(IOCP_EVENT));
			fe->poller_read->refer = 0;
			fe->poller_read->fe    = fe;
			fe->poller_read->type  = IOCP_EVENT_POLLR;
		}
		event = fe->poller_read;
	} else {
		if (fe->reader == NULL) {
			fe->reader = (IOCP_EVENT*) mem_calloc(1, sizeof(IOCP_EVENT));
			fe->reader->refer = 0;
			fe->reader->fe    = fe;
			fe->reader->type = IOCP_EVENT_READ;
		}
		event = fe->reader;
	}

	event->proc = fe->r_proc;
	event->refer++;

	if (is_listener) {
		return iocp_add_listen(ev, fe);
	}

	/* If fe->buff has been set in io.c, we use it as overlapped buffer,
	 * or we must check if the socket is for UDP and being in poll reading
	 * status, if so, we must use the fixed buffer as UDP's reading buffer,
	 * because IOCP will discard UDP packet when no buffer provided.
	 */
	if (fe->buff != NULL && fe->size > 0) {
		wsaData.buf = fe->buff;
		wsaData.len = fe->size;
	} else if (IS_POLLING(fe) && fe->sock_type == SOCK_DGRAM) {
		fe->buff    = fe->packet;
		fe->size    = sizeof(fe->packet);
		fe->len     = 0;
		wsaData.buf = fe->packet;
		wsaData.len = fe->size;
	} else {
		wsaData.buf = fe->buff;
		wsaData.len = fe->size;
	}

	ret = WSARecv(fe->fd, &wsaData, 1, &len, &flags,
		(OVERLAPPED*) &event->overlapped, NULL);

	fe->len = (int) len;

	if (ret != SOCKET_ERROR) {
		fe->mask |= EVENT_READ;
		return 1;
	} else if (acl_fiber_last_error() == ERROR_IO_PENDING) {
		fe->mask |= EVENT_READ;
		return 0;
	} else {
		msg_warn("%s(%d): ReadFile error(%s), fd=%d",
			__FUNCTION__, __LINE__, acl_fiber_last_serror(), fe->fd);
		fe->mask |= EVENT_ERR;
#if 0
		fe->mask &= ~EVENT_READ;
		fe->len = -1;
		array_append(ev->events, event);
#else
		iocp_event_save(ev, event, fe, -1);
#endif
		return -1;
	}
}

int event_iocp_connect(EVENT *ev, FILE_EVENT *fe)
{
	EVENT_IOCP *ei = (EVENT_IOCP*) ev;
	DWORD SentLen = 0;
	DWORD len;
	struct sockaddr_in addr;
	LPFN_CONNECTEX lpfnConnectEx = NULL;
	GUID  GuidConnectEx = WSAID_CONNECTEX;
	int   dwErr, dwBytes;
	BOOL  ret;
	IOCP_EVENT *event;
	static const char *any_ip = "0.0.0.0";

	iocp_check(ei, fe);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(any_ip);
	addr.sin_port        = htons(0);

	// In IOCP, the local address must be bound first, or WSAEINVAL will
	// return when calling lpfnConnectEx.
	if (bind(fe->fd, (struct sockaddr *) &addr, sizeof(struct sockaddr)) < 0) {
		msg_error("%s(%d): bind local ip(%s) error(%s, %d), sock: %u",
			__FUNCTION__, __LINE__, any_ip, last_serror(),
			acl_fiber_last_error(), (unsigned) fe->fd);
		return -1;
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

	if (fe->poller_write == NULL) {
		fe->poller_write = (IOCP_EVENT*) mem_calloc(1, sizeof(IOCP_EVENT));
		fe->poller_write->refer = 0;
		fe->poller_write->fe    = fe;
		fe->poller_write->type = IOCP_EVENT_WRITE;
	}

	event = fe->poller_write;
	event->refer++;

	memset(&event->overlapped, 0, sizeof(event->overlapped));

	ret = lpfnConnectEx(fe->fd,
		(const struct sockaddr *) &fe->peer_addr,
		sizeof(struct sockaddr),
		NULL,
		0,
		&len,
		&event->overlapped);

	if (ret == TRUE) {
		return 0;
	} else if ((dwErr = acl_fiber_last_error()) == ERROR_IO_PENDING) {
		acl_fiber_set_error(FIBER_EINPROGRESS);
		return -1;
	} else {
		msg_warn("%s(%d): ConnectEx error(%s), sock(%u)",
			__FUNCTION__, __LINE__, last_serror(), fe->fd);
		iocp_event_save(ei, event, fe, -1);
		return -1;
	}
}

static int iocp_add_write(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	DWORD sendBytes;
	BOOL  ret;
	IOCP_EVENT *event;

	iocp_check(ev, fe);

	/* Check if the fe has been set STATUS_POLLING in io.c/poll.c/socket.c,
	 * and will set poller_write or writer IOCP_EVENT.
	 */
	if (IS_POLLING(fe) || fe->status & STATUS_CONNECTING) {
		if (fe->poller_write == NULL) {
			fe->poller_write = (IOCP_EVENT*) mem_calloc(1, sizeof(IOCP_EVENT));
			fe->poller_write->refer = 0;
			fe->poller_write->fe    = fe;
			fe->poller_write->type  = IOCP_EVENT_POLLW;
		}
		event = fe->poller_write;
	} else {
		if (fe->writer == NULL) {
			fe->writer        = (IOCP_EVENT*) mem_calloc(1, sizeof(IOCP_EVENT));
			fe->writer->refer = 0;
			fe->writer->fe    = fe;
			fe->writer->type = IOCP_EVENT_WRITE;
		}
		event = fe->writer;
	}

	event->proc = fe->w_proc;

	if (fe->status & STATUS_CONNECTING) {
		fe->mask |= EVENT_WRITE;
		return 0;
	}

	event->refer++;

	memset(&event->overlapped, 0, sizeof(event->overlapped));

	ret = WriteFile((HANDLE) fe->fd, NULL, 0, &sendBytes, &event->overlapped);

	if (ret == TRUE) {
		fe->mask |= EVENT_WRITE;
		return 0;
	} else if (acl_fiber_last_error() == ERROR_IO_PENDING) {
		fe->mask |= EVENT_WRITE;
		return 0;
	} else {
		msg_warn("%s(%d): WriteFile error(%d, %s)", __FUNCTION__,
			__LINE__, acl_fiber_last_error(), last_serror());
		fe->mask |= EVENT_ERR;
#if 0
		fe->mask &= ~EVENT_WRITE;
		fe->len = -1;
		array_append(ev->events, event);
#else
		iocp_event_save(ev, event, fe, -1);
#endif
		return -1;
	}
}

static int iocp_del_read(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	if (!(fe->mask & EVENT_READ)) {
		return 0;
	}

	assert(fe->id >= 0 && fe->id < ev->count);
	fe->mask &= ~EVENT_READ;

	if (fe->reader) {
		if (!CancelIoEx((HANDLE) fe->fd, &fe->reader->overlapped)) {
			msg_error("%s(%d): cancel read error %s, fd=%d", __FUNCTION__,
				__LINE__, acl_fiber_last_serror(), (int) fe->fd);
		}
		fe->reader->type &= ~IOCP_EVENT_READ;
	}
	if (fe->poller_read) {
		fe->poller_read->type &= ~IOCP_EVENT_POLLR;
	}

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

	if (fe->writer) {
		if (!CancelIoEx((HANDLE) fe->fd, &fe->writer->overlapped)) {
			msg_error("%s(%d): cancel write error %s, fd=%d", __FUNCTION__,
				__LINE__, acl_fiber_last_serror(), (int) fe->fd);
		}
		fe->writer->type &= ~IOCP_EVENT_WRITE;
	}
	if (fe->poller_write) {
		fe->poller_write->type &= ~IOCP_EVENT_POLLW;
	}

	if (fe->mask == 0) {
		iocp_remove(ev, fe);
	}
	return 0;
}

static void iocp_event_save(EVENT_IOCP *ei, IOCP_EVENT *event,
	FILE_EVENT *fe, DWORD trans)
{
	if ((event->type & (IOCP_EVENT_READ | IOCP_EVENT_POLLR))) {
		fe->mask &= ~EVENT_READ;
		CLR_READWAIT(fe);
	} else if ((event->type & (IOCP_EVENT_WRITE | IOCP_EVENT_POLLW))) {
		if (fe->status & STATUS_CONNECTING) {
			// Just for the calling of getpeername():
			// If the socket is ready for connecting server, we
			// should set SO_UPDATE_CONNECT_CONTEXT here, because
			// the peer address wasn't associated with the socket
			// automaticlly in IOCP mode.
			DWORD val = 1;
			setsockopt(fe->fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT,
				(char *)&val, sizeof(DWORD));
		}
		fe->mask &= ~EVENT_WRITE;
		CLR_WRITEWAIT(fe);
	}

	fe->len = (int) trans;
	array_append(ei->events, event);
}

static void iocp_wait_one(EVENT_IOCP *ei, int timeout)
{
	IOCP_EVENT *event;

	for (;;) {
		DWORD bytesTransferred;
		FILE_EVENT *fe;
		BOOL isSuccess;

		event = NULL;

		isSuccess = GetQueuedCompletionStatus(ei->h_iocp,
			&bytesTransferred, (PULONG_PTR) &fe,
			(OVERLAPPED**) &event, timeout);

		if (event == NULL) {
			break;
		}

		if (event->type & IOCP_EVENT_DEAD) {
			if (!HasOverlappedIoCompleted(&event->overlapped)) {
				msg_warn("overlapped not completed yet");
			}
			mem_free(event);
			continue;
		}

		event->refer--;

		// If the associated FILE_EVENT with the event has gone in
		// iocp_close_sock(), we should check the event's refer and
		// free it when refer is 0.

		if (event->fe == NULL) {
			if (event->refer == 0) {
				mem_free(event);
			}
			continue;
		}

		assert(fe);

		if (!isSuccess) {
			msg_error("%s(%d): fd=%d, GetQueuedCompletionStatus error=%d, %s",
				__FUNCTION__, __LINE__, (int) fe->fd, acl_fiber_last_error(),
				last_serror());
			iocp_event_save(ei, event, fe, bytesTransferred);
			fe->mask |= EVENT_ERR;
			continue;
		}

		if (fe != event->fe) {
			assert(fe == event->fe);
		}

		if (fe->mask & EVENT_ERR) {
			continue;
		}

		iocp_event_save(ei, event, fe, bytesTransferred);
		timeout = 0;

		// If I need to loop again ?
		break;
	}
}

static void handle_event(EVENT_IOCP *ei, OVERLAPPED_ENTRY *entry)
{
	IOCP_EVENT *event;
	DWORD bytesTransferred;
	FILE_EVENT *fe;

	event = (IOCP_EVENT*) entry->lpOverlapped;
	fe = (FILE_EVENT*) entry->lpCompletionKey;
	bytesTransferred = entry->dwNumberOfBytesTransferred;

	if (event->type & IOCP_EVENT_DEAD) {
		if (!HasOverlappedIoCompleted(&event->overlapped)) {
			msg_warn("overlapped not completed yet");
		}
		mem_free(event);
		return;
	}

	event->refer--;

	// If the associated FILE_EVENT with the event has gone in
	// iocp_close_sock(), we should check the event's refer and
	// free it when refer is 0.

	if (event->fe == NULL) {
		if (event->refer == 0) {
			mem_free(event);
		}
		return;
	}

	if (fe != event->fe) {
		assert(fe == event->fe);
	}

	if (fe->mask & EVENT_ERR) {
		return;
	}

	iocp_event_save(ei, event, fe, bytesTransferred);
}

static void iocp_wait_more(EVENT_IOCP *ei, int timeout)
{
#define MAX_ENTRIES	128
	OVERLAPPED_ENTRY entries[MAX_ENTRIES];
	unsigned long ready;

	for (;;) {
		BOOL isSuccess;
		unsigned long i;

		isSuccess = GetQueuedCompletionStatusEx(ei->h_iocp,
			entries, MAX_ENTRIES, &ready, timeout, FALSE);

		if (ready == 0) {
			break;
		}

		//printf(">>>ready: %d\r\n", ready);
		for (i = 0; i < ready; i++) {
			handle_event(ei, &entries[i]);
		}

		if (!isSuccess) {
			msg_error("%s(%d):GetQueuedCompletionStatus error=%d, %s",
				__FUNCTION__, __LINE__, acl_fiber_last_error(),
				last_serror());
		}

		timeout = 0;

		// If I need to loop again ?
		break;
	}
}

static int __use_wait_more = 1;

static int iocp_wait(EVENT *ev, int timeout)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) ev;
	IOCP_EVENT *event;

	if (__use_wait_more) {
		iocp_wait_more(ei, timeout);
	} else {
		iocp_wait_one(ei, timeout);

	}

	/* peek and handle all IOCP EVENT added in iocp_event_save(). */
	while ((event = (IOCP_EVENT*) ei->events->pop_back(ei->events)) != NULL) {
		if (event->proc && event->fe) {
			event->proc(ev, event->fe);
		}
	}

	return 0;
}

static void iocp_free(EVENT *ev)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) ev;

	if (ei->h_iocp) {
		CloseHandle(ei->h_iocp);
	}
	array_free(ei->events, NULL);
	mem_free(ei->files);
	mem_free(ei);
}

static int iocp_checkfd(EVENT_IOCP *ev, FILE_EVENT *fe)
{
	(void) ev;
	return getsockfamily(fe->fd) == -1 ? -1 : 0;
}

static acl_handle_t iocp_handle(EVENT *ev)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) ev;
	return (acl_handle_t) ei->h_iocp;
}

static const char *iocp_name(void)
{
	return "iocp";
}

EVENT *event_iocp_create(int size)
{
	EVENT_IOCP *ei = (EVENT_IOCP *) mem_calloc(1, sizeof(EVENT_IOCP));

	ei->h_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (ei->h_iocp == NULL) {
		msg_fatal("%s(%d): create iocp error(%s)",
			__FUNCTION__, __LINE__, last_serror());
	}

	ei->events = array_create(100);

	ei->files = (FILE_EVENT**) mem_calloc(size, sizeof(FILE_EVENT*));
	ei->size  = size;
	ei->count = 0;

	ei->event.name   = iocp_name;
	ei->event.handle = iocp_handle;
	ei->event.free   = iocp_free;
	ei->event.flag   = EVENT_F_IOCP;

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
