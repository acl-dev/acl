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
#include "stdlib/acl_htable.h"
#include "net/acl_sane_socket.h"
#include "event/acl_events.h"

#endif  /* ACL_PREPARE_COMPILE */

#include "events_define.h"

#ifdef ACL_EVENTS_STYLE_WMSG

#define WM_SOCKET_NOTIFY	(WM_USER + 8192)

#include "events_fdtable.h"
#include "events.h"
#include "events_wmsg.h"

typedef struct EVENT_WMSG {
	ACL_EVENT event;
	UINT nMsg;
	HWND hWnd;
	HINSTANCE hInstance;
	const char *class_name;
	unsigned int tid;
	int   timer_active;
	ACL_HTABLE *htbl;
	void (*delay_close)(void *ctx);
	void *ctx;
} EVENT_WMSG;

static void stream_on_close(ACL_VSTREAM *stream, void *arg)
{
	const char *myname = "stream_on_close";
	EVENT_WMSG *ev = (EVENT_WMSG*) arg;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE*) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	char  key[64];

	if (fdp == NULL) {
		acl_msg_error("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	snprintf(key, sizeof(key), "%d", sockfd);
	acl_htable_delete(ev->htbl, key, NULL);

	/* 虽然该设置能取消以后的读写消息，但依然不能取消因为
	 * closesocket 而产生的 FD_CLOSE 消息
	 */
	WSAAsyncSelect(sockfd, ev->hWnd, 0, 0);

	/*
	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)
		&& (fdp->flag & EVENT_FDTABLE_FLAG_WRITE))
	{
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
	}
	*/

	if (ev->event.maxfd == ACL_VSTREAM_SOCK(fdp->stream))
		ev->event.maxfd = ACL_SOCKET_INVALID;
	if (fdp->fdidx >= 0 && fdp->fdidx < --ev->event.fdcnt) {
		ev->event.fdtabs[fdp->fdidx] = ev->event.fdtabs[ev->event.fdcnt];
		ev->event.fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
		fdp->fdidx = -1;
	}

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < ev->event.ready_cnt
		&& ev->event.ready[fdp->fdidx_ready] == fdp)
	{
		ev->event.ready[fdp->fdidx_ready] = NULL;
		fdp->fdidx_ready = -1;
	}
	event_fdtable_free(fdp);
	stream->fdp = NULL;
}

static ACL_EVENT_FDTABLE *stream_on_open(EVENT_WMSG *ev, ACL_VSTREAM *stream)
{
	const char *myname = "stream_on_open";
	ACL_EVENT_FDTABLE *fdp = event_fdtable_alloc();
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	char  key[64];

	fdp->stream = stream;
	stream->fdp = (void *) fdp;
	acl_vstream_add_close_handle(stream, stream_on_close, ev);

	snprintf(key, sizeof(key), "%d", sockfd);
	if (acl_htable_enter(ev->htbl, key, fdp) == NULL)
		acl_msg_fatal("%s(%d): add key(%s) error",
			myname, __LINE__, key);

	return fdp;
}

static ACL_EVENT_FDTABLE *read_enable(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context)
{
	const char *myname = "read_enable";
	EVENT_WMSG *ev = (EVENT_WMSG *) eventp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	long lEvent;

	if (fdp == NULL)
		fdp = stream_on_open(ev, stream);

	if (fdp->fdidx == -1) {
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt++] = fdp;
	}

	if ((fdp->stream->type & ACL_VSTREAM_TYPE_LISTEN)) {
		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;
		lEvent = FD_ACCEPT | FD_READ | FD_CLOSE;
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		fdp->flag |= EVENT_FDTABLE_FLAG_READ;
		lEvent = FD_READ | FD_WRITE | FD_CLOSE;
	} else {
		fdp->flag = EVENT_FDTABLE_FLAG_READ | EVENT_FDTABLE_FLAG_EXPT;
		lEvent = FD_READ | FD_CLOSE;
	}

	if (WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, lEvent) != 0)
		acl_msg_fatal("%s(%d): set read error: %s",
			myname, __LINE__, acl_last_serror());

	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->r_callback != callback || fdp->r_context != context) {
		fdp->r_callback = callback;
		fdp->r_context = context;
	}

	if (timeout > 0) {
		fdp->r_timeout = timeout * 1000000;
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
	const char *myname = "event_enable_write";
	EVENT_WMSG *ev = (EVENT_WMSG *) eventp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	long lEvent;

	if (fdp == NULL)
		fdp = stream_on_open(ev, stream);

	if (fdp->fdidx == -1) {
		fdp->fdidx = eventp->fdcnt;
		eventp->fdtabs[eventp->fdcnt++] = fdp;
	}

	if ((fdp->stream->flag & ACL_VSTREAM_FLAG_CONNECTING)) {
		fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		lEvent = FD_CONNECT | FD_WRITE | FD_CLOSE;
	} else if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		fdp->flag |= EVENT_FDTABLE_FLAG_WRITE;
		lEvent = FD_READ | FD_WRITE | FD_CLOSE;
	} else {
		fdp->flag = EVENT_FDTABLE_FLAG_WRITE | EVENT_FDTABLE_FLAG_EXPT;
		lEvent = FD_WRITE | FD_CLOSE;
	}

	if (WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, lEvent) != 0)
		acl_msg_fatal("%s(%d): set read error: %s",
			myname, __LINE__, acl_last_serror());

	if (eventp->maxfd != ACL_SOCKET_INVALID && eventp->maxfd < sockfd)
		eventp->maxfd = sockfd;

	if (fdp->w_callback != callback || fdp->w_context != context) {
		fdp->w_callback = callback;
		fdp->w_context = context;
	}

	if (timeout > 0) {
		fdp->w_timeout = timeout * 1000000;
		fdp->w_ttl = eventp->present + fdp->w_timeout;
	} else {
		fdp->w_ttl = 0;
		fdp->w_timeout = 0;
	}
}

static void event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_read";
	EVENT_WMSG *ev = (EVENT_WMSG *) eventp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx invalid",
			myname, __LINE__, sockfd);
		return;
	}

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		acl_msg_warn("%s(%d): sockfd(%d) not in rmask",
			myname, __LINE__, sockfd);
		return;
	}

	fdp->r_ttl = 0;
	fdp->r_timeout = 0;
	fdp->r_callback = NULL;
	fdp->event_type &= ~(ACL_EVENT_READ | ACL_EVENT_ACCEPT);
	fdp->flag &= ~EVENT_FDTABLE_FLAG_READ;

	if ((fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, FD_WRITE | FD_CLOSE);
		return;
	}

	if (eventp->maxfd == sockfd)
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

	WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, FD_CLOSE);
}

/* event_disable_write - disable request for write events */

static void event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_write";
	EVENT_WMSG *ev = (EVENT_WMSG *) eventp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return;
	}

	if (fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d)'s fdidx invalid",
			myname, __LINE__, sockfd);
		return;
	}

	if (!(fdp->flag & EVENT_FDTABLE_FLAG_WRITE)) {
		acl_msg_warn("%s(%d): sockfd(%d) not in wmask",
			myname, __LINE__, sockfd);
		return;
	}

	fdp->w_ttl = 0;
	fdp->w_timeout = 0;
	fdp->w_callback = NULL;
	fdp->event_type &= ~(ACL_EVENT_WRITE | ACL_EVENT_CONNECT);
	fdp->flag &= ~EVENT_FDTABLE_FLAG_WRITE;

	if ((fdp->flag & EVENT_FDTABLE_FLAG_READ)) {
		WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, FD_READ | FD_CLOSE);
		return;
	}

	if (eventp->maxfd == sockfd)
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

	WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, FD_CLOSE);
}

/* event_disable_readwrite - disable request for read or write events */

static void event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream)
{
	const char *myname = "event_disable_readwrite";
	EVENT_WMSG *ev = (EVENT_WMSG *) eventp;
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);

	if (fdp == NULL)
		return;

	if (fdp->flag == 0 || fdp->fdidx < 0 || fdp->fdidx >= eventp->fdcnt) {
		acl_msg_warn("%s(%d): sockfd(%d) no set, fdp no null",
			myname, __LINE__, sockfd);
		event_fdtable_free(fdp);
		stream->fdp = NULL;
		return;
	}

	if (!eventp->isrset_fn(eventp, stream)
		&& !eventp->iswset_fn(eventp, stream))
	{
		acl_msg_error("%s(%d): sockfd(%d) no set, fdp no null",
			myname, __LINE__, sockfd);
		event_fdtable_free(fdp);
		stream->fdp = NULL;
		return;
	}

	if (eventp->maxfd == sockfd)
		eventp->maxfd = ACL_SOCKET_INVALID;

	if (fdp->fdidx < --eventp->fdcnt) {
		eventp->fdtabs[fdp->fdidx] = eventp->fdtabs[eventp->fdcnt];
		eventp->fdtabs[fdp->fdidx]->fdidx = fdp->fdidx;
	}
	fdp->fdidx = -1;

	WSAAsyncSelect(sockfd, ev->hWnd, ev->nMsg, FD_CLOSE);

	if (fdp->fdidx_ready >= 0
		&& fdp->fdidx_ready < eventp->ready_cnt
		&& eventp->ready[fdp->fdidx_ready] == fdp)
	{
		eventp->ready[fdp->fdidx_ready] = NULL;
	}
	fdp->fdidx_ready = -1;
	event_fdtable_free(fdp);
	stream->fdp = NULL;
}

static int event_isrset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	const char *myname = "event_isrset";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return 0;
	}

	return (fdp->flag & EVENT_FDTABLE_FLAG_READ) == 0 ? 0 : 1;
}

static int event_iswset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	const char *myname = "event_iswset";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return 0;
	}

	return (fdp->flag & EVENT_FDTABLE_FLAG_WRITE) == 0 ? 0 : 1;
}

static int event_isxset(ACL_EVENT *eventp acl_unused, ACL_VSTREAM *stream)
{
	const char *myname = "event_isxset";
	ACL_SOCKET sockfd = ACL_VSTREAM_SOCK(stream);
	ACL_EVENT_FDTABLE *fdp = (ACL_EVENT_FDTABLE *) stream->fdp;

	if (fdp == NULL) {
		acl_msg_warn("%s(%d): fdp null", myname, __LINE__);
		return 0;
	}

	return (fdp->flag & EVENT_FDTABLE_FLAG_EXPT) == 0 ? 0 : 1;
}

#ifdef USE_TLS

static acl_pthread_key_t __event_key;
static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

static EVENT_WMSG *get_hwnd_event(HWND hWnd acl_unused)
{
	EVENT_WMSG *ev = acl_pthread_getspecific(__event_key);
	return ev;
}

static void finish_thread_event(void *arg acl_unused)
{

}

static void init_thread_event(void)
{
	acl_pthread_key_create(&__event_key, finish_thread_event);
}

static void set_hwnd_event(HWND hWnd acl_unused, EVENT_WMSG *ev)
{
	const char *myname = "set_hwnd_event";

	(void) acl_pthread_once(&once_control, init_thread_event);
	if (acl_pthread_getspecific(__event_key) != NULL)
		acl_msg_fatal("%s(%d): __event_key(%d)'s value not null",
			myname, __LINE__, (int) __event_key);
	acl_pthread_setspecific(__event_key, ev);
}

#else

static EVENT_WMSG *get_hwnd_event(HWND hWnd)
{
	EVENT_WMSG *ev = (EVENT_WMSG*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
	return ev;
}

static void set_hwnd_event(HWND hWnd, EVENT_WMSG *ev)
{
	SetWindowLongPtr(hWnd, GWLP_USERDATA, (ULONG_PTR) ev);
}

#endif

static ACL_EVENT_FDTABLE *event_fdtable_find(EVENT_WMSG *ev, ACL_SOCKET sockfd)
{
	ACL_EVENT_FDTABLE *fdp;
	char key[64];

	snprintf(key, sizeof(key), "%d", sockfd);
	fdp = acl_htable_find(ev->htbl, key);
	return fdp;
}

static void handleClose(EVENT_WMSG *ev, ACL_SOCKET sockfd)
{
	const char *myname = "handleClose";
	ACL_EVENT_FDTABLE *fdp = event_fdtable_find(ev, sockfd);

	if (fdp == NULL)
		return;
	else if (fdp->r_callback)
		fdp->r_callback(ACL_EVENT_XCPT, &ev->event,
			fdp->stream, fdp->r_context);
	else if (fdp->w_callback)
		fdp->w_callback(ACL_EVENT_XCPT, &ev->event,
			fdp->stream, fdp->w_context);
	/*
	else
		acl_msg_error("%s(%d): w_callback and r_callback null"
			" for sockfd(%d)", myname, __LINE__, (int) sockfd);
	*/
}

static void handleConnect(EVENT_WMSG *ev, ACL_SOCKET sockfd)
{
	const char *myname = "handleConnect";
	ACL_EVENT_FDTABLE *fdp = event_fdtable_find(ev, sockfd);

	if (fdp == NULL)
		acl_msg_error("%s(%d): fdp null for sockfd(%d)",
			myname, __LINE__, (int) sockfd);
	else if (fdp->w_callback == NULL)
		acl_msg_error("%s(%d): fdp->w_callback null for sockfd(%d)",
			myname, __LINE__, (int) sockfd);
	else {
		fdp->stream->flag &= ~ACL_VSTREAM_FLAG_CONNECTING;
		fdp->w_callback(ACL_EVENT_WRITE | ACL_EVENT_CONNECT,
			&ev->event, fdp->stream, fdp->w_context);
	}
}

static void handleAccept(EVENT_WMSG *ev, ACL_SOCKET sockfd)
{
	const char *myname = "handleAccept";
	ACL_EVENT_FDTABLE *fdp = event_fdtable_find(ev, sockfd);

	if (fdp == NULL)
		acl_msg_fatal("%s(%d): fdp null", myname, __LINE__);
	else if (fdp->r_callback == NULL)
		acl_msg_fatal("%s(%d): fdp callback null", myname, __LINE__);

	fdp->r_callback(ACL_EVENT_READ | ACL_EVENT_ACCEPT, &ev->event,
		fdp->stream, fdp->r_context);
}

static void handleRead(EVENT_WMSG *ev, ACL_SOCKET sockfd)
{
	const char *myname = "handleRead";
	ACL_EVENT_FDTABLE *fdp = event_fdtable_find(ev, sockfd);

	if (fdp == NULL)
		acl_msg_error("%s(%d): fdp null for sockfd(%d)",
			myname, __LINE__, (int) sockfd);
	else if ((fdp->stream->type & ACL_VSTREAM_TYPE_LISTEN))
		fdp->r_callback(ACL_EVENT_READ, &ev->event,
			fdp->stream, fdp->r_context);
	else if (fdp->r_callback != NULL) {
		/* 该描述字可读则设置 ACL_VSTREAM 的系统可读标志从而触发
		 * ACL_VSTREAM 流在读时调用系统的 read 函数
		 */
		fdp->stream->read_ready = 1;
		fdp->r_callback(ACL_EVENT_READ, &ev->event,
			fdp->stream, fdp->r_context);
	}
	/* else
		acl_msg_error("%s(%d): fdp->r_callback null for sockfd(%d)",
			myname, __LINE__, (int) sockfd);
	*/
}

static void handleWrite(EVENT_WMSG *ev, ACL_SOCKET sockfd)
{
	const char *myname = "handleWrite";
	ACL_EVENT_FDTABLE *fdp = event_fdtable_find(ev, sockfd);

	if (fdp == NULL)
		acl_msg_error("%s(%d): fdp null for sockfd(%d)",
			myname, __LINE__, (int) sockfd);
	else if ((fdp->stream->flag & ACL_VSTREAM_FLAG_CONNECTING))
		handleConnect(ev, sockfd);
	else if (fdp->w_callback != NULL)
		fdp->w_callback(ACL_EVENT_WRITE, &ev->event,
			fdp->stream, fdp->w_context);
	/*
	else
		acl_msg_error("%s(%d): fdp->w_callback null for sockfd(%d)",
			myname, __LINE__, (int) sockfd);
	*/
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ACL_SOCKET sockfd;
	EVENT_WMSG *ev = get_hwnd_event(hWnd);

	if (ev == NULL)
		return (DefWindowProc(hWnd, msg, wParam, lParam));

	if (msg == WM_SOCKET_NOTIFY) {
		sockfd = wParam;
		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
			handleAccept(ev, sockfd);
			break;
		case FD_CONNECT:
			handleConnect(ev, sockfd);
			break;
		case FD_READ:
			handleRead(ev, sockfd);
			break;
		case FD_WRITE:
			handleWrite(ev, sockfd);
			break;
		case FD_CLOSE:
			handleClose(ev, sockfd);
			break;
		default:
			break;
		}
	}

	return (DefWindowProc(hWnd, msg, wParam, lParam));
}

static BOOL InitApplication(const char *class_name, HINSTANCE hInstance) 
{
	const char *myname = "InitApplication";
	WNDCLASSEX wcx;

	if (GetClassInfoEx(hInstance, class_name, &wcx))
	{
		/* class already registered */
		acl_msg_info("%s(%d): class(%s) already registered",
			myname, __LINE__, class_name);
		return TRUE;
	}

	/* Fill in the window class structure with parameters
	 * that describe the main window.
	 */

	memset(&wcx, 0, sizeof(wcx));

	wcx.cbSize = sizeof(wcx);          /* size of structure */
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = WndProc;         /* points to window procedure */
	wcx.cbClsExtra = 0;                /* no extra class memory */
	wcx.cbWndExtra = 0;                /* no extra window memory */
	wcx.hInstance = hInstance;         /* handle to instance */

	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);    /* predefined app. icon */
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);      /* predefined arrow */
	wcx.hbrBackground = GetStockObject(WHITE_BRUSH); /* white background brush */
	wcx.lpszMenuName =  NULL;          /* name of menu resource */
	wcx.lpszClassName = class_name;    /* name of window class */
	wcx.hIconSm = LoadImage(hInstance, /* small class icon */
		MAKEINTRESOURCE(5),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	/* Register the window class. */
	if (RegisterClassEx(&wcx) == 0) {
		acl_msg_error("%s(%d): RegisterClassEx error(%d, %s)",
			myname, __LINE__, acl_last_error(), acl_last_serror());
		return (FALSE);
	} else
		return (TRUE);
}

static HWND InitInstance(const char *class_name, HINSTANCE hInstance)
{
	const char *myname = "InitInstance";
	HWND hWnd;
	CREATESTRUCT cs;

	cs.dwExStyle = 0;
	cs.lpszClass = class_name;
	cs.lpszName = "Acl Socket Notification Sink";
	cs.style = WS_OVERLAPPED;
	cs.x = 0;
	cs.y = 0;
	cs.cx = 0;
	cs.cy = 0;
	cs.hwndParent = NULL;
	cs.hMenu = NULL;
	cs.hInstance = hInstance;
	cs.lpCreateParams = NULL;

	hWnd = CreateWindowEx(cs.dwExStyle, cs.lpszClass,
		cs.lpszName, cs.style, cs.x, cs.y, cs.cx, cs.cy,
		cs.hwndParent, cs.hMenu, cs.hInstance, cs.lpCreateParams);
	if (hWnd == NULL)
		acl_msg_error("%s(%d): create windows error: %s",
			myname, __LINE__, acl_last_serror());
	return hWnd;
}

static HWND CreateSockWindow(const char *class_name, HINSTANCE hInstance)
{
	if (InitApplication(class_name, hInstance) == FALSE)
		return (FALSE);
	return InitInstance(class_name, hInstance);
}

static void event_loop(ACL_EVENT *eventp acl_unused)
{

}

static VOID CALLBACK event_timer_callback(HWND hwnd, UINT uMsg,
	UINT_PTR idEvent, DWORD dwTime)
{
	const char *myname = "event_timer_callback";
	EVENT_WMSG *ev = get_hwnd_event(hwnd);
	ACL_EVENT *eventp;
	ACL_EVENT_TIMER *timer;
	ACL_EVENT_NOTIFY_TIME timer_fn;
	void    *timer_arg;

	if (ev == NULL)
		acl_msg_fatal("%s(%d): ev null", myname, __LINE__);
	if (ev->tid != idEvent)
		acl_msg_fatal("%s(%d): ev->tid(%u) != idEvent(%u)",
			myname, __LINE__, (unsigned int) ev->tid,
			(unsigned int) idEvent);

	eventp = &ev->event;
	SET_TIME(eventp->present);

	while ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) != 0) {
		if (timer->when > eventp->present)
			break;
		timer_fn  = timer->callback;
		timer_arg = timer->context;

		/* 如果定时器的时间间隔 > 0 且允许定时器被循环调用，则再重设定时器 */
		if (timer->delay > 0 && timer->keep) {
			timer->ncount++;
			eventp->timer_request(eventp, timer->callback,
				timer->context, timer->delay, timer->keep);
		} else {
			acl_ring_detach(&timer->ring);		/* first this */
			timer->nrefer--;
			if (timer->nrefer != 0)
				acl_msg_fatal("%s(%d): nrefer(%d) != 0",
					myname, __LINE__, timer->nrefer);
			acl_myfree(timer);
		}
		timer_fn(ACL_EVENT_TIME, eventp, timer_arg);
	}

	if ((timer = ACL_FIRST_TIMER(&eventp->timer_head)) == 0) {
		KillTimer(hwnd, idEvent);
		ev->timer_active = 0;
	} else {
		int  delay;

		SET_TIME(eventp->present);
		delay = (int) (timer->when - eventp->present + 999) / 1000;

		/* 要求时间定时器的间隔最少是 1 毫秒 */
		if (delay < 1000)
			delay = 1000;
		SetTimer(ev->hWnd, ev->tid, delay, event_timer_callback);
	}
}

static acl_int64 event_set_timer(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, acl_int64 delay, int keep)
{
	EVENT_WMSG *ev = (EVENT_WMSG*) eventp;
	ACL_EVENT_TIMER *timer;
	acl_int64 when;
	acl_int64 first_delay;

	/* 要求时间定时器的间隔最少是 1 毫秒 */
	if (delay < 1000)
		delay = 1000;

	timer = ACL_FIRST_TIMER(&eventp->timer_head);
	if (timer == NULL)
		first_delay = -1;
	else {
		SET_TIME(eventp->present);
		first_delay = timer->when - eventp->present;
		if (first_delay < 0)
			first_delay = 0;
	}

	when = event_timer_request(eventp, callback, context, delay, keep);

	if (ev->timer_active == 0) {
		/* set the new timer */
		SetTimer(ev->hWnd, ev->tid, (unsigned int) delay / 1000,
			event_timer_callback);
		ev->timer_active = 1;
	} else if (first_delay > delay) {
		/* reset the old timer */
		SetTimer(ev->hWnd, ev->tid, (unsigned int) delay / 1000,
			event_timer_callback);
	}

	return when;
}

static acl_int64 event_del_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context)
{
	EVENT_WMSG *ev = (EVENT_WMSG*) eventp;
	acl_int64 when = event_timer_cancel(eventp, callback, context);

	if (ev->timer_active && ACL_FIRST_TIMER(&eventp->timer_head) == 0) {
		KillTimer(ev->hWnd, ev->tid);
		ev->timer_active = 0;
	}
	return when;
}

static void event_free(ACL_EVENT *eventp)
{
	const char *myname = "event_free";
	EVENT_WMSG *ev = (EVENT_WMSG *) eventp;

	if (eventp == NULL)
		acl_msg_fatal("%s(%d): eventp null", myname, __LINE__);

	if (ev->hWnd != NULL) {
		WNDCLASSEX wcx;

		DestroyWindow(ev->hWnd);
		if (ev->class_name && GetClassInfoEx(ev->hInstance,
			ev->class_name, &wcx))
		{
			acl_msg_info("unregister class: %s", ev->class_name);
			UnregisterClass(ev->class_name, ev->hInstance);
		}
	}
	acl_htable_free(ev->htbl, NULL);
	acl_myfree(ev);
}

static const char *__class_name = "__AclEventsMainWClass";
ACL_EVENT *event_new_wmsg(UINT nMsg)
{
	ACL_EVENT *eventp;
	EVENT_WMSG *ev;

	HINSTANCE hInstance = GetModuleHandle(NULL);
	HWND hWnd = CreateSockWindow(__class_name, hInstance);

	if (hWnd == NULL)
		return (NULL);

	eventp = event_alloc(sizeof(EVENT_WMSG));

	snprintf(eventp->name, sizeof(eventp->name), "events - wmsg");
	eventp->event_mode           = ACL_EVENT_WMSG;
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
	eventp->timer_request        = event_set_timer;
	eventp->timer_cancel         = event_del_timer;
	eventp->timer_keep           = event_timer_keep;
	eventp->timer_ifkeep         = event_timer_ifkeep;

	ev = (EVENT_WMSG*) eventp;
	ev->nMsg = nMsg > 0 ? nMsg : WM_SOCKET_NOTIFY;
	ev->htbl = acl_htable_create(100, 0);
	ev->hWnd = hWnd;
	ev->hInstance = hInstance;
	ev->class_name = __class_name;
	ev->tid = acl_pthread_self();
	ev->timer_active = 0;
	ev->delay_close = NULL;
	ev->ctx = NULL;

	set_hwnd_event(hWnd, ev);
	return eventp;
}

HWND acl_event_wmsg_hwnd(ACL_EVENT *eventp)
{
	EVENT_WMSG *ev = (EVENT_WMSG*) eventp;
	return ev->hWnd;
}

#endif /* ACL_EVENTS_STYLE_WMSG */
