#ifndef ACL_EVENTS_H_INCLUDED
#define ACL_EVENTS_H_INCLUDED

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include <time.h>
#include "../stdlib/acl_vstream.h"
#include "acl_timer.h"

/*--------------- Global macro definitions -------------------------------*/
 /* Event codes. */
#define ACL_EVENT_READ          (1 << 0)      /**< read event */
#define	ACL_EVENT_ACCEPT        (1 << 1)      /**< accept one connection */
#define ACL_EVENT_WRITE         (1 << 2)      /**< write event */
#define	ACL_EVENT_CONNECT       (1 << 3)      /**< client has connected the server */
#define ACL_EVENT_XCPT          (1 << 4)      /**< exception */
#define ACL_EVENT_TIME          (1 << 5)      /**< timer event */
#define	ACL_EVENT_RW_TIMEOUT    (1 << 6)      /**< read/write timeout event */
#define	ACL_EVENT_TIMEOUT       ACL_EVENT_RW_TIMEOUT

#define	ACL_EVENT_FD_IDLE	0
#define	ACL_EVENT_FD_BUSY	1

#define ACL_EVENT_ERROR		ACL_EVENT_XCPT

#define	ACL_EVENT_SELECT	0
#define	ACL_EVENT_POLL		1
#define	ACL_EVENT_KERNEL	2
#define ACL_EVENT_WMSG		3

 /*
  * Dummies.
  */
#define ACL_EVENT_NULL_TYPE	0
#define ACL_EVENT_NULL_CONTEXT	((char *) 0)


/* in acl_events.c */

typedef	struct	ACL_EVENT		ACL_EVENT;
typedef	struct	ACL_EVENT_FDTABLE	ACL_EVENT_FDTABLE;

/*
 * External interface.
 */
#if 0
typedef void (*ACL_EVENT_NOTIFY_FN) (int event_type, void *context);
typedef	ACL_EVENT_NOTIFY_FN	ACL_EVENT_NOTIFY_RDWR;
typedef	ACL_EVENT_NOTIFY_FN	ACL_EVENT_NOTIFY_TIME;
#else
typedef void (*ACL_EVENT_NOTIFY_RDWR)(int event_type, ACL_EVENT *event,
		ACL_VSTREAM *stream, void *context);
typedef void (*ACL_EVENT_NOTIFY_TIME)(int event_type, ACL_EVENT *event,
		void *context);
#endif

/*------------------------------------------------------------------*/

/**
 * Create an event loop object. Different backends can be chosen depending
 * on application needs; this function only creates the event object.
 * @param event_mode {int} Event loop mode; currently supports:
 *  ACL_EVENT_SELECT, ACL_EVENT_KERNEL, ACL_EVENT_POLL, ACL_EVENT_WMSG
 * @param use_thr {int} Whether to use multi-threaded event
 *  processing; 0 means single-threaded
 * @param delay_sec {int} Maximum wait time in seconds for each
 *  event loop iteration.
 *  When event_mode is ACL_EVENT_WMSG and this value is greater than 0, it is
 *  used as the message timeout value passed to acl_event_new_wmsg; otherwise
 *  the default Windows message timeout is used.
 * @param delay_usec {int} Maximum wait time in microseconds for each iteration
 *  (used only in select-based mode)
 * @return {ACL_EVENT*} Pointer to the new event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new(int event_mode, int use_thr,
	int delay_sec, int delay_usec);

/**
 * Create a new select-based event object (single-threaded).
 * @param delay_sec {int} Second-level timeout value used when calling select()
 * @param delay_usec {int} Microsecond-level timeout value used
 *  when calling select()
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_select(int delay_sec, int delay_usec);

/**
 * Create a new select-based event object with multi-thread support.
 * @param delay_sec {int} Second-level timeout value used when calling select()
 * @param delay_usec {int} Microsecond-level timeout value used
 *  when calling select()
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_select_thr(int delay_sec, int delay_usec);

/**
 * Create a new poll-based event object (single-threaded).
 * @param delay_sec {int} Second-level timeout value used when calling poll()
 * @param delay_usec {int} Microsecond-level timeout value used
 *  when calling poll()
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_poll(int delay_sec, int delay_usec);

/**
 * Create a new poll-based event object with multi-thread support.
 * @param delay_sec {int} Second-level timeout value used when calling poll()
 * @param delay_usec {int} Microsecond-level timeout value used
 *  when calling poll()
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_poll_thr(int delay_sec, int delay_usec);

/**
 * Create a new high-performance event object using
 * epoll/devpoll/kqueue, single-threaded.
 * @param delay_sec {int} Second-level timeout value used inside the event loop
 * @param delay_usec {int} Microsecond-level timeout value used
 *  inside the event loop (optional)
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_kernel(int delay_sec, int delay_usec);

/**
 * Create a new high-performance event object using epoll/devpoll/kqueue
 * with multi-thread support.
 * @param delay_sec {int} Second-level timeout value used inside the event loop
 * @param delay_usec {int} Microsecond-level timeout value used
 *  inside the event loop (optional)
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_kernel_thr(int delay_sec, int delay_usec);

/**
 * Create an event object that is driven by the Windows message system.
 * @param nMsg {unsigned int} If non-zero, bind the asynchronous events to this
 *        Windows message ID; otherwise use the default message ID.
 * @return {ACL_EVENT*} Pointer to the event object, or NULL on failure
 */
ACL_API ACL_EVENT *acl_event_new_wmsg(unsigned int nMsg);

#if defined (_WIN32) || defined(_WIN64)
ACL_API HWND acl_event_wmsg_hwnd(ACL_EVENT *eventp);
#endif

/**
 * Add a "watch-dog" event to wake up the select-based event loop in
 * multi-threaded mode and avoid being blocked indefinitely while other
 * threads are adding or closing connections.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 */
ACL_API void acl_event_add_dog(ACL_EVENT *eventp);

/**
 * Set user-defined hooks that are called before and after processing
 * events in the event loop.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param fire_begin {void (*)(ACL_EVENT*, void*)} Callback
 *  invoked before dispatching events
 * @param fire_end {void (*)(ACL_EVENT*, void*)} Callback invoked
 *  after dispatching events
 * @param ctx {void*} User context passed as the second argument to both callbacks
 */
ACL_API void acl_event_set_fire_hook(ACL_EVENT *eventp,
		void (*fire_begin)(ACL_EVENT*, void*),
		void (*fire_end)(ACL_EVENT*, void*),
		void* ctx);

/**
 * Set the internal check interval for the event loop, used to monitor whether
 * any events are stuck in an abnormal state. The default is 100 ms.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param n {int} Interval in milliseconds
 */
ACL_API void acl_event_set_check_inter(ACL_EVENT *eventp, int n);

/**
 * Free the event structure and its internal resources.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 */
ACL_API void acl_event_free(ACL_EVENT *eventp);

/**
 * Return the current timestamp from the event object.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @return {acl_int64} Current event time in microseconds
 */
ACL_API acl_int64 acl_event_time(ACL_EVENT *eventp);

/**
 * Drain and process all pending events in the event object.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 */
ACL_API void acl_event_drain(ACL_EVENT *eventp);

/**
 * Enable read events for a stream, and register a callback to be invoked when
 * data is readable, or when the connection is closed or times out.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream; the callback is
 *  triggered only for streams registered here
 * @param read_timeout {int} Read timeout in seconds
 * @param callback {ACL_EVENT_NOTIFY_RDWR} Callback invoked when
 *  the stream is readable or timed out
 * @param context {void*} User context passed to the callback
 */
ACL_API void acl_event_enable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int read_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context);

/**
 * Enable write events for a stream, and register a callback to be invoked when
 * the stream is writable, or when the connection is closed or times out.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream; the callback is
 *  triggered only for streams registered here
 * @param write_timeout {int} Write timeout in seconds
 * @param callback {ACL_EVENT_NOTIFY_RDWR} Callback invoked when
 *  the stream is writable or timed out
 * @param context {void*} User context passed to the callback
 */
ACL_API void acl_event_enable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int write_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context);

/**
 * Enable accept events for a listening socket, and register a callback to be
 * invoked when new connections arrive, are closed, or time out.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL listening stream
 * @param read_timeout {int} Accept timeout in seconds; 0 means no timeout
 * @param callback {ACL_EVENT_NOTIFY_RDWR} Callback invoked when
 *  a connection is accepted or times out
 * @param context {void*} User context passed to the callback
 */
ACL_API void acl_event_enable_listen(ACL_EVENT *eventp, ACL_VSTREAM *stream,
	int read_timeout, ACL_EVENT_NOTIFY_RDWR callback, void *context);

/**
 * Disable all read-related events associated with a given stream.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API void acl_event_disable_read(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Disable all write-related events associated with a given stream.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API void acl_event_disable_write(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Disable both read and write events associated with a given stream.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API void acl_event_disable_readwrite(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Test whether the given stream has any pending readable, writable or
 * exceptional events.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API int acl_event_isset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Test whether the given stream has a pending read event.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API int acl_event_isrset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Test whether the given stream has a pending write event.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API int acl_event_iswset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Test whether the given stream has a pending exceptional event.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param stream {ACL_VSTREAM*} Non-NULL stream
 */
ACL_API int acl_event_isxset(ACL_EVENT *eventp, ACL_VSTREAM *stream);

/**
 * Request a timer event.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param callback {ACL_EVENT_NOTIFY_TIME} Timer callback
 * @param context {void*} User context passed to the callback
 * @param delay {acl_int64} First trigger time: eventp->event_present + delay (microseconds)
 * @param keep {int} Whether the timer should repeat automatically
 * @return {acl_int64} Scheduled execution time in microseconds
 */
ACL_API acl_int64 acl_event_request_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context, acl_int64 delay, int keep);

/**
 * Cancel a timer event.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param callback {ACL_EVENT_NOTIFY_TIME} Timer callback
 * @param context {void*} User context passed to the callback
 * @return acl_int64 {acl_int64} Remaining time until the event
 *  would have fired, in microseconds
 */
ACL_API acl_int64 acl_event_cancel_timer(ACL_EVENT *eventp,
	ACL_EVENT_NOTIFY_TIME callback, void *context);

/**
 * Control whether a previously created timer should be kept and automatically
 * re-scheduled after it fires, so that it can be used in a loop.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param callback {ACL_EVENT_NOTIFY_TIME} Non-NULL timer callback
 * @param context {void*} User context for the callback
 * @param onoff {int} Non-zero to keep repeating the timer; 0 to
 *  disable repetition
 */
ACL_API void acl_event_keep_timer(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context, int onoff);

/**
 * Test whether the specified timer is currently configured to repeat.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param callback {ACL_EVENT_NOTIFY_TIME} Non-NULL timer callback
 * @param context {void*} User context for the callback
 * @return {int} Non-zero if the timer is configured to repeat; 0 otherwise
 */
ACL_API int acl_event_timer_ifkeep(ACL_EVENT *eventp, ACL_EVENT_NOTIFY_TIME callback,
	void *context);

/**
 * Run the event loop and block until it is stopped.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 */
ACL_API void acl_event_loop(ACL_EVENT *eventp);

/**
 * Set the second-level delay used when the event loop waits for new events.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param sec {int} Delay in seconds
 */
ACL_API void acl_event_set_delay_sec(ACL_EVENT *eventp, int sec);

/**
 * Set the microsecond-level delay used when the event loop waits
 * for new events.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @param usec {int} Delay in microseconds
 */
ACL_API void acl_event_set_delay_usec(ACL_EVENT *eventp, int usec);

/**
 * Get the second-level delay used when the event loop waits for new events.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @return {int} Delay in seconds
 */
ACL_API int acl_event_get_delay_sec(ACL_EVENT *eventp);

/**
 * Get the microsecond-level delay used when the event loop waits
 * for new events.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @return {int} Delay in microseconds
 */
ACL_API int acl_event_get_delay_usec(ACL_EVENT *eventp);

/**
 * Test whether the event loop is using a multi-threaded model.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @return {int} 0: no; non-zero: yes
 */
ACL_API int acl_event_use_thread(ACL_EVENT *eventp);

/**
 * Get the current event loop mode.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @return {int} One of ACL_EVENT_SELECT, ACL_EVENT_KERNEL, ACL_EVENT_POLL, etc.
 */
ACL_API int acl_event_mode(ACL_EVENT *eventp);

/**
 * Get the number of I/O events processed in the last event loop iteration.
 * @param eventp {ACL_EVENT*} Non-NULL event object
 * @return {int} Number of events processed in the most recent loop
 */
ACL_API int acl_event_last_nready(ACL_EVENT *eventp);

#ifdef	__cplusplus
}
#endif

#endif
