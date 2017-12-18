#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include <sys/epoll.h>
#include "fiber/lib_fiber.h"

#define	TYPE_NONE	0
#define	TYPE_SOCK	1
#define	TYPE_NOSOCK	2

#define	EVENT_NONE	0
#define	EVENT_READABLE	(unsigned) (1 << 0)
#define	EVENT_WRITABLE	(unsigned) (1 << 1)
#define	EVENT_ERROR	(unsigned) (1 << 2)

typedef struct FILE_EVENT   FILE_EVENT;
typedef struct POLL_CTX     POLL_CTX;
typedef struct POLL_EVENT   POLL_EVENT;
typedef struct EPOLL_CTX    EPOLL_CTX;
typedef struct EPOLL_EVENT  EPOLL_EVENT;
typedef struct FIRED_EVENT  FIRED_EVENT;
typedef struct DEFER_DELETE DEFER_DELETE;
typedef struct EVENT        EVENT;

typedef void event_proc(EVENT *ev, int fd, void *ctx, int mask);
typedef void poll_proc(EVENT *ev, POLL_EVENT *pe);
typedef void epoll_proc(EVENT *ev, EPOLL_EVENT *ee);

struct FILE_EVENT {
	int type;
	int mask;
	int mask_fired;
	event_proc   *r_proc;
	event_proc   *w_proc;
	void         *r_ctx;
	void         *w_ctx;
	POLL_EVENT   *pe;
	DEFER_DELETE *r_defer;
	DEFER_DELETE *w_defer;
};

struct POLL_EVENT {
	ACL_RING   me;
	ACL_FIBER *fiber;
	poll_proc *proc;
	int        nready;
	int        nfds;
	struct pollfd *fds;
};

struct EPOLL_CTX {
	int  fd;
	int  op;
	int  mask;
	int  rmask;
	EPOLL_EVENT *ee;
	epoll_data_t data;
};

struct EPOLL_EVENT {
	ACL_RING    me;
	ACL_FIBER  *fiber;
	epoll_proc *proc;
	size_t      nfds;
	EPOLL_CTX **fds;
	int         epfd;

	struct epoll_event *events;
	int maxevents;
	int nready;
};

struct FIRED_EVENT {
	int fd;
	int mask;
};

struct DEFER_DELETE {
	int fd;
	int mask;
	int pos;
};

struct EVENT {
	int   timeout;
	int   setsize;
	int   maxfd;
	int   r_ndefer;
	int   w_ndefer;
	FILE_EVENT   *events;
	FIRED_EVENT  *fired;
	DEFER_DELETE *r_defers;
	DEFER_DELETE *w_defers;

#define	USE_RING
#ifdef	USE_RING		// xxx: some bugs ?
	ACL_RING   poll_list;
	ACL_RING   epoll_list;
#elif	defined(USE_STACK)
	ACL_STACK *poll_list;
	ACL_STACK *epoll_list;
#else
	ACL_FIFO  *poll_list;
	ACL_FIFO  *epoll_list;
#endif

	const char *(*name)(void);
	int  (*handle)(EVENT *);
	int  (*loop)(EVENT *, int);
	int  (*add)(EVENT *, int, int);
	int  (*del)(EVENT *, int, int);
	void (*free)(EVENT *);
};

EVENT *event_create(int size);
const char *event_name(EVENT *ev);
int  event_handle(EVENT *ev);
int  event_size(EVENT *ev);
void event_free(EVENT *ev);
int  event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx);
void event_del(EVENT *ev, int fd, int mask);
void event_del_nodelay(EVENT *ev, int fd, int mask);
int  event_process(EVENT *ev, int left);
int  event_readable(EVENT *ev, int fd);
int  event_writeable(EVENT *ev, int fd);
void event_clear_readable(EVENT *ev, int fd);
void event_clear_writeable(EVENT *ev, int fd);
void event_clear(EVENT *ev, int fd);

#endif
