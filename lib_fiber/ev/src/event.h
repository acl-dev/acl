#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include <sys/epoll.h>
#include "fiber/lib_fiber.h"

#define SET_TIME(x) do { \
    struct timeval _tv; \
    gettimeofday(&_tv, NULL); \
    (x) = ((acl_int64) _tv.tv_sec) * 1000 + ((acl_int64) _tv.tv_usec)/ 1000; \
} while (0)

typedef struct POLLFD       POLLFD;
typedef struct FILE_EVENT   FILE_EVENT;
typedef struct POLL_CTX     POLL_CTX;
typedef struct POLL_EVENT   POLL_EVENT;
typedef struct EPOLL_CTX    EPOLL_CTX;
typedef struct EPOLL_EVENT  EPOLL_EVENT;
typedef struct EVENT        EVENT;

typedef void event_proc(EVENT *ev, FILE_EVENT *fe);
typedef void poll_proc(EVENT *ev, POLL_EVENT *pe);
typedef void epoll_proc(EVENT *ev, EPOLL_EVENT *ee);

/**
 * for each connection fd
 */
struct FILE_EVENT {
	ACL_RING   me;
	ACL_FIBER *fiber;
	int fd;
	int type;
#define	TYPE_NONE	0
#define	TYPE_SOCK	1
#define	TYPE_NOSOCK	2

	unsigned oper;
#define	EVENT_ADD_READ	(unsigned) (1 << 0)
#define	EVENT_ADD_WRITE	(unsigned) (1 << 1)
#define	EVENT_DEL_READ	(unsigned) (1 << 2)
#define	EVENT_DEL_WRITE	(unsigned) (1 << 3)

	unsigned mask;
#define	EVENT_NONE	0
#define	EVENT_READ	(unsigned) (1 << 0)
#define	EVENT_WRITE	(unsigned) (1 << 1)
#define	EVENT_ERROR	(unsigned) (1 << 2)

	event_proc   *r_proc;
	event_proc   *w_proc;
	POLLFD       *pfd;
	EPOLL_CTX    *epx;
};

struct POLLFD {
	FILE_EVENT *fe;
	POLL_EVENT *pe;
	struct pollfd *pfd;
};

struct POLL_EVENT {
	ACL_RING   me;
	ACL_FIBER *fiber;
	poll_proc *proc;
	int        nready;
	int        nfds;
	POLLFD    *fds;
};

struct EPOLL_CTX {
	int  fd;
	int  op;
	int  mask;
	int  rmask;
	FILE_EVENT  *fe;
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

struct EVENT {
	ACL_RING events;
	int   timeout;
	int   setsize;
	int   maxfd;

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
	void (*free)(EVENT *);

	int  (*event_loop)(EVENT *, int);

	event_proc *add_read;
	event_proc *add_write;
	event_proc *del_read;
	event_proc *del_write;
};

/* file_event.c */
void file_event_init(FILE_EVENT *fe, int fd);
FILE_EVENT *file_event_alloc(int fd);
void file_event_free(FILE_EVENT *fe);

/* event.c */
EVENT *event_create(int size);
const char *event_name(EVENT *ev);
int  event_handle(EVENT *ev);
int  event_size(EVENT *ev);
void event_free(EVENT *ev);

void event_add_read(EVENT *ev, FILE_EVENT *fe, event_proc *proc);
void event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc);
void event_del_read(EVENT *ev, FILE_EVENT *fe);
void event_del_write(EVENT *ev, FILE_EVENT *fe);
int  event_process(EVENT *ev, int left);

#endif
