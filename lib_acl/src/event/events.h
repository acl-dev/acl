#ifndef __EVENTS_INCLUDED_H__
#define __EVENTS_INCLUDED_H__

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include <time.h>
#include "stdlib/acl_ring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_fifo.h"
#include "thread/acl_thread.h"
#include "event/acl_events.h"

#include "events_dog.h"
#include "fdmap.h"
#include "events_define.h"
#include "events_epoll.h"
#include "events_devpoll.h"

/*
 * I/O events. We pre-allocate one data structure per file descriptor. XXX
 * For now use FD_SETSIZE as defined along with the fd-set type.
 */

#if defined(_WIN32) || defined(_WIN64)
typedef struct IOCP_EVENT IOCP_EVENT;
#endif

struct ACL_EVENT_FDTABLE {
	ACL_VSTREAM *stream;
	ACL_EVENT_NOTIFY_RDWR r_callback;
	ACL_EVENT_NOTIFY_RDWR w_callback;
	void *r_context;
	void *w_context;
	acl_int64 r_ttl;
	acl_int64 w_ttl;
	acl_int64 r_timeout;
	acl_int64 w_timeout;
	int   event_type;
	int   listener;			/* if the fd is a listening fd */
	int   flag;
#define EVENT_FDTABLE_FLAG_READ         (1 << 1)
#define EVENT_FDTABLE_FLAG_WRITE        (1 << 2)
#define EVENT_FDTABLE_FLAG_EXPT         (1 << 3)
#define EVENT_FDTABLE_FLAG_ADD_READ     (1 << 4)
#define EVENT_FDTABLE_FLAG_ADD_WRITE    (1 << 5)
#define EVENT_FDTABLE_FLAG_DEL_READ     (1 << 6)
#define EVENT_FDTABLE_FLAG_DEL_WRITE    (1 << 7)
#define EVENT_FDTABLE_FLAG_DELAY_OPER   (1 << 8)
#define EVENT_FDTABLE_FLAG_IOCP         (1 << 9)
#define	EVENT_FDTABLE_FLAG_FIRE		(1 << 10)

	int   fdidx;
	int   fdidx_ready;
	ACL_RING delay_entry;

#if defined(_WIN32) || defined(_WIN64)
	HANDLE h_iocp;
	IOCP_EVENT *event_read;
	IOCP_EVENT *event_write;
#endif
};

struct	ACL_EVENT {
	/* 事件引擎名称标识 */
	char  name[128];

	/**
	 * 事件引擎:
	 * ACL_EVENT_SELECT, ACL_EVENT_KERNEL,
	 * ACL_EVENT_POLL, ACL_EVENT_WMSG
	 */
	int   event_mode;
	/* 该事件引擎是否在多线程环境下使用 */
	int   use_thread;

	/* 避免事件引擎被嵌套调用 */
	int   nested;
	/* 当前时间截(微秒级) */
	acl_int64 present;
	acl_int64 last_check;
	acl_int64 check_inter;
	acl_int64 last_debug;
	/* 事件引擎的最大等待时间(秒) */
	int   delay_sec;
	/* 事件引擎的最大等待时间(微秒) */
	int   delay_usec;
	/* 本次循环前缓冲区有数据可读的描述字个数 */
	int   read_ready;
	/* 定时器任务列表头 */
	ACL_RING timer_head;

	/* 套接字最大个数 */
	int   fdsize;
	/* 当前套接字个数 */
	int   fdcnt;
	/* 事件循环时准备好的套接字个数 */
	int   ready_cnt;
	/* 套接字事件对象表集合 */
	ACL_EVENT_FDTABLE **fdtabs;
	/* 准备好的套接字事件对象表集合 */
	ACL_EVENT_FDTABLE **ready;
	/* 本进程中最大套接字值 */
	ACL_SOCKET   maxfd;

	/* 事件引擎的循环检查过程 */
	void (*loop_fn)(ACL_EVENT *ev);
	/* 有些事件引擎可能需要提前进行初始化 */
	void (*init_fn)(ACL_EVENT *ev);
	/* 释放事件引擎对象 */
	void (*free_fn)(ACL_EVENT *ev);
	/* 当在多线程下使用事件引擎时，需要此接口及时唤醒事件引擎 */
	void (*add_dog_fn)(ACL_EVENT *ev);

	/* 开始监控某个套接字的可读状态 */
	void (*enable_read_fn)(ACL_EVENT *, ACL_VSTREAM *, int,
		ACL_EVENT_NOTIFY_RDWR, void *);
	/* 开始监控某个套接字的可写状态 */
	void (*enable_write_fn)(ACL_EVENT *, ACL_VSTREAM *, int,
		ACL_EVENT_NOTIFY_RDWR, void *);
	/* 开始监控监听套接口的可读状态 */
	void (*enable_listen_fn)(ACL_EVENT *, ACL_VSTREAM *, int,
		ACL_EVENT_NOTIFY_RDWR, void *);

	/* 停止监控某个套接口的可读状态 */
	void (*disable_read_fn)(ACL_EVENT *, ACL_VSTREAM *);
	/* 停止监控某个套接口的可写状态 */
	void (*disable_write_fn)(ACL_EVENT *, ACL_VSTREAM *);
	/* 停止监控某个套接口的读、写状态 */
	void (*disable_readwrite_fn)(ACL_EVENT *, ACL_VSTREAM *);

	/* 套接口的可读状态是否被监控 */
	int  (*isrset_fn)(ACL_EVENT *, ACL_VSTREAM *);
	/* 套接口的可写状态是否被监控 */
	int  (*iswset_fn)(ACL_EVENT *, ACL_VSTREAM *);
	/* 套接口的异常状态是否被监控 */
	int  (*isxset_fn)(ACL_EVENT *, ACL_VSTREAM *);

	/* 添加定时器任务 */
	acl_int64 (*timer_request)(ACL_EVENT *, ACL_EVENT_NOTIFY_TIME,
		void *, acl_int64, int);
	/* 取消定时器任务 */
	acl_int64 (*timer_cancel)(ACL_EVENT *ev,
		ACL_EVENT_NOTIFY_TIME, void *);
	/* 设置定时器是否为循环执行 */
	void (*timer_keep)(ACL_EVENT *, ACL_EVENT_NOTIFY_TIME, void *, int);
	/* 定时器是否循环执行的 */
	int  (*timer_ifkeep)(ACL_EVENT *, ACL_EVENT_NOTIFY_TIME, void *);

	/* 每次调用事件回调函数前若该函数指针非空则先调用此函数 */
	void (*fire_begin)(ACL_EVENT *, void *);
	/* 每次调用事件回调函数后若该函数指针非空则调用此函数 */
	void (*fire_end)(ACL_EVENT *, void *);
	/* fire_begin/fire_finish 的第二个参数 */
	void *fire_ctx;
};

/* 如果采用自旋锁，必须保持加锁时间非常短 */
/*
#ifdef	ACL_HAS_SPINLOCK
#define	EVENT_USE_SPINLOCK
#endif
*/

typedef struct EVENT_THR {
	ACL_EVENT event;

	/* just for thread */
#ifdef	EVENT_USE_SPINLOCK
	pthread_spinlock_t tm_mutex;
	pthread_spinlock_t tb_mutex;
#else
	acl_pthread_mutex_t tm_mutex;
	acl_pthread_mutex_t tb_mutex;
#endif
	EVENT_DOG *evdog;
	int     blocked;
} EVENT_THR;

/* local macro */
#define	FD_ENTRY_CLR(_fdp_) do {  \
	_fdp_->stream = NULL;  \
	_fdp_->r_callback = NULL;  \
	_fdp_->w_callback = NULL;  \
	_fdp_->r_context = NULL;  \
	_fdp_->w_context = NULL;  \
	_fdp_->r_ttl = 0; \
	_fdp_->w_ttl = 0; \
	_fdp_->r_timeout = 0; \
	_fdp_->w_timeout = 0; \
	_fdp_->listener = 0; \
	_fdp_->event_type = 0; \
	_fdp_->flag = 0; \
	_fdp_->fdidx = -1; \
	_fdp_->fdidx_ready = -1; \
	_fdp_->fifo_info = NULL; \
} while (0);

/* in events.c */
void event_check_fds(ACL_EVENT *ev);
int  event_prepare(ACL_EVENT *ev);
void event_fire(ACL_EVENT *ev);
int  event_thr_prepare(ACL_EVENT *ev);
void event_thr_fire(ACL_EVENT *ev);

/* in events_alloc.c */
ACL_EVENT *event_alloc(size_t size);

/* in events_select.c */
ACL_EVENT *event_new_select(void);

/* in events_select_thr.c */
ACL_EVENT *event_new_select_thr(void);

/* in events_poll.c */
ACL_EVENT *event_new_poll(int fdsize);

/* in events_poll_thr.c */
ACL_EVENT *event_poll_alloc_thr(int fdsize);

/* in events_kernel.c */
ACL_EVENT *event_new_kernel(int fdsize);

/* in events_kernel_thr.c */
#ifdef	ACL_EVENTS_KERNEL_STYLE
ACL_EVENT *event_new_kernel_thr(int fdsize);
#endif

#if (ACL_EVENTS_KERNEL_STYLE == ACL_EVENTS_STYLE_EPOLL)
ACL_EVENT *event_epoll_alloc_thr(int fdsize);
#endif

struct ACL_EVENT_TIMER {
	acl_int64  when;                /* when event is wanted  */
	acl_int64  delay;               /* timer deley           */
	ACL_EVENT_NOTIFY_TIME callback; /* callback function     */
	int   event_type;
	void *context;                  /* callback context      */
	ACL_RING ring;                  /* linkage               */
	int   nrefer;                   /* refered's count       */
	int   ncount;                   /* timer callback count  */
	int   keep;                     /* if timer call restart */
};

#define ACL_RING_TO_TIMER(r) \
	((ACL_EVENT_TIMER *) ((char *) (r) - offsetof(ACL_EVENT_TIMER, ring)))

#define ACL_FIRST_TIMER(head) \
	(acl_ring_succ(head) != (head) ? ACL_RING_TO_TIMER(acl_ring_succ(head)) : 0)

#ifdef	EVENT_USE_SPINLOCK

# define LOCK_INIT(mutex_in) do { \
	pthread_spinlock_t *sp = mutex_in; \
	int   status = pthread_spin_init(sp, PTHREAD_PROCESS_PRIVATE); \
	if (status != 0) \
		acl_msg_fatal("%s(%d): init lock error(%s)", \
			__FILE__, __LINE__, strerror(status)); \
} while (0)

# define LOCK_DESTROY(mutex_in) do { \
	pthread_spinlock_t *sp = mutex_in; \
	pthread_spin_destroy(sp); \
} while (0)

# define THREAD_LOCK(mutex_in) do { \
	pthread_spinlock_t *sp = mutex_in; \
	int   status = pthread_spin_lock(sp); \
	if (status != 0) \
		acl_msg_fatal("%s(%d): lock error(%s)", \
			__FILE__, __LINE__, strerror(status)); \
} while (0)

# define THREAD_UNLOCK(mutex_in) do { \
	pthread_spinlock_t *sp = mutex_in; \
	int   status = pthread_spin_unlock(sp); \
	if (status != 0) \
		acl_msg_fatal("%s(%d): unlock error(%s)", \
			__FILE__, __LINE__, strerror(status)); \
} while (0)

#else

# define LOCK_INIT(mutex_in) do { \
	acl_pthread_mutex_t *mutex_ptr = (mutex_in); \
	int   status = acl_pthread_mutex_init(mutex_ptr, NULL); \
	if (status != 0) \
		acl_msg_fatal("%s(%d): init lock error(%s)", \
			__FILE__, __LINE__, strerror(status)); \
} while (0)

# define LOCK_DESTROY(mutex_in) do { \
	acl_pthread_mutex_t *mutex_ptr = (mutex_in); \
	(void) acl_pthread_mutex_destroy(mutex_ptr); \
} while (0)

# define THREAD_LOCK(mutex_in) do { \
	acl_pthread_mutex_t *mutex_ptr = (mutex_in); \
	int   status = acl_pthread_mutex_lock(mutex_ptr); \
	if (status != 0) \
		acl_msg_fatal("%s(%d): lock error(%s)", \
			__FILE__, __LINE__, strerror(status)); \
} while (0)

# define THREAD_UNLOCK(mutex_in) do { \
	acl_pthread_mutex_t *mutex_ptr = (mutex_in); \
	int   status = acl_pthread_mutex_unlock(mutex_ptr); \
	if (status != 0) \
		acl_msg_fatal("%s(%d): unlock error(%s)", \
			__FILE__, __LINE__, strerror(status)); \
} while (0)

#endif

#define SET_TIME(x) {  \
	struct timeval _tv;  \
	gettimeofday(&_tv, NULL);  \
	(x) = ((acl_int64) _tv.tv_sec) * 1000000 + ((acl_int64) _tv.tv_usec);  \
}

/* in events_timer.c */
acl_int64 event_timer_request(ACL_EVENT *ev, ACL_EVENT_NOTIFY_TIME callback,
	void *context, acl_int64 delay, int keep);
acl_int64 event_timer_cancel(ACL_EVENT *ev, ACL_EVENT_NOTIFY_TIME callback,
	void *context);
void event_timer_keep(ACL_EVENT *ev, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int keep);
int  event_timer_ifkeep(ACL_EVENT *ev, ACL_EVENT_NOTIFY_TIME callback,
	void *context);

/* in events_timer_thr.c */
acl_int64 event_timer_request_thr(ACL_EVENT *ev,
	ACL_EVENT_NOTIFY_TIME callback, void *context,
	acl_int64 delay, int keep);
acl_int64 event_timer_cancel_thr(ACL_EVENT *ev,
	ACL_EVENT_NOTIFY_TIME callback, void *context);

void event_timer_keep_thr(ACL_EVENT *ev, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int keep);
int  event_timer_ifkeep_thr(ACL_EVENT *ev, ACL_EVENT_NOTIFY_TIME callback,
	void *context);

#ifdef	__cplusplus
}
#endif

#endif
