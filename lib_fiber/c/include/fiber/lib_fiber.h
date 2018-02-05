#ifndef LIB_FIBER_INCLUDE_H
#define LIB_FIBER_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined (_WIN64)
typedef long ssize_t;

# define	FIBER_ETIMEDOUT		WSAETIMEDOUT
# define	FIBER_ENOMEM		WSAENOBUFS
# define	FIBER_EINVAL		WSAEINVAL
# define	FIBER_ECONNREFUSED	WSAECONNREFUSED
# define	FIBER_ECONNRESET	WSAECONNRESET
# define	FIBER_EHOSTDOWN		WSAEHOSTDOWN
# define	FIBER_EHOSTUNREACH	WSAEHOSTUNREACH
# define	FIBER_EINTR		WSAEINTR
# define	FIBER_ENETDOWN		WSAENETDOWN
# define	FIBER_ENETUNREACH	WSAENETUNREACH
# define	FIBER_ENOTCONN		WSAENOTCONN
# define	FIBER_EISCONN		WSAEISCONN
# define	FIBER_EWOULDBLOCK	WSAEWOULDBLOCK
# define	FIBER_EAGAIN		FIBER_EWOULDBLOCK	/* xxx */
# define	FIBER_ENOBUFS		WSAENOBUFS
# define	FIBER_ECONNABORTED	WSAECONNABORTED
# define	FIBER_EINPROGRESS	WSAEINPROGRESS

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <poll.h>
#include <sys/select.h>

# define	FIBER_ETIMEDOUT		ETIMEDOUT
# define	FIBER_ENOMEM		ENOMEM
# define	FIBER_EINVAL		EINVAL
# define	FIBER_ECONNREFUSED	ECONNREFUSED
# define	FIBER_ECONNRESET	ECONNRESET
# define	FIBER_EHOSTDOWN		EHOSTDOWN
# define	FIBER_EHOSTUNREACH	EHOSTUNREACH
# define	FIBER_EINTR		EINTR
# define	FIBER_EAGAIN		EAGAIN
# define	FIBER_ENETDOWN		ENETDOWN
# define	FIBER_ENETUNREACH	ENETUNREACH
# define	FIBER_ENOTCONN		ENOTCONN
# define	FIBER_EISCONN		EISCONN
# define	FIBER_EWOULDBLOCK	EWOULDBLOCK
# define	FIBER_ENOBUFS		ENOBUFS
# define	FIBER_ECONNABORTED	ECONNABORTED
# define	FIBER_EINPROGRESS	EINPROGRESS

#endif

#ifdef FIBER_LIB
# ifndef FIBER_API
#  define FIBER_API
# endif
#elif defined(FIBER_DLL) || defined(_WINDLL)
# if defined(FIBER_EXPORTS) || defined(fiber_EXPORTS)
#  ifndef FIBER_API
#   define FIBER_API __declspec(dllexport)
#  endif
# elif !defined(FIBER_API)
#  define FIBER_API __declspec(dllimport)
# endif
#elif !defined(FIBER_API)
# define FIBER_API
#endif

/**
 * 协程结构类型
 */
typedef struct ACL_FIBER ACL_FIBER;

/**
 * 设置是否需要 hook 系统中的 IO 相关的 API，内部缺省值为 1
 * @param onoff {int} 是否需要 hook
 */
FIBER_API void acl_fiber_hook_api(int onoff);

/**
 * 创建一个协程
 * @param fn {void (*)(ACL_FIBER*, void*)} 协程运行时的回调函数地址
 * @param arg {void*} 回调 fn 函数时的第二个参数
 * @param size {size_t} 所创建协程所占栈空间大小
 * @return {ACL_FIBER*}
 */
FIBER_API ACL_FIBER* acl_fiber_create(void (*fn)(ACL_FIBER*, void*),
	void* arg, size_t size);

/**
 * 返回当前线程中处于消亡状态的协程数
 * @retur {int}
 */
FIBER_API int acl_fiber_ndead(void);

FIBER_API void acl_fiber_check_timer(size_t max);

/**
 * 返回当前正在运行的协程对象
 * @retur {ACL_FIBER*} 返回 NULL 表示当前没有正在运行的协程
 */
FIBER_API ACL_FIBER* acl_fiber_running(void);

/**
 * 获得所给协程的协程 ID 号
 * @param fiber {const ACL_FIBER*} acl_fiber_create 创建的协程对象，必须非空
 * @return {unsigned int} 协程 ID 号
 */
FIBER_API unsigned int acl_fiber_id(const ACL_FIBER* fiber);

/**
 * 获得当前所运行的协程的 ID 号
 * @return {unsigned int} 当前运行协程的 ID 号
 */
FIBER_API unsigned int acl_fiber_self(void);

/**
 * 设置所给协程的错误号
 * @param fiber {ACL_FIBER*} 指定的协程对象，为 NULL 则使用当前运行的协程
 * @param errnum {int} 错误号
 */
FIBER_API void acl_fiber_set_errno(ACL_FIBER* fiber, int errnum);

/**
 * 获得指定协程的错误号
 * @param fiber {ACL_FIBER*} 指定的协程对象，若为 NULL 则使用当前协程对象
 * @return {int} 所给协程错误号
 */
FIBER_API int acl_fiber_errno(ACL_FIBER* fiber);

/**
 * 获得当前系统级的 errno 号
 * @return {int}
 */
FIBER_API int acl_fiber_sys_errno(void);

/**
 * 设置当前系统的 errno 号
 * @param errnum {int}
 */
FIBER_API void acl_fiber_sys_errno_set(int errnum);

/**
 * 是否保持所指定协程的错误号，当设置为“保持”后，则该协程仅保持当前状态下的
 * 错误号，之后该协程的错误号 errno 将不再改变，走到再次调用本函数取消保持
 * @param fiber {ACL_FIBER*} 指定的协程对象，为 NULL 则使用当前运行的协程
 * @param yesno {int} 是否保持
 */
FIBER_API void acl_fiber_keep_errno(ACL_FIBER* fiber, int yesno);

/**
 * 获得指定协程的当前状态
 * @param fiber {const ACL_FIBER*} 指定的协程对象，为 NULL 则使用当前协程
 * @return {int} 协程状态
 */
FIBER_API int acl_fiber_status(const ACL_FIBER* fiber);

/**
 * 通知处于休眠状态的协程退出 
 * @param fiber {const ACL_FIBER*} 指定的协程对象，必须非 NULL
 */
FIBER_API void acl_fiber_kill(ACL_FIBER* fiber);

/**
 * 检查本协程是否被其它协程通知退出
 * @param fiber {const ACL_FIBER*} 指定的协程对象，若为 NULL 则自动使用当前
 *  正在运行的协程
 * @return {int} 返回值为 0 表示没有被通知退出，非 0 表示被通知退出
 */
FIBER_API int acl_fiber_killed(ACL_FIBER* fiber);

/**
 * 唤醒因 IO 等原因处于休眠的协程
 * @param fiber {const ACL_FIBER*} 协程对象，必须非 NULL
 * @param signum {int} SIGINT, SIGKILL, SIGTERM ... 参考系统中 bits/signum.h
 */
FIBER_API void acl_fiber_signal(ACL_FIBER* fiber, int signum);

/**
 * 获得其它协程发送给指定协程的信号值
 * @param fiber {const ACL_FIBER*} 指定的协程对象，为 NULL 时则使用当前协程
 * @retur {int} 返回指定协程收到的信号值
 */
FIBER_API int acl_fiber_signum(ACL_FIBER* fiber);

/**
 * 将当前运行的协程挂起，由调度器选择下一个需要运行的协程
 * @return {int}
 */
FIBER_API int acl_fiber_yield(void);

/**
 * 将指定协程对象置入待运行队列中
 * @param fiber {ACL_FIBER*} 指定协程，必须非 NULL
 */
FIBER_API void acl_fiber_ready(ACL_FIBER* fiber);

/**
 * 将当前运行的协程挂起，同时执行等待队列下一个待运行的协程
 */
FIBER_API void acl_fiber_switch(void);

/**
 * 设置当前线程的协程调度器是否为自启动模式，自启动模式即当创建协程后会检查是否处于协程
 * 调度状态，如果非协程调度状态，则自动启动调度器调度协程过程（如不调用本函数，则内部缺省
 * 为非自动启动模式）
 */
FIBER_API void acl_fiber_schedule_init(int on);

/**
 * 调用本函数启动协程的调度过程
 */
FIBER_API void acl_fiber_schedule(void);

/**
 * 采用指定的事件引擎启动协程调度器，内部缺省的事件引擎为 FIBER_EVENT_KERNEL
 * @param event_mode {int} 事件引擎类型，参见：FIBER_EVENT_XXX
 */
#define FIBER_EVENT_KERNEL	0	/* epoll/kqueue	*/
#define FIBER_EVENT_POLL	1	/* poll		*/
#define FIBER_EVENT_SELECT	2	/* select	*/
#define FIBER_EVENT_WMSG	3	/* win message	*/
FIBER_API void acl_fiber_schedule_with(int event_mode);

/**
 * 设置协程调度时的事件引擎类型
 * @param event_mode {int} 事件引擎类型，参见：FIBER_EVENT_XXX
 */
FIBER_API void acl_fiber_schedule_set_event(int event_mode);

/**
 * 调用本函数检测当前线程是否处于协程调度状态
 * @return {int} 0 表示非协程状态，非 0 表示处于协程调度状态
 */
FIBER_API int acl_fiber_scheduled(void);

/**
 * 停止协程过程
 */
FIBER_API void acl_fiber_schedule_stop(void);

/**
 * 使当前运行的协程休眠指定毫秒数
 * @param milliseconds {unsigned int} 指定要休眠的毫秒数
 * @return {unsigned int} 本协程休眠后再次被唤醒后剩余的毫秒数
 */
FIBER_API unsigned int acl_fiber_delay(unsigned int milliseconds);

/**
 * 使当前运行的协程休眠指定秒数
 * @param seconds {unsigned int} 指定要休眠的秒数
 * @return {unsigned int} 本协程休眠后再次被唤醒后剩余的秒数
 */
FIBER_API unsigned int acl_fiber_sleep(unsigned int seconds);

/**
 * 创建一个协程用作定时器
 * @param milliseconds {unsigned int} 所创建定时器被唤醒的毫秒数
 * @param size {size_t} 所创建协程的栈空间大小
 * @param fn {void (*)(ACL_FIBER*, void*)} 定时器协程被唤醒时的回调函数
 * @param ctx {void*} 回调 fn 函数时的第二个参数
 * @return {ACL_FIBER*} 新创建的定时器协程
 */
FIBER_API ACL_FIBER* acl_fiber_create_timer(unsigned int milliseconds,
	size_t size, void (*fn)(ACL_FIBER*, void*), void* ctx);

/**
 * 在定时器协程未被唤醒前，可以通过本函数重置该协程被唤醒的时间
 * @param timer {ACL_FIBER*} 由 acl_fiber_create_timer 创建的定时器协程
 * @param milliseconds {unsigned int} 指定该定时器协程被唤醒的毫秒数
 */
FIBER_API void acl_fiber_reset_timer(ACL_FIBER* timer, unsigned int milliseconds);

/**
 * 本函数设置 DNS 服务器的地址
 * @param ip {const char*} DNS 服务器 IP 地址
 * @param port {int} DNS 服务器的端口
 */
FIBER_API void acl_fiber_set_dns(const char* ip, int port);

/* for fiber specific */

/**
 * 设定当前协程的局部变量
 * @param key {int*} 协程局部变量的索引键的地址，初始时该值应 <= 0，内部会自动
 *  分配一个 > 0 的索引键，并给该地址赋值，后面的协程可以复用该值设置各自的
 *  局部变量，该指针必须非 NULL
 * @param ctx {void *} 协程局部变量
 * @param free_fn {void (*)(void*)} 当协程退出时会调用此函数释放协程局部变量
 * @return {int} 返回所设置的协程局部变量的键值，返回 -1 表示当前协程不存在
 */
FIBER_API int acl_fiber_set_specific(int* key, void* ctx, void (*free_fn)(void*));

/**
 * 获得当前协程局部变量
 * @param key {int} 由 acl_fiber_set_specific 返回的键值
 * @retur {void*} 返回 NULL 表示不存在
 */
FIBER_API void* acl_fiber_get_specific(int key);

/* fiber locking */

/**
 * 协程互斥锁，线程非安全，只能用在同一线程内
 */
typedef struct ACL_FIBER_MUTEX ACL_FIBER_MUTEX;

/**
 * 协程读写锁，线程非安全，只能用在同一线程内
 */
typedef struct ACL_FIBER_RWLOCK ACL_FIBER_RWLOCK;

/**
 * 创建协程互斥锁，线程非安全，只能用在同一线程内
 * @return {ACL_FIBER_MUTEX*}
 */
FIBER_API ACL_FIBER_MUTEX* acl_fiber_mutex_create(void);

/**
 * 释放协程互斥锁
 * @param l {ACL_FIBER_MUTEX*} 由 acl_fiber_mutex_create 创建的协程互斥锁
 */
FIBER_API void acl_fiber_mutex_free(ACL_FIBER_MUTEX* l);

/**
 * 对协程互斥锁进行阻塞式加锁，如果加锁成功则返回，否则则阻塞
 * @param l {ACL_FIBER_MUTEX*} 由 acl_fiber_mutex_create 创建的协程互斥锁
 */
FIBER_API void acl_fiber_mutex_lock(ACL_FIBER_MUTEX* l);

/**
 * 对协程互斥锁尝试性进行加锁，无论是否成功加锁都会立即返回
 * @param l {ACL_FIBER_MUTEX*} 由 acl_fiber_mutex_create 创建的协程互斥锁
 * @return {int} 如果加锁成功则返回 0 值，否则返回 -1
 */
FIBER_API int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX* l);

/**
 * 加锁成功的协程调用本函数进行解锁，调用本函数的协程必须是该锁的属主，否则
 * 内部会产生断言
 * @param l {ACL_FIBER_MUTEX*} 由 acl_fiber_mutex_create 创建的协程互斥锁
 */
FIBER_API void acl_fiber_mutex_unlock(ACL_FIBER_MUTEX* l);

/**
 * 创建协程读写锁，线程非安全，只能用在同一线程内
 * @return {ACL_FIBER_RWLOCK*}
 */
FIBER_API ACL_FIBER_RWLOCK* acl_fiber_rwlock_create(void);

/**
 * 释放协程读写锁
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 */
FIBER_API void acl_fiber_rwlock_free(ACL_FIBER_RWLOCK* l);

/**
 * 对协程读写锁加读锁，如果该锁当前正被其它协程加了读锁，则本协程依然可以
 * 正常加读锁，如果该锁当前正被其它协程加了写锁，则本协程进入阻塞状态，直至
 * 写锁释放
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 */
FIBER_API void acl_fiber_rwlock_rlock(ACL_FIBER_RWLOCK* l);

/**
 * 对协程读写锁尝试性加读锁，加锁无论是否成功都会立即返回
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 * @retur {int} 返回 1 表示加锁成功，返回 0 表示加锁失败
 */
FIBER_API int acl_fiber_rwlock_tryrlock(ACL_FIBER_RWLOCK* l);

/**
 * 对协程读写锁加写锁，只有当该锁未被任何协程加读/写锁时才会返回，否则阻塞，
 * 直至该锁可加写锁
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 */
FIBER_API void acl_fiber_rwlock_wlock(ACL_FIBER_RWLOCK* l);

/**
 * 对协程读写锁尝试性加写锁，无论是否加锁成功都会立即返回
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 * @return {int} 返回 1 表示加写锁成功，返回 0 表示加锁失败
 */
FIBER_API int acl_fiber_rwlock_trywlock(ACL_FIBER_RWLOCK* l);

/**
 * 对协程读写锁成功加读锁的协程调用本函数解读锁，调用者必须是之前已成功加读
 * 锁成功的协程
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 */
FIBER_API void acl_fiber_rwlock_runlock(ACL_FIBER_RWLOCK* l);
/**
 * 对协程读写锁成功加写锁的协程调用本函数解写锁，调用者必须是之前已成功加写
 * 锁成功的协程
 * @param l {ACL_FIBER_RWLOCK*} 由 acl_fiber_rwlock_create 创建的读写锁
 */
FIBER_API void acl_fiber_rwlock_wunlock(ACL_FIBER_RWLOCK* l);

/* fiber_event.c */

/* 线程安全的协程锁，可以用在不同线程的协程之间及不同线程之间的互斥 */
typedef struct ACL_FIBER_EVENT ACL_FIBER_EVENT;

/**
 * 创建基于事件的协程/线程混合锁
 * @return {ACL_FIBER_EVENT *}
 */
FIBER_API ACL_FIBER_EVENT *acl_fiber_event_create(void);

/**
 * 释放事件锁
 * @param {ACL_FIBER_EVENT *}
 */
FIBER_API void acl_fiber_event_free(ACL_FIBER_EVENT *event);

/**
 * 等待事件锁可用
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 返回 0 表示成功，-1 表示出错
 */
FIBER_API int acl_fiber_event_wait(ACL_FIBER_EVENT *event);

/**
 * 尝试等待事件锁可用
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 返回 0 表示成功，-1 表示锁被占用
 */
FIBER_API int acl_fiber_event_trywait(ACL_FIBER_EVENT *event);

/**
 * 事件锁拥有者通知等待者事件锁可用，则等待者收到通知后则可获得事件锁
 * @param {ACL_FIBER_EVENT *}
 * @return {int} 返回 0 表示成功，-1 表示出错
 */
FIBER_API int acl_fiber_event_notify(ACL_FIBER_EVENT *event);

/* fiber semaphore */

typedef struct ACL_FIBER_SEM ACL_FIBER_SEM;

/**
 * 创建协程信号量，同时内部会将当前线程与该信号量绑定
 * @param num {int} 信号量初始值（必须 >= 0）
 * @return {ACL_FIBER_SEM *}
 */
FIBER_API ACL_FIBER_SEM* acl_fiber_sem_create(int num);

/**
 * 释放协程信号量
 * @param {ACL_FIBER_SEM *}
 */
FIBER_API void acl_fiber_sem_free(ACL_FIBER_SEM* sem);

/**
 * 获得当前协程信号量所绑定的线程 ID
 * @param sem {ACL_FIBER_SEM*} 协程信号量对象
 * @return {unsigned long}
 */
#if !defined(_WIN32) && !defined(_WIN64)
FIBER_API unsigned long acl_fiber_sem_get_tid(ACL_FIBER_SEM* sem);
#endif

/**
 * 设置指定协程信号量的的线程 ID，当改变本协程信号量所属的线程时如果等待的协程
 * 数据非 0 则内部自动 fatal，即当协程信号量上等待协程非空时禁止调用本方法
 * @param sem {ACL_FIBER_SEM*} 协程信号量对象
 * @param {unsigned long} 线程 ID
 */
FIBER_API void acl_fiber_sem_set_tid(ACL_FIBER_SEM* sem, unsigned long tid);

/**
 * 当协程信号量 > 0 时使信号量减 1，否则等待信号量 > 0
 * @param sem {ACL_FIBER_SEM *}
 * @retur {int} 返回信号量当前值，如果返回 -1 表明当前线程与协程信号量所属线程
 *  不是同一线程，此时该方法不等待立即返回
 */
FIBER_API int acl_fiber_sem_wait(ACL_FIBER_SEM* sem);

/**
 * 尝试使协程信号量减 1
 * @param sem {ACL_FIBER_SEM *}
 * @retur {int} 成功减 1 时返回值 >= 0，返回 -1 表示当前信号量不可用，或当前
 *  调用者线程与协程信号量所属线程不是同一线程
 */
FIBER_API int acl_fiber_sem_trywait(ACL_FIBER_SEM* sem);

/**
 * 使协程信号量加 1
 * @param sem {ACL_FIBER_SEM *}
 * @retur {int} 返回信号量当前值，返回 -1 表示当前调用者线程与协程信号量所属
 *  线程不是同一线程
 */
FIBER_API int acl_fiber_sem_post(ACL_FIBER_SEM* sem);

/**
 * 获得指定协程信号量的当前值，该值反映了目前等待该信号量的数量
 * @param sem {ACL_FIBER_SEM*}
 * @retur {int}
 */
FIBER_API int acl_fiber_sem_num(ACL_FIBER_SEM* sem);

/* channel communication */

/**
 * 协程间通信的管道
 */
typedef struct ACL_CHANNEL ACL_CHANNEL;

/**
 * 创建协程通信管道
 * @param elemsize {int} 在 ACL_CHANNEL 进行传输的对象的固定尺寸大小（字节）
 * @param bufsize {int} ACL_CHANNEL 内部缓冲区大小，即可以缓存 elemsize 尺寸大小
 *  对象的个数
 * @return {CHANNNEL*}
 */
FIBER_API ACL_CHANNEL* acl_channel_create(int elemsize, int bufsize);

/**
 * 释放由 acl_channel_create 创建的协程通信管道对象
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 */
FIBER_API void acl_channel_free(ACL_CHANNEL* c);

/**
 * 阻塞式向指定 ACL_CHANNEL 中发送指定的对象地址
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param v {void*} 被发送的对象地址
 * @return {int} 返回值 >= 0
 */
FIBER_API int acl_channel_send(ACL_CHANNEL* c, void* v);

/**
 * 非阻塞式向指定 ACL_CHANNEL 中发送指定的对象，内部会根据 acl_channel_create 中指定
 * 的 elemsize 对象大小进行数据拷贝
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param v {void*} 被发送的对象地址
 */
FIBER_API int acl_channel_send_nb(ACL_CHANNEL* c, void* v);

/**
 * 从指定的 ACL_CHANNEL 中阻塞式读取对象，
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param v {void*} 存放结果内容
 * @return {int} 返回值 >= 0 表示成功读到数据
 */
FIBER_API int acl_channel_recv(ACL_CHANNEL* c, void* v);

/**
 * 从指定的 ACL_CHANNEL 中非阻塞式读取对象，无论是否读到数据都会立即返回
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param v {void*} 存放结果内容
 * @return {int} 返回值 >= 0 表示成功读到数据，否则表示未读到数据
 */
FIBER_API int acl_channel_recv_nb(ACL_CHANNEL* c, void* v);

/**
 * 向指定的 ACL_CHANNEL 中阻塞式发送指定对象的地址
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param v {void*} 被发送对象的地址
 * @return {int} 返回值 >= 0
 */
FIBER_API int acl_channel_sendp(ACL_CHANNEL* c, void* v);

/**
 * 从指定的 CHANNLE 中阻塞式接收由 acl_channel_sendp 发送的对象的地址
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @return {void*} 返回非 NULL，指定接收到的对象的地址
 */
FIBER_API void *acl_channel_recvp(ACL_CHANNEL* c);

/**
 * 向指定的 ACL_CHANNEL 中非阻塞式发送指定对象的地址
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param v {void*} 被发送对象的地址
 * @return {int} 返回值 >= 0
 */
FIBER_API int acl_channel_sendp_nb(ACL_CHANNEL* c, void* v);

/**
 * 从指定的 CHANNLE 中阻塞式接收由 acl_channel_sendp 发送的对象的地址
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @return {void*} 返回非 NULL，指定接收到的对象的地址，如果返回 NULL 表示
 *  没有读到任何对象
 */
FIBER_API void *acl_channel_recvp_nb(ACL_CHANNEL* c);

/**
 * 向指定的 ACL_CHANNEL 中发送无符号长整形数值
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param val {unsigned long} 要发送的数值
 * @return {int} 返回值 >= 0
 */
FIBER_API int acl_channel_sendul(ACL_CHANNEL* c, unsigned long val);

/**
 * 从指定的 ACL_CHANNEL 中接收无符号长整形数值
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @return {unsigned long}
 */
FIBER_API unsigned long acl_channel_recvul(ACL_CHANNEL* c);

/**
 * 向指定的 ACL_CHANNEL 中以非阻塞方式发送无符号长整形数值
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @param val {unsigned long} 要发送的数值
 * @return {int} 返回值 >= 0
 */
FIBER_API int acl_channel_sendul_nb(ACL_CHANNEL* c, unsigned long val);

/**
 * 从指定的 ACL_CHANNEL 中以非阻塞方式接收无符号长整形数值
 * @param c {ACL_CHANNEL*} 由 acl_channel_create 创建的管道对象
 * @return {unsigned long}
 */
FIBER_API unsigned long acl_channel_recvul_nb(ACL_CHANNEL* c);

/**************************** fiber wrapper *********************************/
#if defined(_WIN32) || defined(_WIN64)
typedef SOCKET socket_t;
typedef int socklen_t;
#else
typedef int socket_t;
#define INVALID_SOCKET	-1
#endif

FIBER_API socket_t acl_fiber_socket(int domain, int type, int protocol);
FIBER_API int acl_fiber_listen(socket_t, int backlog);

#if defined(_WIN32) || defined(_WIN64)
FIBER_API int WINAPI acl_fiber_close(socket_t fd);
FIBER_API socket_t WINAPI acl_fiber_accept(
	socket_t, struct sockaddr *, socklen_t *);
FIBER_API int WINAPI acl_fiber_connect(
	socket_t , const struct sockaddr *, socklen_t );

FIBER_API int WINAPI acl_fiber_recv(
	socket_t, char* buf, int len, int flags);
FIBER_API int WINAPI acl_fiber_recvfrom(socket_t, char* buf, size_t len,
	int flags, struct sockaddr* src_addr, socklen_t* addrlen);

FIBER_API int WINAPI acl_fiber_send(socket_t, const char* buf,
	int len, int flags);
FIBER_API int WINAPI acl_fiber_sendto(socket_t, const char* buf, size_t len,
	int flags, const struct sockaddr* dest_addr, socklen_t addrlen);

FIBER_API int WINAPI acl_fiber_select(int nfds, fd_set *readfds,
	fd_set *writefds, fd_set *exceptfds, const struct timeval *timeout);
FIBER_API int WINAPI acl_fiber_poll(struct pollfd *fds,
	unsigned long nfds, int timeout);
#else
FIBER_API int acl_fiber_close(socket_t fd);
FIBER_API socket_t acl_fiber_accept(socket_t , struct sockaddr *, socklen_t *);
FIBER_API int acl_fiber_connect(socket_t , const struct sockaddr *, socklen_t );
FIBER_API ssize_t acl_fiber_read(socket_t, void* buf, size_t count);
FIBER_API ssize_t acl_fiber_readv(socket_t, const struct iovec* iov, int iovcnt);
FIBER_API ssize_t acl_fiber_recvmsg(socket_t, struct msghdr* msg, int flags);

FIBER_API ssize_t acl_fiber_write(socket_t, const void* buf, size_t count);
FIBER_API ssize_t acl_fiber_writev(socket_t, const struct iovec* iov, int iovcnt);
FIBER_API ssize_t acl_fiber_sendmsg(socket_t, const struct msghdr* msg, int flags);

FIBER_API ssize_t acl_fiber_recv(socket_t, void* buf, size_t len, int flags);
FIBER_API ssize_t acl_fiber_recvfrom(socket_t, void* buf, size_t len, int flags,
	struct sockaddr* src_addr, socklen_t* addrlen);

FIBER_API ssize_t acl_fiber_send(socket_t, const void* buf, size_t len, int flags);
FIBER_API ssize_t acl_fiber_sendto(socket_t, const void* buf, size_t len, int flags,
	const struct sockaddr* dest_addr, socklen_t addrlen);

FIBER_API int acl_fiber_select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout);
FIBER_API int acl_fiber_poll(struct pollfd *fds, nfds_t nfds, int timeout);
#endif

/****************************************************************************/

/**
 * 在将写日志至日志文件前回调用户自定义的函数，且将日志信息传递给该函数，
 * 只有当用户通过 msg_pre_write 进行设置后才生效
 * @param ctx {void*} 用户的自定义参数
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 格式参数列表
 */
typedef void (*FIBER_MSG_PRE_WRITE_FN)(void *ctx, const char *fmt, va_list ap);

/**
 * 应用通过此函数类型可以自定义日志记录函数，当应用在打开日志前调用
 * msg_register 注册了自定义记录函数，则当应用写日志时便用此自定义
 * 函数记录日志，否则用缺省的日志记录函数
 * @param ctx {void*} 应用传递进去的参数
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 参数列表
 */
typedef void (*FIBER_MSG_WRITE_FN) (void *ctx, const char *fmt, va_list ap);

/**
 * 在打开日志前调用此函数注册应用自己的日志记录函数
 * @param write_fn {MSG_WRITE_FN} 自定义日志记录函数
 * @param ctx {void*} 自定义参数
 */
FIBER_API void acl_fiber_msg_register(FIBER_MSG_WRITE_FN write_fn, void *ctx);

/**
 * 将 msg_register 注册自定义函数清除，采用缺省的日志函数集
 */
FIBER_API void acl_fiber_msg_unregister(void);

/**
 * 在打开日志前调用此函数注册应用的私有函数，在记录日志前会先记录信息通过
 * 此注册的函数传递给应用
 * @param pre_write {MSG_PRE_WRITE_FN} 日志记录前调用的函数
 * @param ctx {void*} 自定义参数
 */
FIBER_API void acl_fiber_msg_pre_write(FIBER_MSG_PRE_WRITE_FN pre_write, void *ctx);

/**
 * 当未调用 msg_open 方式打开日志时，调用了 msg_info/error/fatal/warn
 * 的操作，是否允许信息输出至标准输出屏幕上，通过此函数来设置该开关，该开关
 * 仅影响是否需要将信息输出至终端屏幕而不影响是否输出至文件中
 * @param onoff {int} 非 0 表示允许输出至屏幕
 */
FIBER_API void acl_fiber_msg_stdout_enable(int onoff);

/**
 * 获得上次系统调用出错时的错误号
 * @return {int} 错误号
 */
FIBER_API int acl_fiber_last_error(void);

/**
 * 手工设置错误号
 * @param errnum {int} 错误号
 */
void acl_fiber_set_error(int errnum);

/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
