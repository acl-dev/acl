#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include "define.h"
#include "common/gettimeofday.h"
#include "common/timer_cache.h"

#ifdef	HAS_EPOLL
#include <sys/epoll.h>
#endif
#include "fiber/libfiber.h"

#if defined(USE_FAST_TIME)
#define SET_TIME(x) do { \
    struct timeval _tv; \
    acl_fiber_gettimeofday(&_tv, NULL); \
    (x) = ((long long) _tv.tv_sec) * 1000 + ((long long) _tv.tv_usec)/ 1000; \
} while (0)
#else
#define SET_TIME(x) do { \
struct timeval _tv; \
    gettimeofday(&_tv, NULL); \
    (x) = ((long long) _tv.tv_sec) * 1000 + ((long long) _tv.tv_usec)/ 1000; \
} while (0)
#endif

typedef struct FILE_EVENT   FILE_EVENT;
typedef struct EVENT        EVENT;

#ifdef HAS_POLL
typedef struct POLLFD       POLLFD;
typedef struct POLL_CTX     POLL_CTX;
typedef struct POLL_EVENT   POLL_EVENT;
#endif

#ifdef HAS_EPOLL
typedef struct EPOLL_CTX    EPOLL_CTX;
typedef struct EPOLL_EVENT  EPOLL_EVENT;
#endif

typedef int  event_oper(EVENT *ev, FILE_EVENT *fe);
typedef void event_proc(EVENT *ev, FILE_EVENT *fe);

#ifdef HAS_POLL
typedef void poll_proc(EVENT *ev, POLL_EVENT *pe);
#endif

#ifdef HAS_EPOLL
typedef void epoll_proc(EVENT *ev, EPOLL_EVENT *ee);
#endif

#ifdef HAS_IOCP
typedef struct IOCP_EVENT IOCP_EVENT;
#endif

/**
 * for each connection fd
 */
struct FILE_EVENT {
	RING       me;
	ACL_FIBER *fiber_r;
	ACL_FIBER *fiber_w;
	socket_t   fd;
	int id;
	unsigned status;
#define	STATUS_NONE		0
#define	STATUS_CONNECTING	(unsigned) (1 << 0)	// In connecting status
#define	STATUS_READABLE		(unsigned) (1 << 1)	// Ready for reading
#define	STATUS_WRITABLE		(unsigned) (1 << 2)	// Ready for writing
#define	STATUS_POLLING		(unsigned) (1 << 3)	// In polling status
#define	STATUS_NDUBLOCK		(unsigned) (1 << 4)	// If need to set unblock
#define	STATUS_READWAIT		(unsigned) (1 << 5)	// Wait for readable
#define	STATUS_WRITEWAIT	(unsigned) (1 << 6)	// Wait for Writable
#define	STATUS_CLOSING		(unsigned) (1 << 7)	// In closing status
#define	STATUS_CLOSED		(unsigned) (1 << 8)	// In closed status

#define	SET_CONNECTING(x)	((x)->status |= STATUS_CONNECTING)
#define	SET_READABLE(x)		((x)->status |= STATUS_READABLE)
#define	SET_WRITABLE(x)		((x)->status |= STATUS_WRITABLE)
#define	SET_POLLING(x)		((x)->status |= STATUS_POLLING)
#define	SET_NDUBLOCK(x)		((x)->status |= STATUS_NDUBLOCK)
#define	SET_READWAIT(x)		((x)->status |= STATUS_READWAIT)
#define	SET_WRITEWAIT(x)	((x)->status |= STATUS_WRITEWAIT)
#define	SET_CLOSING(x)		((x)->status |= STATUS_CLOSING)
#define	SET_CLOSED(x)		((x)->status |= STATUS_CLOSED)

#define	CLR_CONNECTING(x)	((x)->status &= ~STATUS_CONNECTING)
#define	CLR_READABLE(x)		((x)->status &= ~STATUS_READABLE)
#define	CLR_WRITABLE(x)		((x)->status &= ~STATUS_WRITABLE)
#define	CLR_POLLING(x)		((x)->status &= ~STATUS_POLLING)
#define	CLR_NDUBLOCK(x)		((x)->status &= ~STATUS_NDUBLOCK)
#define	CLR_READWAIT(x)		((x)->status &= ~STATUS_READWAIT)
#define	CLR_WRITEWAIT(x)	((x)->status &= ~STATUS_WRITEWAIT)
#define	CLR_CLOSING(x)		((x)->status &= ~STATUS_CLOSING)
#define	CLR_CLOSED(x)		((x)->status &= ~STATUS_CLOSED)

#define	IS_CONNECTING(x)	((x)->status & STATUS_CONNECTING)
#define	IS_READABLE(x)		((x)->status & STATUS_READABLE)
#define	IS_WRITABLE(x)		((x)->status & STATUS_WRITABLE)
#define	IS_POLLING(x)		((x)->status & STATUS_POLLING)
#define	IS_NDUBLOCK(x)		((x)->status & STATUS_NDUBLOCK)
#define	IS_READWAIT(x)		((x)->status & STATUS_READWAIT)
#define	IS_WRITEWAIT(x)		((x)->status & STATUS_WRITEWAIT)
#define	IS_CLOSING(x)		((x)->status & STATUS_CLOSING)
#define	IS_CLOSED(x)		((x)->status & STATUS_CLOSED)

	unsigned type;
#define	TYPE_NONE		0
#define	TYPE_SPIPE		1
#define	TYPE_FILE		2
#define	TYPE_BADFD		3

	unsigned oper;
#define	EVENT_ADD_READ		(unsigned) (1 << 0)
#define	EVENT_DEL_READ		(unsigned) (1 << 1)
#define	EVENT_ADD_WRITE		(unsigned) (1 << 2)
#define	EVENT_DEL_WRITE		(unsigned) (1 << 3)

	unsigned mask;
#define	EVENT_NONE		0
#define	EVENT_READ		(unsigned) (1 << 0)
#define	EVENT_WRITE		(unsigned) (1 << 1)
#define	EVENT_ERR		(unsigned) (1 << 2)
#define	EVENT_HUP		(unsigned) (1 << 3)
#define	EVENT_NVAL		(unsigned) (1 << 4)

	event_proc   *r_proc;
	event_proc   *w_proc;
#ifdef HAS_POLL
	POLLFD       *pfd;
#endif
#ifdef HAS_EPOLL
	EPOLL_CTX    *epx;
#endif

#ifdef HAS_IOCP
	char          packet[1500];  // just for UDP packet
	char         *buff;
	int           size;
	int           len;
	HANDLE        h_iocp;
	IOCP_EVENT   *reader;
	IOCP_EVENT   *writer;
	IOCP_EVENT   *poller_read;
	IOCP_EVENT   *poller_write;
	socket_t      iocp_sock;
	int           sock_type;
	struct sockaddr_in peer_addr;
#endif
};

#ifdef HAS_POLL
struct POLL_EVENT {
	RING       me;

	ACL_FIBER *fiber;
	poll_proc *proc;
	long long  expire;
	int        nready;
	int        nfds;
	POLLFD    *fds;
};
#endif

#ifdef	HAS_EPOLL

typedef struct EPOLL EPOLL;

struct EPOLL_EVENT {
	RING        me;
	ACL_FIBER  *fiber;
	EPOLL      *epoll;

	epoll_proc *proc;
	long long   expire;

	struct epoll_event *events;
	int maxevents;
	int nready;
};
#endif

struct EVENT {
	RING events;
	int  timeout;
	int  fdcount;
	ssize_t  setsize;
	socket_t maxfd;

	long long stamp;  // the stamp of the current fiber scheduler
	unsigned flag;
#define EVENT_F_IOCP (1 << 0)
#define EVENT_IS_IOCP(x) ((x)->flag & EVENT_F_IOCP)

#ifdef HAS_POLL
	TIMER_CACHE *poll_list;
	RING   poll_ready;
#endif

#ifdef HAS_EPOLL
	TIMER_CACHE *epoll_list;
	RING   epoll_ready;
#endif

	unsigned waiter;

	const char *(*name)(void);
	acl_handle_t (*handle)(EVENT *);
	void (*free)(EVENT *);

	int  (*event_fflush)(EVENT *);
	int  (*event_wait)(EVENT *, int);

	event_oper *checkfd;
	event_oper *add_read;
	event_oper *add_write;
	event_oper *del_read;
	event_oper *del_write;
	event_oper *close_sock;
};

/* file_event.c */
void file_event_init(FILE_EVENT *fe, socket_t fd);
FILE_EVENT *file_event_alloc(socket_t fd);
void file_event_free(FILE_EVENT *fe);

/* event.c */
void event_set(int event_mode);
EVENT *event_create(int size);
const char *event_name(EVENT *ev);
acl_handle_t event_handle(EVENT *ev);
void event_free(EVENT *ev);
void event_close(EVENT *ev, FILE_EVENT *fe);
long long event_set_stamp(EVENT *ev);
long long event_get_stamp(EVENT *ev);

// check if the fd in fe is a valid socket, return 1 if yes, -1 if the fd
// is not a valid fd, 0 if the fd is valid but not a socket.
int  event_checkfd(EVENT *ev, FILE_EVENT *fe);

// event_add_read() and event_add_write() add the fd in fe to the IO event,
// return 1 if adding ok, return 0 if the fd is a valid fd but not a socket,
// return -1 if the fd is not a valid fd.
int  event_add_read(EVENT *ev, FILE_EVENT *fe, event_proc *proc);
int  event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc);

void event_del_read(EVENT *ev, FILE_EVENT *fe);
void event_del_write(EVENT *ev, FILE_EVENT *fe);
int  event_process(EVENT *ev, int left);

#endif
