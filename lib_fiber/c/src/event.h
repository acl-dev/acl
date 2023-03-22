#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include "define.h"
#include "common/gettimeofday.h"
#include "common/timer_cache.h"

#ifdef	HAS_EPOLL
#include <sys/epoll.h>
#endif

#ifdef	HAS_IO_URING
#include <sys/types.h>
#include <sys/socket.h>
#include <liburing.h>
#endif

#include "fiber/libfiber.h"

#if defined(USE_CLOCK_GETTIME)
# ifdef CLOCK_REALTIME_COARSE
#  define CLOCK_ID CLOCK_REALTIME_COARSE
# elif defined(CLOCK_REALTIME)
#  define CLOCK_ID CLOCK_REALTIME
# else
#  undef USE_CLOCK_GETTIME
# endif
#endif

#if defined(USE_FAST_TIME)
# define SET_TIME(x) do { \
    struct timeval _tv; \
    acl_fiber_gettimeofday(&_tv, NULL); \
    (x) = ((long long) _tv.tv_sec) * 1000 + ((long long) _tv.tv_usec)/ 1000; \
} while (0)
#elif defined(USE_CLOCK_GETTIME)
# define SET_TIME(x) do { \
    struct timespec _ts; \
    if (clock_gettime(CLOCK_ID, &_ts) == 0) { \
        (x) = ((long long) _ts.tv_sec) * 1000 + ((long long) _ts.tv_nsec) / 1000000; \
    } else { \
        abort(); \
    } \
} while (0)
#else
# define SET_TIME(x) do { \
    struct timeval _tv; \
    if (gettimeofday(&_tv, NULL) == 0) { \
        (x) = ((long long) _tv.tv_sec) * 1000 + ((long long) _tv.tv_usec)/ 1000; \
    } else { \
        abort(); \
    } \
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

#ifdef HAS_IO_URING
typedef struct IO_URING_CTX {
	FILE_EVENT *fe;
	int res;
	//int cnt;
	unsigned mask;
} IO_URING_CTX;
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
#define	STATUS_NONE		(unsigned) (0)
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
#define	TYPE_NONE		(unsigned) (0)
#define	TYPE_SPIPE		(unsigned) (1 << 0)
#define	TYPE_FILE		(unsigned) (1 << 1)
#define	TYPE_BADFD		(unsigned) (1 << 2)
#define	TYPE_EVENTABLE		(unsigned) (1 << 3)
#define	TYPE_INTERNAL		(unsigned) (1 << 4)

	unsigned oper;
#define	EVENT_ADD_READ		(unsigned) (1 << 0)
#define	EVENT_DEL_READ		(unsigned) (1 << 1)
#define	EVENT_ADD_WRITE		(unsigned) (1 << 2)
#define	EVENT_DEL_WRITE		(unsigned) (1 << 3)

	unsigned mask;
#define	EVENT_NONE		(unsigned) (0)
#define	EVENT_DIRECT		(unsigned) (1 << 0)
#define	EVENT_SYSIO		(unsigned) (1 << 1)
#define	EVENT_READ		(unsigned) (1 << 2)
#define	EVENT_WRITE		(unsigned) (1 << 3)
#define	EVENT_ERR		(unsigned) (1 << 4)
#define	EVENT_HUP		(unsigned) (1 << 5)
#define	EVENT_NVAL		(unsigned) (1 << 6)

#ifdef	HAS_IO_URING

#define	EVENT_ACCEPT		(unsigned) (1 << 7)
#define	EVENT_CONNECT		(unsigned) (1 << 8)
#define	EVENT_POLLIN		(unsigned) (1 << 9)
#define	EVENT_POLLOUT		(unsigned) (1 << 10)
#define	EVENT_FILE_OPENAT	(unsigned) (1 << 11)
#define	EVENT_FILE_CLOSE	(unsigned) (1 << 12)
#define	EVENT_FILE_CANCEL	(unsigned) (1 << 13)
#define	EVENT_FILE_UNLINK	(unsigned) (1 << 14)
#define	EVENT_FILE_STAT		(unsigned) (1 << 15)
#define	EVENT_FILE_STATX	(unsigned) (1 << 16)
#define	EVENT_FILE_RENAMEAT	(unsigned) (1 << 17)
#define	EVENT_FILE_RENAMEAT2	(unsigned) (1 << 18)
#define	EVENT_DIR_MKDIRAT	(unsigned) (1 << 19)
#define	EVENT_SPLICE		(unsigned) (1 << 20)

#define	EVENT_READV		(unsigned) (1 << 21)
#define	EVENT_RECV		(unsigned) (1 << 22)
#define	EVENT_RECVFROM		(unsigned) (1 << 23)
#define	EVENT_RECVMSG		(unsigned) (1 << 24)

#define	EVENT_WRITEV		(unsigned) (1 << 25)
#define	EVENT_SEND		(unsigned) (1 << 26)
#define	EVENT_SENDTO		(unsigned) (1 << 27)
#define	EVENT_SENDMSG		(unsigned) (1 << 28)
#endif // HAS_IO_URING

	event_proc   *r_proc;
	event_proc   *w_proc;
#ifdef HAS_POLL
	POLLFD       *pfd;
#endif
#ifdef HAS_EPOLL
	EPOLL_CTX    *epx;
#endif

#ifdef HAS_IO_URING
	// Because in the sharing stack mode, the below objects maybe used
	// by the io_uring kernel thread, we must add them here.
	union {
		struct {
			char *buf;
			unsigned len;
			__u64 off;
		} read_ctx;

		struct {
			const struct iovec *iov;
			int cnt;
			__u64 off;
		} readv_ctx;

		struct {
			char *buf;
			unsigned len;
			int flags;
		} recv_ctx;

#if defined(IO_URING_HAS_RECVFROM)
		struct {
			char *buf;
			unsigned len;
			int flags;
			struct sockaddr *src_addr;
			socklen_t *addrlen;
		} recvfrom_ctx;
#endif

		struct {
			struct msghdr *msg;
			int flags;
		} recvmsg_ctx;
	} in;

	union {
		struct {
			const void *buf;
			unsigned len;
			__u64 off;
		} write_ctx;

		struct {
			const struct iovec *iov;
			int cnt;
			__u64 off;
		} writev_ctx;

		struct {
			const void *buf;
			unsigned len;
			int flags;
		} send_ctx;

#if defined(IO_URING_HAS_SENDTO)
		struct {
			const void *buf;
			unsigned len;
			int flags;
			const struct sockaddr *dest_addr;
			socklen_t addrlen;
		} sendto_ctx;
#endif

		struct {
			const struct msghdr *msg;
			int flags;
		} sendmsg_ctx;
	} out;

	union {
		struct {
			struct sockaddr_in addr;
			socklen_t          len;
		} peer;

#ifdef HAS_STATX
		struct statx *statxbuf;
#endif
		char  *path;
	} var;

	struct IO_URING_CTX reader_ctx;
	struct IO_URING_CTX writer_ctx;
	struct __kernel_timespec rts;
	struct __kernel_timespec wts;
	int           r_timeout;
	int           w_timeout;

#endif

	ACL_FIBER_SEM* mbox_wsem; // Used in sync_waiter_wakeup.c

#ifdef HAS_IOCP
	char          packet[512];  // Just for UDP packet
	char         *rbuf;
	int           rsize;
	int           res;
	HANDLE        h_iocp;
	IOCP_EVENT   *reader;
	IOCP_EVENT   *writer;
	IOCP_EVENT   *poller_read;
	IOCP_EVENT   *poller_write;
	socket_t      iocp_sock;
	int           sock_type;
	union {
		struct {
			struct sockaddr_in addr;
			socklen_t          len;
		} peer;
	} var;
#endif
	short refer;
	short busy;
#define	EVENT_BUSY_NONE		(0)
#define	EVENT_BUSY_READ		(1 << 0)
#define	EVENT_BUSY_WRITE	(1 << 1)
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
#define EVENT_F_IOCP		(1 << 0)
#define	EVENT_F_IO_URING	(1 << 1)
#define EVENT_IS_IOCP(x)	((x)->flag & EVENT_F_IOCP)
#define	EVENT_IS_IO_URING(x)	((x)->flag & EVENT_F_IO_URING)

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

#if 1
#define	WAITER_INC(e) ((e)->waiter++)
#define	WAITER_DEC(e) ((e)->waiter--)
#else
#define	WAITER_INC(e) { printf("%s(%d): thread-%lu, ++waiter=%d\r\n", __FUNCTION__, __LINE__, (unsigned long) pthread_self(), ++(e)->waiter); }
#define	WAITER_DEC(e) { printf("%s(%d): thread-%lu, --waiter=%d\r\n", __FUNCTION__, __LINE__, (unsigned long) pthread_self(), --(e)->waiter); }
#endif

/* file_event.c */
void file_event_init(FILE_EVENT *fe, socket_t fd);
FILE_EVENT *file_event_alloc(socket_t fd);
int file_event_refer(FILE_EVENT *fe);
int file_event_unrefer(FILE_EVENT *fe);

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
