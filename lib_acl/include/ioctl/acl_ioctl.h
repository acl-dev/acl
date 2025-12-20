#ifndef	ACL_IOCTL_INCLUDE_H
#define	ACL_IOCTL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include "../event/acl_events.h"
#include "../stdlib/acl_vstream.h"

typedef struct ACL_IOCTL ACL_IOCTL;

/**
 * Callback type for I/O completion events. Users should implement their own
 * handler according to this prototype.
 * @param event_type {int} Event state, one of:
 *        ACL_EVENT_READ: data is ready for reading;
 *        ACL_EVENT_WRITE: buffer space is available for writing;
 *        ACL_EVENT_RW_TIMEOUT: read/write timeout;
 *        ACL_EVENT_XCPT: internal I/O exception.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Stream object
 * @param context {void*} User-defined context pointer
 */
typedef void (*ACL_IOCTL_NOTIFY_FN)(int event_type, ACL_IOCTL *ioc,
	ACL_VSTREAM *stream, void *context);

typedef void (*ACL_IOCTL_WORKER_FN)(ACL_IOCTL *ioc, void *arg);
typedef void (*ACL_IOCTL_THREAD_INIT_FN)(void *);
typedef void (*ACL_IOCTL_THREAD_EXIT_FN)(void *);

/*------------------------------------------------------------------*/
/* in acl_ioctl.c */
/**
 * Create an I/O controller.
 * @param max_threads {int} Maximum number of worker threads allowed
 * @param idle_timeout {int} Idle timeout for each thread, in
 *  seconds; if a thread stays idle longer than this value it
 *  will exit automatically
 * @return {ACL_IOCTL*} I/O controller handle
 */
ACL_API ACL_IOCTL *acl_ioctl_create(int max_threads, int idle_timeout);

/**
 * Create an I/O controller with a specified event backend.
 * @param event_mode {int} Event mode: ACL_EVENT_SELECT/ACL_EVENT_KERNEL
 * @param max_threads {int} Maximum number of worker threads allowed
 * @param idle_timeout {int} Idle timeout for each thread, in
 *  seconds; if a thread stays idle longer than this value it
 *  will exit automatically
 * @param delay_sec {int} Second-level delay used inside the event loop
 * @param delay_usec {int} Microsecond-level delay used inside the event loop
 * @return {ACL_IOCTL*} I/O controller handle
 */
ACL_API ACL_IOCTL *acl_ioctl_create_ex(int event_mode, int max_threads,
	int idle_timeout, int delay_sec, int delay_usec);

/**
 * Add a "watch-dog" event to the underlying event loop in multi-threaded mode
 * to prevent the select loop from being blocked while other threads are adding
 * or closing connections.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 */
ACL_API void acl_ioctl_add_dog(ACL_IOCTL *ioc);

/**
 * Set various control parameters for the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param name {int} One of the ACL_IOCTL_CTL_* constants below
 * @param ... Variable arguments depending on the control item
 */
ACL_API void acl_ioctl_ctl(ACL_IOCTL *ioc, int name, ...);
#define	ACL_IOCTL_CTL_END          0  /**< End of control list marker */
#define	ACL_IOCTL_CTL_THREAD_MAX   1  /**< Maximum number of worker threads */
#define	ACL_IOCTL_CTL_THREAD_IDLE  2  /**< Thread idle timeout */
#define	ACL_IOCTL_CTL_DELAY_SEC    3  /**< Select timeout in seconds */
#define	ACL_IOCTL_CTL_DELAY_USEC   4  /**< Select timeout in microseconds */
#define	ACL_IOCTL_CTL_INIT_FN      5  /**< Thread init callback when a worker starts */
#define	ACL_IOCTL_CTL_EXIT_FN      6  /**< Thread exit callback when a worker quits */
#define	ACL_IOCTL_CTL_INIT_CTX     7  /**< Context used by the thread init callback */
#define	ACL_IOCTL_CTL_EXIT_CTX     8  /**< Context used by the thread exit callback */
#define ACL_IOCTL_CTL_THREAD_STACKSIZE  9  /**< Stack size of worker threads (bytes) */

/**
 * Free all resources associated with an I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 */
ACL_API void acl_ioctl_free(ACL_IOCTL *ioc);

/**
 * Start the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @return {int} 0 on success; < 0 on error
 */
ACL_API int acl_ioctl_start(ACL_IOCTL *ioc);

/**
 * Run the message loop (for single-threaded mode).
 * @param ioc {ACL_IOCTL*} I/O controller handle
 */
ACL_API void acl_ioctl_loop(ACL_IOCTL *ioc);

/**
 * Get the underlying event object of the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @return {ACL_EVENT*} Associated event object
 */
ACL_API ACL_EVENT *acl_ioctl_event(ACL_IOCTL *ioc);

/**
 * Disable both read and write events on a stream managed by the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 */
ACL_API void acl_ioctl_disable_readwrite(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Disable read events on a stream managed by the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 */
ACL_API void acl_ioctl_disable_read(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Disable write events on a stream managed by the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 */
ACL_API void acl_ioctl_disable_write(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Test whether a stream has any enabled I/O events (read or write).
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 * @return {int} 1 if any event is enabled; 0 otherwise
 */
ACL_API int acl_ioctl_isset(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Test whether a stream has read events enabled.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 * @return {int} 1 if read events are enabled; 0 otherwise
 */
ACL_API int acl_ioctl_isrset(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Test whether a stream has write events enabled.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 * @return {int} 1 if write events are enabled; 0 otherwise
 */
ACL_API int acl_ioctl_iswset(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Close a stream in IOCP mode; the stream will be closed automatically after
 * all pending I/O is finished.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 * @return {int} Indicates whether the stream has been closed:
 *         0: the stream still has pending events; close will be done asynchronously;
 *         1: no pending events; the stream has been closed synchronously.
 */
ACL_API int acl_ioctl_iocp_close(ACL_IOCTL *ioc, ACL_VSTREAM *stream);

/**
 * Register a read event handler for a client stream.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 * @param timeout {int} Read timeout in seconds
 * @param callback {ACL_IOCTL_NOTIFY_FN} Callback invoked when
 *  data is readable or a timeout occurs
 * @param context {void*} User-defined context passed to the callback
 */
ACL_API void acl_ioctl_enable_read(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/**
 * Register a write event handler for a client stream.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream
 * @param timeout {int} Write timeout in seconds
 * @param callback {ACL_IOCTL_NOTIFY_FN} Callback invoked when
 *  the stream is writable or a timeout occurs
 * @param context {void*} User-defined context passed to the callback
 */
ACL_API void acl_ioctl_enable_write(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/**
 * Asynchronously connect to a remote server; when the connection succeeds or
 * times out, the user callback is invoked.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Client stream used to detect remote server state
 * @param timeout {int} Connection timeout in seconds
 * @param callback {ACL_IOCTL_NOTIFY_FN} Callback invoked when the connection
 *        succeeds, fails, or times out
 * @param context {void*} User-defined context passed to the callback
 */
ACL_API void acl_ioctl_enable_connect(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/**
 * Register a listening socket with the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param stream {ACL_VSTREAM*} Local listening stream whose
 *  state changes are monitored
 * @param timeout {int} Timeout for waiting for new connections; if no new
 *  connection arrives within the timeout, the callback can handle the timeout
 *  event. A value of 0 means wait indefinitely until a connection arrives.
 * @param callback {ACL_IOCTL_NOTIFY_FN} Callback invoked when a connection is
 *  accepted, an error occurs, or a timeout happens
 * @param context {void*} User-defined context passed to the callback
 */
ACL_API void acl_ioctl_enable_listen(ACL_IOCTL *ioc, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN callback, void *context);

/*------------------------------------------------------------------*/
/**
 * Connect to a remote server.
 * @param addr {const char*} Remote server address, format:
 *  ip:port, e.g. "192.168.0.1:80"
 * @param timeout {int} Connection timeout mode:
 *         1) 0:   Non-blocking connect to the remote server
 *         2) -1:  Blocking connect until success or failure
 *         3) > 0: Timed blocking connect to the remote server
 * @return {ACL_VSTREAM*} Client stream.
 *  != NULL: connection succeeded and a stream is returned;
 *  == NULL: connection failed or an error occurred.
 * Note:
 *     When timeout is 0), the returned ACL_VSTREAM must be registered for
 *     asynchronous connect state checking via acl_ioctl_enable_connect()
 *     in order to know when the connection actually completes.
 */
ACL_API ACL_VSTREAM *acl_ioctl_connect(const char *addr, int timeout);

/**
 * Create a listening socket.
 * @param addr {const char*} Local address to bind, format:
 *  ip:port, e.g. "127.0.0.1:80"
 * @param qlen {int} Listen backlog length
 * @return {ACL_VSTREAM*} Listening stream; != NULL on success, NULL on error.
 * Note: For asynchronous handling, call acl_ioctl_enable_listen() to accept
 *     client connections via callback.
 */
ACL_API ACL_VSTREAM *acl_ioctl_listen(const char *addr, int qlen);

/**
 * Create a listening socket with extended options.
 * @param addr {const char*} Local address to bind, format:
 *  ip:port, e.g. "127.0.0.1:80"
 * @param qlen {int} Listen backlog length
 * @param block_mode {int} Blocking mode: ACL_BLOCKING for blocking,
 *  ACL_NON_BLOCKING for non-blocking
 * @param io_bufsize {int} Buffer size (bytes) for each client stream
 * @param io_timeout {int} Read/write timeout for client streams, in seconds
 * @return {ACL_VSTREAM*} Listening stream; != NULL on success, NULL on error.
 * Note: For asynchronous handling, call acl_ioctl_enable_listen() to accept
 *     client connections via callback.
 */
ACL_API ACL_VSTREAM *acl_ioctl_listen_ex(const char *addr, int qlen,
	int block_mode, int io_bufsize, int io_timeout);

/**
 * Accept a client connection from a listening socket.
 * @param sstream {ACL_VSTREAM*} Listening stream
 * @param ipbuf {char*} Buffer to store the client's address
 * @param size {int} Size of ipbuf
 * @return {ACL_VSTREAM*} Client stream. != NULL means a client was accepted;
 *  == NULL may indicate the system interrupted the call and the
 *  caller should retry.
 */
ACL_API ACL_VSTREAM *acl_ioctl_accept(ACL_VSTREAM *sstream,
	char *ipbuf, int size);

/**
 * Request a timer event tied to the I/O controller; a simple wrapper of
 * acl_event_request_timer.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} Timer callback
 * @param context {void*} One of the arguments passed to timer_fn
 * @param idle_limit {acl_int64} Idle time limit in microseconds
 * @return {acl_int64} Remaining time in microseconds
 */
ACL_API acl_int64 acl_ioctl_request_timer(ACL_IOCTL *ioc,
	ACL_EVENT_NOTIFY_TIME timer_fn, void *context, acl_int64 idle_limit);

/**
 * Cancel a timer event; a simple wrapper of acl_event_cancel_timer.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} Timer callback
 * @param context {void*} One of the arguments passed to timer_fn
 * @return {acl_int64} Remaining time in microseconds
 */
ACL_API acl_int64 acl_ioctl_cancel_timer(ACL_IOCTL *ioc,                                                            
	ACL_EVENT_NOTIFY_TIME timer_fn, void *context);

/**
 * Add a new job to the worker thread pool managed by the I/O controller.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @param callback {ACL_IOCTL_WORKER_FN} Worker callback
 * @param arg {void*} User-defined argument passed to the worker callback
 * @return {int} 0: ok; < 0: error
 */
ACL_API int acl_ioctl_add(ACL_IOCTL *ioc,
	ACL_IOCTL_WORKER_FN callback, void *arg);

/**
 * Get the number of worker threads currently in the pool.
 * @param ioc {ACL_IOCTL*} I/O controller handle
 * @return {int} Number of worker threads; -1 on error; >= 0 on success
 */
ACL_API int acl_ioctl_nworker(ACL_IOCTL *ioc);

#ifdef	__cplusplus
}
#endif

#endif
