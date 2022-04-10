#ifndef FIBER_BASE_INCLUDE_H
#define FIBER_BASE_INCLUDE_H

#include <stdarg.h>
#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ACL_FIBER_ATTR {
	unsigned int   oflag;
#define	ACL_FIBER_ATTR_SHARE_STACK	(unsigned) 1 << 0

	size_t stack_size;
} ACL_FIBER_ATTR;

void acl_fiber_attr_init(ACL_FIBER_ATTR *attr);
void acl_fiber_attr_setstacksize(ACL_FIBER_ATTR *attr, size_t size);
void acl_fiber_attr_setsharestack(ACL_FIBER_ATTR *attr, int on);

typedef ACL_FIBER *((*FIBER_ALLOC_FN)(void (*)(ACL_FIBER *), const ACL_FIBER_ATTR *));
typedef ACL_FIBER *((*FIBER_ORIGIN_FN)(void));

FIBER_API void acl_fiber_register(FIBER_ALLOC_FN alloc_fn,
	FIBER_ORIGIN_FN origin_fn);

FIBER_API ACL_FIBER *acl_fiber_alloc(size_t size, void **pptr);

/**
 * Set flag if the system API should be hooked, default value is 1 internal
 * @param onoff {int} if need to hook the system API
 */
FIBER_API void acl_fiber_hook_api(int onoff);

/**
 * Set the global flag that if in non-blocking status, just for windows
 * @param yes {int} set in non-blocking status global if yes not 0
 */
FIBER_API void acl_fiber_set_non_blocking(int yes);

/**
 * Set the shared stack's memory size in shared stack mode
 * @param size {size_t} must more than 1024 for shared stack mode.
 */
FIBER_API void acl_fiber_set_shared_stack_size(size_t size);

/**
 * Get the shared stack's memory size if shared stack was enabled
 * @return {size_t} return > 0 if shared stack was enabled, or return 0.
 */
FIBER_API size_t acl_fiber_get_shared_stack_size(void);

/**
 * Create and start one fiber
 * @param fn {void (*)(ACL_FIBER*, void*)} the callback of fiber running
 * @param arg {void*} the second parameter of the callback fn
 * @param size {size_t} the virual memory size of the fiber created
 * @return {ACL_FIBER*}
 */
FIBER_API ACL_FIBER* acl_fiber_create(void (*fn)(ACL_FIBER*, void*),
	void* arg, size_t size);

FIBER_API ACL_FIBER* acl_fiber_create2(const ACL_FIBER_ATTR *attr,
	void (*fn)(ACL_FIBER*, void*), void* arg);

/**
 * Get the fibers count in deading status
 * @return {unsigned}
 */
FIBER_API unsigned acl_fiber_ndead(void);

/**
 * Get the fibers count in aliving status
 * @return {unsigned}
 */
FIBER_API unsigned acl_fiber_number(void);

/**
 * Create one fiber in background for freeing the dead fibers, specify the
 * maximum fibers in every recyling process
 * @param max {size_t} the maximum fibers to freed in every recyling process
 */
FIBER_API void acl_fiber_check_timer(size_t max);

/**
 * Get the current running fiber
 * @retur {ACL_FIBER*} if no running fiber NULL will be returned
 */
FIBER_API ACL_FIBER* acl_fiber_running(void);

/**
 * If the fiber using shared stack?
 * @param fiber {const ACL_FIBER*}
 * @return {int} return 0 if using private stack, or the shared stack was used
 */
FIBER_API int acl_fiber_use_share_stack(const ACL_FIBER *fiber);

/**
 * Get the fiber ID of the specified fiber
 * @param fiber {const ACL_FIBER*} the specified fiber object
 * @return {unsigned int} return the fiber ID
 */
FIBER_API unsigned int acl_fiber_id(const ACL_FIBER* fiber);

/**
 * Get the current running fiber's ID
 * @return {unsigned int} the current fiber's ID
 */
FIBER_API unsigned int acl_fiber_self(void);

/**
 * Set the error number to the specified fiber object
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 *  fiber will be used
 * @param errnum {int} the error number
 */
FIBER_API void acl_fiber_set_errno(ACL_FIBER* fiber, int errnum);

/**
 * Get the error number of assosiated fiber
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @return {int} get the error number of assosiated fiber
 */
FIBER_API int acl_fiber_errno(ACL_FIBER* fiber);

/**
 * @deprecated
 * @param fiber {ACL_FIBER*}
 * @param yesno {int}
 */
FIBER_API void acl_fiber_keep_errno(ACL_FIBER* fiber, int yesno);

/**
 * Get the assosiated fiber's status
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @return {int}
 */
FIBER_API int acl_fiber_status(const ACL_FIBER* fiber);

/**
 * Kill the suspended fiber and notify it to exit
 * @param fiber {const ACL_FIBER*} the specified fiber, NOT NULL
 */
FIBER_API void acl_fiber_kill(ACL_FIBER* fiber);

/**
 * Check if the specified fiber has been killed
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @return {int} non zero returned if been killed
 */
FIBER_API int acl_fiber_killed(ACL_FIBER* fiber);

/**
 * Check if the specified fiber has been signaled
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @return {int} non zero returned if been signed
 */
FIBER_API int acl_fiber_signaled(ACL_FIBER* fiber);

/**
 * Check if the specified fiber's socket has been closed by another fiber
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @return {int} non zero returned if been closed
 */
FIBER_API int acl_fiber_closed(ACL_FIBER* fiber);

/**
 * Check if the specified fiber has been canceled
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @return {int} non zero returned if been canceled
 */
FIBER_API int acl_fiber_canceled(ACL_FIBER* fiber);

/**
 * Wakeup the suspended fiber with the assosiated signal number
 * @param fiber {const ACL_FIBER*} the specified fiber, NOT NULL
 * @param signum {int} SIGINT, SIGKILL, SIGTERM ... refer to bits/signum.h
 */
FIBER_API void acl_fiber_signal(ACL_FIBER* fiber, int signum);

/**
 * Get the signal number got from other fiber
 * @param fiber {ACL_FIBER*} the specified fiber, if NULL the current running
 * @retur {int} the signal number got
 */
FIBER_API int acl_fiber_signum(ACL_FIBER* fiber);

/**
 * Suspend the current running fiber
 * @return {int}
 */
FIBER_API int acl_fiber_yield(void);

/**
 * Add the suspended fiber into resuming queue
 * @param fiber {ACL_FIBER*} the fiber, NOT NULL
 */
FIBER_API void acl_fiber_ready(ACL_FIBER* fiber);

/**
 * Suspend the current fiber and switch to run the next ready fiber
 */
FIBER_API void acl_fiber_switch(void);

/**
 * Set the fiber schedule process with automatically, in this way, when one
 * fiber was created, the schedule process will start automatically, but only
 * the first fiber was started, so you can create the other fibers in this
 * fiber. The default schedule mode is non-automatically, you should call the
 * acl_fiber_schedule or acl_fiber_schedule_with explicit
 */
FIBER_API void acl_fiber_schedule_init(int on);

/**
 * Start the fiber schedule process, the fibers in the ready quque will be
 * started in sequence.
 */
FIBER_API void acl_fiber_schedule(void);

/**
 * Start the fiber schedule process with the specified event type, the default
 * event type is FIBER_EVENT_KERNEL. acl_fiber_schedule using the default
 * event type. FIBER_EVENT_KERNEL is diffrent for diffrent OS platform:
 * Linux: epoll; BSD: kqueue; Windows: iocp.
 * @param event_mode {int} the event type, defined as FIBER_EVENT_XXX
 */
#define FIBER_EVENT_KERNEL	0	/* epoll/kqueue/iocp	*/
#define FIBER_EVENT_POLL	1	/* poll			*/
#define FIBER_EVENT_SELECT	2	/* select		*/
#define FIBER_EVENT_WMSG	3	/* win message		*/
FIBER_API void acl_fiber_schedule_with(int event_mode);

/**
 * Set the event type, the default type is FIBER_EVENT_KERNEL, this function
 * must be called before acl_fiber_schedule.
 * @param event_mode {int} the event type, defined as FIBER_EVENT_XXX
 */
FIBER_API void acl_fiber_schedule_set_event(int event_mode);

/**
 * Check if the current thread is in fiber schedule status
 * @return {int} non zero returned if in fiber schedule status
 */
FIBER_API int acl_fiber_scheduled(void);

/**
 * Stop the fiber schedule process, all fibers will be stopped
 */
FIBER_API void acl_fiber_schedule_stop(void);

/**
 * Let the current fiber sleep for a while
 * @param milliseconds {unsigned int} the milliseconds to sleep
 * @return {unsigned int} the rest milliseconds returned after wakeup
 */
FIBER_API unsigned int acl_fiber_delay(unsigned int milliseconds);

/**
 * Let the current fiber sleep for a while
 * @param seconds {unsigned int} the seconds to sleep
 * @return {unsigned int} the rest seconds returned after wakeup
 */
FIBER_API unsigned int acl_fiber_sleep(unsigned int seconds);

/**
 * Create one fiber timer
 * @param milliseconds {unsigned int} the timer wakeup milliseconds
 * @param size {size_t} the virtual memory of the created fiber
 * @param fn {void (*)(ACL_FIBER*, void*)} the callback when fiber wakeup
 * @param ctx {void*} the second parameter of the callback fn
 * @return {ACL_FIBER*} the new created fiber returned
 */
FIBER_API ACL_FIBER* acl_fiber_create_timer(unsigned int milliseconds,
	size_t size, void (*fn)(ACL_FIBER*, void*), void* ctx);

/**
 * Reset the timer milliseconds time before the timer fiber wakeup
 * @param timer {ACL_FIBER*} the fiber created by acl_fiber_create_timer
 * @param milliseconds {unsigned int} the new timer wakeup milliseconds
 */
FIBER_API void acl_fiber_reset_timer(ACL_FIBER* timer, unsigned int milliseconds);

/**
 * Set the DNS service addr
 * @param ip {const char*} ip of the DNS service
 * @param port {int} port of the DNS service
 */
FIBER_API void acl_fiber_set_dns(const char* ip, int port);

/* For fiber specific */

/**
 * Set the current fiber's local object
 * @param key {int*} the addr of indexed key, its initial value should <= 0,
 *  and one integer which > 0 will be set for it; the fiber local object will
 *  be assosiated with the indexed key.
 * @param ctx {void *} the fiber local object
 * @param free_fn {void (*)(void*)} the callback will be called before the
 *  current fiber exiting
 * @return {int} the integer value(>0) of indexed key returned, value less than
 *  0 will be returned if no running fiber
 */
FIBER_API int acl_fiber_set_specific(int* key, void* ctx, void (*free_fn)(void*));

/**
 * Get the current fiber's local object assosiated with the specified indexed key
 * @param key {int} the integer value returned by acl_fiber_set_specific
 * @retur {void*} NULL returned if no fiber local object with the specified key
 */
FIBER_API void* acl_fiber_get_specific(int key);

/****************************************************************************/

/**
 * Log function type used in fiber logging process, should be set by the
 * function acl_fiber_msg_pre_write
 * @param ctx {void*} the user's context
 * @param fmt {const char*} format of parameters
 * @param ap {va_list} list of parameters
 */
typedef void (*FIBER_MSG_PRE_WRITE_FN)(void *ctx, const char *fmt, va_list ap);

/**
 * Log function type used in fiber logging process, should be set by the
 * function acl_fiber_msg_register. This can be used by user for get the
 * logging information of fiber
 * @param ctx {void*} the user's context
 * @param fmt {const char*} format of parameters
 * @param ap {va_list} list of parameters
 */
typedef void (*FIBER_MSG_WRITE_FN) (void *ctx, const char *fmt, va_list ap);

/**
 * Set the user's log saving function when process started
 * @param write_fn {MSG_WRITE_FN} log function defined by the user
 * @param ctx {void*} parameter will be transfered to write_fn
 */
FIBER_API void acl_fiber_msg_register(FIBER_MSG_WRITE_FN write_fn, void *ctx);

/**
 * Cleanup the registered log callback by acl_fiber_msg_register
 */
FIBER_API void acl_fiber_msg_unregister(void);

/**
 * Register the user's callback
 * @param pre_write {MSG_PRE_WRITE_FN}
 * @param ctx {void*}
 */
FIBER_API void acl_fiber_msg_pre_write(FIBER_MSG_PRE_WRITE_FN pre_write, void *ctx);

/**
 * If showing the fiber schedule process's log to stdout
 * @param onoff {int} log will be showed to stdout if onoff isn't 0
 */
FIBER_API void acl_fiber_msg_stdout_enable(int onoff);

/**
 * Get the system error number of last system API calling
 * @return {int} error number
 */
FIBER_API int acl_fiber_last_error(void);

/**
 * Get the error information of last system API calling
 * @return {const char*}
 */
FIBER_API const char *acl_fiber_last_serror(void);

/**
 * Convert errno to string
 * @param errnum {int}
 * @param buf {char*} hold the result
 * @param size {size_t} buf's size
 * @retur {const char*} the addr of buf
 */
FIBER_API const char *acl_fiber_strerror(int errnum, char *buf, size_t size);

/**
 * Set the system error number
 * @param errnum {int} the error number
 */
FIBER_API void acl_fiber_set_error(int errnum);

/**
 * Set the fd limit for the current process
 * @param limit {int} the fd limit to be set
 * @return {int} the real fd limit will be returned
 */
FIBER_API int acl_fiber_set_fdlimit(int limit);

#if !defined(_WIN32) || !defined(_WIN64)
FIBER_API int acl_fiber_gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

FIBER_API void acl_fiber_memstat(void);

/****************************************************************************/

#ifdef __cplusplus
}
#endif

#endif
