/**
 * @file	acl_aio.h
 * @author	zsx
 * @date	2010-1-2
 * @brief	This file defines the ACL_AIO asynchronous communication
 *  framework and ACL_ASTREAM asynchronous stream objects, along with
 *  their related interfaces.
 * @version	1.1
 */

#ifndef	ACL_AIO_INCLUDE_H
#define	ACL_AIO_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#include <stdarg.h>
#ifdef	ACL_UNIX
#include <sys/uio.h>
#endif

#include "../stdlib/acl_stdlib.h"
#include "../event/acl_events.h"
#include "../net/acl_netdb.h"

/*---------------- Data structure type definitions --------------------------*/

/**
 * Asynchronous I/O framework object type definition.
 */
typedef struct ACL_AIO ACL_AIO;

/**
 * Asynchronous stream type definition.
 */
typedef struct ACL_ASTREAM ACL_ASTREAM;

/**
 * Event notification callback function type. When a monitored stream
 * has readable data, this callback is called to notify the
 * user-registered function.
 * Currently, the following asynchronous functions trigger this callback:
 *   acl_aio_gets, acl_aio_gets_nonl, acl_aio_read, acl_aio_readn.
 * @param astream {ACL_ASTREAM*} Asynchronous stream pointer
 * @param context {void*} User-defined context parameter
 * @param data {const char*} Pointer to the data read from the stream
 * @param dlen {int} Length of data
 * @return {int} If this function returns -1, the corresponding
 *  stream should be closed.
 */
typedef int (*ACL_AIO_READ_FN)(ACL_ASTREAM *astream,
	void *context, char *data, int dlen);

/**
 * Event notification callback function type. When a certain
 * asynchronous stream becomes readable/writable, this callback is
 * called to notify the user-registered function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream pointer
 * @param context {void*} User-defined context parameter
 * @return {int} If this function returns -1, the corresponding
 *  asynchronous stream should be closed.
 */
typedef int (*ACL_AIO_NOTIFY_FN)(ACL_ASTREAM *astream, void *context);

/**
 * Event notification callback function type. When a certain monitored
 * stream has writable data, this callback is called to notify the
 * user-registered function.
 * Currently, the following asynchronous functions trigger this callback:
 *   acl_aio_writen, acl_aio_writev, acl_aio_fprintf, acl_aio_vfprintf.
 * @param astream {ACL_ASTREAM*} Asynchronous stream pointer
 * @param context {void*} User-defined context parameter
 * @return {int} If this function returns -1, the corresponding
 *  asynchronous stream should be closed.
 */
typedef int (*ACL_AIO_WRITE_FN)(ACL_ASTREAM *astream, void *context);

/**
 * When a new client connection is accepted on a listening
 * stream, the asynchronous framework will call this callback
 * and pass the new connection to the user; similarly, when
 * the user sets a timeout value for the stream and the
 * timeout expires, this callback will also be triggered.
 * This callback is called by the asynchronous framework:
 * acl_aio_accept.
 * @param cstream {ACL_ASTREAM*} The client stream obtained through
 *  accept() on sstream
 * @param context {void*} User-defined context data
 * @return {int} If this callback returns -1, it indicates to stop
 *  accepting new client connections
 */
/**
 * When a certain listening stream accepts a new client connection,
 * the asynchronous framework receives the connection and passes it
 * to the user; otherwise, if the user sets a timeout value for this
 * listening stream and the timeout value is reached, it will also
 * trigger this callback and return. This callback is triggered by
 * the asynchronous function: acl_aio_accept.
 * @param cstream {ACL_ASTREAM*} Client stream obtained from sstream
 *  through accept()
 * @param context {void*} User-defined context parameter
 * @return {int} If this function's return value is -1, it indicates
 *  to stop accepting new client connections
 */
typedef int (*ACL_AIO_ACCEPT_FN)(ACL_ASTREAM *cstream,	void *context);

/**
 * When a certain listening stream has a new client connection, the
 * asynchronous framework will callback the user-registered function,
 * and the user needs to use this listening stream to accept the
 * client connection. This callback is triggered by the asynchronous
 * function: acl_aio_listen.
 * @param sstream {ACL_ASTREAM*} Listening stream
 * @param context {void*} User-defined context parameter
 * @return {int} If this function's return value is -1, it will not
 *  affect the listening stream's functionality
 * Note: Note that this callback function's functionality is different
 *  from ACL_AIO_ACCEPT_FN.
 */
typedef int (*ACL_AIO_LISTEN_FN)(ACL_ASTREAM *sstream, void *context);

/**
 * When asynchronously connecting to a remote server, event
 * notification callback function triggered when connection fails,
 * times out, or succeeds, and passed to the user-registered
 * function. This callback is triggered by the asynchronous function:
 * acl_aio_connect.
 * @param cstream {ACL_ASTREAM*} Client stream being monitored for
 *  connection status
 * @param context {void*} User-defined context parameter
 * @return {int} If calling this function returns -1, the
 *  asynchronous stream needs to be closed
 */
typedef int (*ACL_AIO_CONNECT_FN)(ACL_ASTREAM *cstream, void *context);

typedef struct ACL_ASTREAM_CTX ACL_ASTREAM_CTX;

ACL_API int acl_astream_get_status(const ACL_ASTREAM_CTX *ctx);
#define ACL_ASTREAM_STATUS_INVALID		-1
#define ACL_ASTREAM_STATUS_OK			0
#define ACL_ASTREAM_STATUS_NS_ERROR		1
#define ACL_ASTREAM_STATUS_CONNECT_ERROR	2
#define ACL_ASTREAM_STATUS_CONNECT_TIMEOUT	3

ACL_API const ACL_SOCKADDR *acl_astream_get_ns_addr(const ACL_ASTREAM_CTX *ctx);
ACL_API const ACL_SOCKADDR *acl_astream_get_serv_addr(const ACL_ASTREAM_CTX *ctx);
ACL_API ACL_ASTREAM *acl_astream_get_conn(const ACL_ASTREAM_CTX *ctx);
ACL_API void *acl_astream_get_ctx(const ACL_ASTREAM_CTX *ctx);

/**
 * Callback function type when asynchronously connecting to a remote
 * server, used by acl_aio_connect_addr()
 * @param ctx {ACL_ASTREAM_CTX*} Callback function's context
 *  parameter, use acl_astream_get_xxx to get the object pointers
 *  contained in this structure
 */
typedef int (*ACL_AIO_CONNECT_ADDR_FN)(const ACL_ASTREAM_CTX *ctx);

/**
 * Callback function pointer when read/write operations timeout.
 * @param astream {ACL_ASTREAM*} Asynchronous stream pointer
 * @param context {void*} User data parameter
 * @return {int} If this function's return value is -1, for
 *  read/write streams it indicates to close the asynchronous
 *  read/write stream, for listening streams it indicates to stop
 *  accepting new client connections, otherwise 0 indicates continue
 */
typedef int (*ACL_AIO_TIMEO_FN)(ACL_ASTREAM *astream, void *context);

/**
 * Function that needs to be called back to the user-registered
 * function when closing an asynchronous read/write stream.
 * @param astream {ACL_ASTREAM*} Asynchronous stream pointer
 * @param context {void*} User data parameter
 * @return {int} Regardless of the return value, the asynchronous
 *  stream needs to be closed
 */
typedef int (*ACL_AIO_CLOSE_FN)(ACL_ASTREAM *astream, void *context);

/* Asynchronous stream type definition */

struct ACL_ASTREAM {
	ACL_AIO *aio;		/**< Asynchronous event framework */
	ACL_VSTREAM *stream;	/**< Synchronous stream */

	ACL_VSTRING strbuf;	/**< Internal string buffer */
	int   timeout;		/**< IO timeout time */
	int   nrefer;		/**< Reference count, prevents premature
				 *   closure before all references are released */
	int   flag;		/**< Flag bits */
#define ACL_AIO_FLAG_IOCP_CLOSE     (1 << 0) /* Whether to use
						 * IOCP close flag bit */
#define	ACL_AIO_FLAG_ISRD           (1 << 1) /* Whether read
						 * event is registered */
#define	ACL_AIO_FLAG_ISWR           (1 << 2) /* Whether write
						 * event is registered */
#define ACL_AIO_FLAG_DELAY_CLOSE    (1 << 3) /* Whether in
						 * delayed close state */
#define ACL_AIO_FLAG_DEAD           (1 << 4) /* Whether socket
						 * is already dead */
#define	ACL_AIO_FLAG_FLUSH_CLOSE    (1 << 5) /* Whether to
						 * flush all data before
						 * closing */

	ACL_FIFO write_fifo;	/**< First-in-first-out queue for asynchronous writes */
	int   write_left;	/**< Remaining unwritten data length */
	int   write_offset;	/**< Offset of the first position in write buffer */
	int   write_nested;	/**< Write nested depth */
	int   write_nested_limit;  /**< Write nested depth limit */

	int   (*read_ready_fn) (ACL_VSTREAM *, ACL_VSTRING *, int *);
	int   read_nested;	/**< Read nested depth */
	int   read_nested_limit;  /**< Read nested depth limit */
	int   count;		/**< Second parameter value used
				 *   when calling acl_aio_readn()/2 */
	int   keep_read;	/**< Whether to enable continuous read */
	int   accept_nloop;	/**<  Number of times acl_aio_accept
				 *   internally loops accept */
	int   error;		/**< Current socket's error code */
	int   line_length;	/**< When reading by line, this value
				 *   limits the maximum length per line */

	ACL_AIO_ACCEPT_FN  accept_fn;	/**< Callback function when accept succeeds */
	ACL_AIO_LISTEN_FN  listen_fn;	/**< Callback function when listening
					 *   stream has connection */
	void *context;			/**< User-set parameter */

	ACL_AIO_NOTIFY_FN  can_read_fn; /**< Callback function when readable */
	void *can_read_ctx;		/**< One of can_read_fn parameters */
	ACL_AIO_NOTIFY_FN  can_write_fn; /**< Callback function when writable */
	void *can_write_ctx;		/**< One of can_write_fn parameters */

	ACL_ARRAY *read_handles;	/**< Additional callback functions when reading */
	ACL_ARRAY *write_handles;	/**< Additional callback functions when writing */
	ACL_ARRAY *close_handles;	/**< Additional callback functions when closing */
	ACL_ARRAY *timeo_handles;	/**< Additional callback functions when timeout */
	ACL_ARRAY *connect_handles;	/**< Additional callback functions
					 *   when connection succeeds */
	ACL_FIFO   reader_fifo;		/**< Callback function queue when reading */
	ACL_FIFO   writer_fifo;		/**< Callback function queue when writing */

	/* Callback function when readable */
	void (*event_read_callback)(int event_type, ACL_ASTREAM *astream);
};

/**
 * Set asynchronous IO timeout time.
 */
#define ACL_AIO_SET_TIMEOUT(stream_ptr, _timeo_) do {  \
	ACL_ASTREAM *__stream_ptr = stream_ptr;        \
	__stream_ptr->timeout = _timeo_;               \
} while(0)

/**
 * Set asynchronous stream context parameter.
 */
#define ACL_AIO_SET_CTX(stream_ptr, _ctx_) do {  \
	ACL_ASTREAM *__stream_ptr = stream_ptr;  \
	__stream_ptr->context = _ctx_;           \
} while(0)

/*---------------- Asynchronous framework interfaces ------------------------*/

/**
 * Create an asynchronous communication asynchronous framework object,
 * specifying whether to use epoll/devpoll
 * @param event_mode {int} Event handling mode: ACL_EVENT_SELECT, ACL_EVENT_POLL
 *  , ACL_EVENT_KERNEL, ACL_EVENT_WMSG
 * @return {ACL_AIO*} Returns an asynchronous framework object.
 *  OK: != NULL; ERR: == NULL.
 */
ACL_API ACL_AIO *acl_aio_create(int event_mode);

/**
 * Create an asynchronous framework object, specifying whether to use
 * epoll/devpoll/windows message
 * @param event_mode {int} Event handling mode: ACL_EVENT_SELECT, ACL_EVENT_POLL
 *  , ACL_EVENT_KERNEL, ACL_EVENT_WMSG
 * @param nMsg {unsigned int} For _WIN32 message queue mode, when event_mode is
 *  ACL_EVENT_WMSG this value is effective, indicating the message
 *  value bound to the asynchronous framework
 * @return {ACL_AIO*} Returns an asynchronous framework object.
 *  OK: != NULL; ERR: == NULL.
 */
ACL_API ACL_AIO *acl_aio_create2(int event_mode, unsigned int nMsg);

/**
 * Create an asynchronous framework using an event object.
 * @param event {ACL_EVENT *}
 * @return {ACL_AIO *}
 */
ACL_API ACL_AIO *acl_aio_create3(ACL_EVENT *event);

/**
 * Get the DNS query object bound to this aio framework.
 * @param aio {ACL_AIO*}
 * @return {ACL_DNS*} If NULL indicates no DNS query object is bound,
 *  otherwise when the value is not NULL, the application can
 *  directly convert this value to ACL_DNS type XXX. Due to circular
 *  dependency header file issues, it is declared as void*.
 */
ACL_API void *acl_aio_dns(ACL_AIO *aio);

/**
 * Set DNS server address list. Only when DNS server address is set,
 * internal optimization will support asynchronous connection to
 * server address.
 * @param aio {ACL_AIO*}
 * @param dns_list {const char*} DNS server address list,
 *  format: ip1:port,ip2:port...
 * @param timeout {int} DNS query timeout time (seconds)
 * @return {int} Whether DNS query object was created successfully,
 *  0 indicates success, -1 indicates failure. Failure reasons
 *  include: unable to create UDP socket or UDP socket creation
 *  failed
 */
ACL_API int acl_aio_set_dns(ACL_AIO *aio, const char *dns_list, int timeout);

/**
 * Delete DNS server address list.
 * @param aio {ACL_AIO*}
 * @param dns_list {const char*} DNS server address list,
 *  format: ip1:port,ip2:port...
 */
ACL_API void acl_aio_del_dns(ACL_AIO *aio, const char *dns_list);

/**
 * Clear all DNS server addresses bound to aio.
 * @param aio {ACL_AIO*}
 */
ACL_API void acl_aio_clear_dns(ACL_AIO *aio);

/**
 * Free an asynchronous communication asynchronous framework object,
 * and also free the empty aio->event object.
 * @param aio {ACL_AIO*} Asynchronous framework object
 */
ACL_API void acl_aio_free(ACL_AIO *aio);

/**
 * Free an asynchronous communication asynchronous framework object.
 * @param keep {int} Whether to also free the event object bound to aio
 * @param aio {ACL_AIO*} Asynchronous framework object
 */
ACL_API void acl_aio_free2(ACL_AIO *aio, int keep);

/**
 * Asynchronous IO message loop (called in single-threaded mode)
 * @param aio {ACL_AIO*} Asynchronous framework object
 */
ACL_API void acl_aio_loop(ACL_AIO *aio);

/**
 * Get the number of ready events in the last event loop.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @return {int} -1 indicates an error occurred, otherwise returns value >= 0
 */
ACL_API int acl_aio_last_nready(ACL_AIO *aio);

/**
 * Check all asynchronous streams in ACL_AIO that should be closed,
 * and some asynchronous streams that need to be delayed will be
 * closed.
 * @param aio {ACL_AIO*} Asynchronous framework object
 */
ACL_API void acl_aio_check(ACL_AIO *aio);

/**
 * Get the event loop object.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @return {ACL_EVENT*}
 */
ACL_API ACL_EVENT *acl_aio_event(ACL_AIO *aio);

/**
 * Get the event loop's mode.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @return {int} ACL_EVENT_KERNEL/ACL_EVENT_SELECT/ACL_EVENT_POLL
 */
ACL_API int acl_aio_event_mode(ACL_AIO *aio);

/**
 * Check if asynchronous IO framework is in continuous read mode.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @return {int} != 0: Yes; == 0: No
 */
ACL_API int acl_aio_get_keep_read(ACL_AIO *aio);

/**
 * Set asynchronous IO framework's continuous read mode.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param onoff {int} 0: Disable continuous read; != 0: Enable continuous read
 */
ACL_API void acl_aio_set_keep_read(ACL_AIO *aio, int onoff);

/**
 * Get the second part of the current asynchronous framework loop's wait time.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @return {int} When using select/poll/epoll/kqueue/devpoll, the
 *  wait time in seconds
 */
ACL_API int acl_aio_get_delay_sec(ACL_AIO *aio);

/**
 * Get the microsecond part of the current asynchronous framework
 * loop's wait time.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @return {int} Microsecond-level wait time for
 *  select/poll/epoll/kqueue/devpoll
 */
ACL_API int acl_aio_get_delay_usec(ACL_AIO *aio);

/**
 * Set the second-level part of the asynchronous framework loop's wait time.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param delay_sec {int} Second-level wait time for
 *  select/poll/epoll/kqueue/devpoll
 */
ACL_API void acl_aio_set_delay_sec(ACL_AIO *aio, int delay_sec);

/**
 * Set the microsecond-level part of the asynchronous framework
 * loop's wait time.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param delay_usec {int} Microsecond-level wait time for
 *  select/poll/epoll/kqueue/devpoll
 */
ACL_API void acl_aio_set_delay_usec(ACL_AIO *aio, int delay_usec);

/**
 * Set the event loop's timeout check interval, internal default value is 100 ms
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param check_inter {int} Timeout check interval (seconds)
 */
ACL_API void acl_aio_set_check_inter(ACL_AIO *aio, int check_inter);

/**
 * Set the asynchronous stream's read buffer size.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param rbuf_size {int} Read buffer size
 */
ACL_API void acl_aio_set_rbuf_size(ACL_AIO *aio, int rbuf_size);

/**
 * Set the number of times a listening stream loops to accept
 * client connections each time.
 * @param astream {ACL_ASTREAM*} Listening stream
 * @param nloop {int}
 */
ACL_API void acl_aio_set_accept_nloop(ACL_ASTREAM *astream, int nloop);

/**
 * Get the asynchronous framework object from an asynchronous stream.
 * @param stream {ACL_ASTREAM*} Asynchronous IO stream
 * @return {ACL_AIO*} Asynchronous framework object
 */
ACL_API ACL_AIO *acl_aio_handle(ACL_ASTREAM *stream);

/**
 * Set the context for an asynchronous stream.
 * @param stream {ACL_ASTREAM*} Asynchronous IO stream
 * @param ctx {void*} Context
 */
ACL_API void acl_aio_set_ctx(ACL_ASTREAM *stream, void *ctx);

/**
 * Get the context for an asynchronous stream.
 * @param stream {ACL_ASTREAM*} Asynchronous IO stream
 * @return {void*} Context of the asynchronous stream
 */
ACL_API void *acl_aio_get_ctx(ACL_ASTREAM *stream);

/**
 * Create an asynchronous stream object.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param stream {ACL_VSTREAM*} Stream to be monitored, which can
 *  trigger read/write
 *  events and timeout callbacks registered by the user.
 * @return {ACL_ASTREAM*} Asynchronous stream object
 */
ACL_API ACL_ASTREAM *acl_aio_open(ACL_AIO *aio, ACL_VSTREAM *stream);

/**
 * Close an asynchronous IO stream using IOCP (Windows-specific).
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 */
ACL_API void acl_aio_iocp_close(ACL_ASTREAM *astream);

/**
 * Whether to flush all data in the write buffer before closing the
 * socket. The internal default value is 1 (need to write all data
 * before closing the socket). If you need to close the socket
 * immediately without waiting for all data to be written, you need
 * to call this function and set the second parameter to 0
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @param yes {int} If 0, indicates that all data in the write
 *  buffer does not need to be written before closing the socket
 */
ACL_API void acl_aio_flush_on_close(ACL_ASTREAM *astream, int yes);

/**
 * Get the underlying IO stream, this function is mainly used to
 * convert asynchronous IO to synchronous IO for read/write
 * @param astream {ACL_ASTREAM*} Asynchronous IO stream
 * @return {ACL_VSTREAM*} Stream object
 */
ACL_API ACL_VSTREAM *acl_aio_cancel(ACL_ASTREAM *astream);

/**
 * Get the maximum number of client connections that a listening
 * stream can accept each time.
 * @param astream {ACL_ASTREAM *} Listening stream
 * @return {int} Maximum number of connections accepted each time
 * @return {int} Maximum number of client connections that a
 *  listening stream can loop to accept during each accept process.
 *  The minimum value is 1
 */
ACL_API int acl_aio_get_accept_max(ACL_ASTREAM *astream);

/**
 * Set the maximum number of client connections that a listening
 * stream can accept each time.
 * @param astream {ACL_ASTREAM *} Listening stream
 * @param accept_max {int} Maximum number of client connections
 *  that a listening stream can loop to accept during each accept
 *  process.
 *  The minimum value is 1
 */
ACL_API void acl_aio_set_accept_max(ACL_ASTREAM *astream, int accept_max);

/**
 * Add additional read callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_add_read_hook(ACL_ASTREAM *astream,
	ACL_AIO_READ_FN callback, void *ctx);

/**
 * Add additional write callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_add_write_hook(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN callback, void *ctx);

/**
 * Add additional close callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_add_close_hook(ACL_ASTREAM *astream,
	ACL_AIO_CLOSE_FN callback, void *ctx);

/**
 * Add additional timeout callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_add_timeo_hook(ACL_ASTREAM *astream,
	ACL_AIO_TIMEO_FN callback, void *ctx);

/**
 * Add additional connection success callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_add_connect_hook(ACL_ASTREAM *astream,
	ACL_AIO_CONNECT_FN callback, void *ctx);

/**
 * Delete additional read callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_del_read_hook(ACL_ASTREAM *astream,
	ACL_AIO_READ_FN callback, void *ctx);

/**
 * Delete additional write callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_del_write_hook(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN callback, void *ctx);

/**
 * Delete additional close callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_del_close_hook(ACL_ASTREAM *astream,
	ACL_AIO_CLOSE_FN callback, void *ctx);

/**
 * Delete additional timeout callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_del_timeo_hook(ACL_ASTREAM *astream,
	ACL_AIO_TIMEO_FN callback, void *ctx);

/**
 * Delete additional connection success callback function.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 * @param callback {ACL_AIO_READ_FN} Callback function, must not be NULL
 * @param ctx {void*} Callback function's context parameter, must not be NULL
 */
ACL_API void acl_aio_del_connect_hook(ACL_ASTREAM *astream,
	ACL_AIO_CONNECT_FN callback, void *ctx);

/**
 * Clear all additional read callback functions.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 */
ACL_API void acl_aio_clean_read_hooks(ACL_ASTREAM *astream);

/**
 * Clear all additional write callback functions.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 */
ACL_API void acl_aio_clean_write_hooks(ACL_ASTREAM *astream);

/**
 * Clear all additional close callback functions.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 */
ACL_API void acl_aio_clean_close_hooks(ACL_ASTREAM *astream);

/**
 * Clear all additional timeout callback functions.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 */
ACL_API void acl_aio_clean_timeo_hooks(ACL_ASTREAM *astream);

/**
* Clear all additional connection success callback functions.
* @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
*/
ACL_API void acl_aio_clean_connect_hooks(ACL_ASTREAM *astream);

/**
 * Clear all additional callback functions.
 * @param astream {ACL_ASTREAM*} Asynchronous stream, must not be NULL
 */
ACL_API void acl_aio_clean_hooks(ACL_ASTREAM *astream);

/**
 * Configure asynchronous stream.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @param name {int} First parameter name
 * @param ... Parameter list format: ACL_AIO_CTL_XXX, xxx, the
 *  last parameter name
 *  is ACL_AIO_CTL_END
 */
ACL_API void acl_aio_ctl(ACL_ASTREAM *astream, int name, ...);
#define ACL_AIO_CTL_END                 0   /**< Control end flag */
#define ACL_AIO_CTL_ACCEPT_FN           1   /**< Set accept
						 *   connection callback
						 *   function */
#define ACL_AIO_CTL_LISTEN_FN           2   /**< Set listen
						 *   connection callback
						 *   function */
#define ACL_AIO_CTL_CTX                 3   /**< Set application context */
#define ACL_AIO_CTL_TIMEOUT             4   /**< Set timeout time */
#define	ACL_AIO_CTL_LINE_LENGTH         5   /**< Set maximum
						 *   line length for reading
						 *   data */
#define ACL_AIO_CTL_STREAM              10  /**< Set ACL_VSTREAM object */
#define ACL_AIO_CTL_READ_NESTED         11  /**< Set read nested level */
#define ACL_AIO_CTL_WRITE_NESTED        12  /**< Set write nested level */
#define ACL_AIO_CTL_KEEP_READ            13  /**< Set keep read flag */
#define	ACL_AIO_CTL_READ_HOOK_ADD       14  /**< Add read hook callback */
#define	ACL_AIO_CTL_READ_HOOK_DEL       15  /**< Delete read hook callback */
#define	ACL_AIO_CTL_WRITE_HOOK_ADD      16  /**< Add write hook callback */
#define	ACL_AIO_CTL_WRITE_HOOK_DEL      17  /**< Delete write hook callback */
#define	ACL_AIO_CTL_CLOSE_HOOK_ADD      18  /**< Add close hook callback */
#define	ACL_AIO_CTL_CLOSE_HOOK_DEL      19  /**< Delete close hook callback */
#define	ACL_AIO_CTL_TIMEO_HOOK_ADD      20  /**< Add timeout hook callback */
#define	ACL_AIO_CTL_TIMEO_HOOK_DEL      21  /**< Delete timeout hook callback */
#define	ACL_AIO_CTL_CONNECT_HOOK_ADD    22  /**< Add connect hook callback */
#define	ACL_AIO_CTL_CONNECT_HOOK_DEL    23  /**< Delete connect hook callback */

/**
 * Get the underlying ACL_VSTREAM object.
 * @param astream {ACL_ASTREAM*} Asynchronous IO stream
 * @return {ACL_VSTREAM*} Stream object
 */
ACL_API ACL_VSTREAM *acl_aio_vstream(ACL_ASTREAM *astream);

/*-------------------- Asynchronous read interface -------------------------*/

/**
 * Asynchronously read a line, successfully read a line of
 * data or timeout will call the user's registered function:
 * notify_fn
 * @param astream {ACL_ASTREAM*} Stream to be monitored, which
 *  can trigger read/write events and timeout callbacks
 *  registered by the user.
 * Note: This function is an asynchronous read function.
 *     If the maximum line length limit is set through
 *     acl_aio_stream_set_line_length, when the received data
 *     exceeds this length, to avoid buffer overflow, the internal
 *     processing will directly call the callback function when the
 *     buffer reaches this length, and will not wait for the line
 *     terminator to directly pass the data to the user's registered
 *     callback function
 */
ACL_API void acl_aio_gets(ACL_ASTREAM *astream);

/**
 * Asynchronously read a line, successfully read a line of data
 * or timeout will call the user's registered function: notify_fn,
 * unlike acl_aio_gets function, the only difference is that the
 * returned data does not contain "\r\n" or "\n", when a blank
 * line is read, then dlen == 0.
 * @param astream {ACL_ASTREAM*} Stream to be monitored, which
 *  can trigger read/write events and timeout callbacks
 *  registered by the user.
 * Note: This function is an asynchronous read function.
 *     When the data exceeds the limit, to avoid buffer
 *     overflow, the internal processing will directly call
 *     the callback function when the buffer reaches this
 *     length, and will not wait for the line terminator to
 *     directly pass the data to the user's registered
 *     callback function
 */
ACL_API void acl_aio_gets_nonl(ACL_ASTREAM *astream);

/**
 * Asynchronously read data, the read data format is
 * determined by the user's needs.
 * @param astream {ACL_ASTREAM*} Stream to be monitored. When
 *  readable data is available, it has already read a certain
 *  length of data and then triggers the event notification
 *  callback
 * Note: This function is an asynchronous read function.
 */
ACL_API void acl_aio_read(ACL_ASTREAM *astream);

/**
 * Asynchronously read data of the required length, when the
 * data length reaches the required length or timeout occurs,
 * the event notification callback will be triggered
 * @param astream {ACL_ASTREAM*} Stream to be monitored. When
 *  readable data is available, it has already read the
 *  required length of data and then triggers the event
 *  notification callback
 * @param count {int} Required data length, must be > 0.
 * Note: This function is an asynchronous read function.
 */
ACL_API void acl_aio_readn(ACL_ASTREAM *astream, int count);

/**
 * Synchronously read a line.
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @return {ACL_VSTRING*} If a line has been read, returns a
 *  non-empty user-allocated ACL_VSTRING object, and should
 *  call ACL_VSTRING_RESET(s) to reset it after use; if no
 *  line has been read, returns NULL
 */
ACL_API ACL_VSTRING *acl_aio_gets_peek(ACL_ASTREAM *astream);

/**
 * Synchronously read a line (excluding \n or \r\n)
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @return {ACL_VSTRING*} If a line has been read, returns a
 *  non-empty user-allocated ACL_VSTRING object, and should
 *  call ACL_VSTRING_RESET(s) to reset it after use, if a
 *  blank line is read, the returned ACL_VSTRING's internal
 *  data length (ACL_VSTRING_LEN value) should be 0;
 *  if no line has been read, returns NULL
 */
ACL_API ACL_VSTRING *acl_aio_gets_nonl_peek(ACL_ASTREAM *astream);

/**
 * Synchronously read data from the asynchronous stream, if
 * data is available, returns a non-empty buffer, otherwise
 * returns NULL
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @param count {int*} After return, will store the actual
 *  number of bytes read this time, the value is always >= 0
 * @return {ACL_VSTRING*} If data is available, returns a
 *  non-empty buffer (if the user uses this buffer, need to
 *  call ACL_VSTRING_RESET(s) to reset this buffer), otherwise
 *  returns NULL
 */
ACL_API ACL_VSTRING *acl_aio_read_peek(ACL_ASTREAM *astream, int *count);

/**
 * Synchronously read data of the required length from the
 * asynchronous stream, if the required length is reached,
 * returns a buffer
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @param count {int*} Required data length, after return will
 *  store the actual number of bytes read this time, the
 *  stored value is always >= 0
 * @return {ACL_VSTRING*} If the required length is reached,
 *  returns a non-empty buffer (if the user uses this buffer,
 *  need to call ACL_VSTRING_RESET(s) to reset this buffer),
 *  otherwise returns NULL
 */
ACL_API ACL_VSTRING *acl_aio_readn_peek(ACL_ASTREAM *astream, int *count);

/**
 * Set the asynchronous stream to readable state, when data
 * becomes readable, call the user's callback function
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @param can_read_fn {ACL_AIO_NOTIFY_FN} User callback function
 * @param context {void*} One of the parameters for can_read_fn
 */
ACL_API void acl_aio_enable_read(ACL_ASTREAM *astream,
	ACL_AIO_NOTIFY_FN can_read_fn, void *context);

/**
 * Check if data is readable in the asynchronous stream.
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @return {int} ACL_VSTREAM_EOF indicates the stream should
 *  be closed by the application; 0 indicates no data is
 *  readable; > 0 indicates data is readable
 */
ACL_API int acl_aio_can_read(ACL_ASTREAM *astream);

/**
 * Stop monitoring read events for an asynchronous stream.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 */
ACL_API void acl_aio_disable_read(ACL_ASTREAM *astream);

/**
 * Check if the stream is registered in the asynchronous
 * event's read event set.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @return {int} 0: No; != 0: Yes
 */
ACL_API int acl_aio_isrset(ACL_ASTREAM *astream);

/**
 * Set the maximum length limit for each line of data when
 * reading a line, the main purpose is to prevent the other
 * party from sending a line of data that is too long,
 * causing the local receive buffer memory overflow
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @param len {int} When this value > 0, it will limit the
 *  length of the read line data
 */
ACL_API void acl_aio_stream_set_line_length(ACL_ASTREAM *astream, int len);

/**
 * Get the maximum length limit set for reading a line.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @return {int}
 */
ACL_API int acl_aio_stream_get_line_length(ACL_ASTREAM *astream);

/**
 * Set whether the asynchronous stream keeps reading, the
 * default behavior is to automatically inherit the keep_read
 * flag from ACL_AIO (the default behavior is to keep
 * reading)
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @param onoff {int} 0 indicates to disable the keep read
 *  function, != 0 indicates to enable the keep read function
 */
ACL_API void acl_aio_stream_set_keep_read(ACL_ASTREAM *astream, int onoff);

/**
 * Check if the asynchronous stream has the keep read function enabled.
 * @return {int} 0 indicates the keep read function is
 *  disabled, != 0 indicates the keep read function is
 *  enabled
 */
ACL_API int acl_aio_stream_get_keep_read(ACL_ASTREAM *astream);

/*------------------ Asynchronous write interface ----------------*/

/**
 * Asynchronously write data, when data is written or write
 * succeeds, the event notification callback will be
 * triggered
 * @param astream {ACL_ASTREAM*} Stream to be monitored.
 * @param data {const char*} Data pointer address
 * @param dlen {int} data length
 */
ACL_API void acl_aio_writen(ACL_ASTREAM *astream, const char *data, int dlen);

/**
 * Asynchronously write data, when data is written or write
 * succeeds, the event notification callback will be
 * triggered, using system
 * writev
 * @param astream {ACL_ASTREAM*} Stream to be monitored.
 * @param vector {const struct iovec*} Data array pointer
 * @param count {int} Length of vector array
 */
ACL_API void acl_aio_writev(ACL_ASTREAM *astream,
		const struct iovec *vector, int count);

/**
 * Asynchronously write data in format, when data is written
 * or write succeeds, the event notification callback will be
 * triggered
 * @param astream {ACL_ASTREAM*} Stream to be monitored
 * @param fmt {const char*} Format string
 * @param ap {va_list} Format string's parameter list
 */
ACL_API void acl_aio_vfprintf(ACL_ASTREAM *astream, const char *fmt, va_list ap);

/**
 * Asynchronously write data in format, when data is written
 * or write succeeds, the event notification callback will be
 * triggered
 * @param astream {ACL_ASTREAM*} Stream to be monitored
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(2, 3) acl_aio_fprintf(ACL_ASTREAM *astream, const char *fmt, ...);

/**
 * Get the current pending send data length.
 * @param astream {ACL_ASTREAM*}
 * @return {size_tt}
 */
ACL_API size_t acl_aio_send_pending(ACL_ASTREAM *astream);

/**
 * Set the asynchronous stream to writable state, when data
 * becomes writable, call the user's callback function
 * @param astream {ACL_ASTREM*} Asynchronous stream
 * @param can_write_fn {ACL_AIO_NOTIFY_FN} User callback function
 * @param context {void*} One of the parameters for can_write_fn
 */
ACL_API void acl_aio_enable_write(ACL_ASTREAM *astream,
	ACL_AIO_NOTIFY_FN can_write_fn, void *context);

/**
 * Stop monitoring write events for an asynchronous stream.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 */
ACL_API void acl_aio_disable_write(ACL_ASTREAM *astream);

/**
 * Check if the stream is registered in the asynchronous
 * event's write event set.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @return {int} 0: No; != 0: Yes
 */
ACL_API int acl_aio_iswset(ACL_ASTREAM *astream);

/*------------- Asynchronous connection interface ---------------------------*/

/**
 * Asynchronously accept a client connection, when each client
 * connection is accepted, call the user
 * @param astream {ACL_ASTREAM*} Stream in listening state
 */
ACL_API void acl_aio_accept(ACL_ASTREAM *astream);

/**
 * Asynchronous listen, when a connection is established or
 * connection timeout occurs, the event notification callback
 * will be triggered, when connection timeout, the user can
 * register their own callback function accept() callback
 * function.
 * @param astream {ACL_ASTREAM*} Stream in listening state
 */
ACL_API void acl_aio_listen(ACL_ASTREAM *astream);

/*------------------ Asynchronous connection interface ----------------------*/

/**
 * Asynchronously connect to a remote server, when connection
 * succeeds or connection timeout occurs, the event
 * notification callback will be triggered.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param addr {const char*} Remote server address, format:
 *  ip:port, e.g.: 192.168.0.1:80
 * @param local {const char*} Local bound IP address, if this
 *  parameter is not empty, the first letter is @ indicates
 *  binding to the specified IP address, if it is #
 *  indicates binding to the specified network interface
 * @param timeout {int} Connection timeout time value, unit is seconds
 * @return {ACL_ASTREAM*} Returns whether the asynchronous
 *  connection object was created successfully
 */
ACL_API ACL_ASTREAM *acl_aio_connect2(ACL_AIO *aio, const char *addr,
		const char *local, int timeout);
ACL_API ACL_ASTREAM *acl_aio_connect(ACL_AIO *aio, const char *addr, int timeout);

/**
 * Asynchronously connect to a remote server using domain name
 * address. Unlike acl_aio_connect function, this function
 * needs to resolve the domain name address through
 * acl_aio_set_dns to set the DNS server address
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param addr {const char*} Domain name address, format:
 *  domain:port, e.g.: www.sina.com:80
 * @param local {const char*} Local bound IP address, if this
 *  parameter is not empty, the first letter is @ indicates
 *  binding to the specified IP address, if it is #
 *  indicates binding to the specified network interface
 * @param timeout {int} Connection timeout time value, unit is seconds
 * @param callback {ACL_AIO_CONNECT_ADDR_FN}
 * @param context {void*} Parameter passed to the callback function
 * @return {int} If 0 indicates the asynchronous connection
 *  process has started, the asynchronous connection process,
 *  if < 0 indicates an error occurred, the error may be that
 *  the DNS server address is not set in ACL_AIO object or
 *  the user has not set it through acl_aio_set_dns
 */
ACL_API int acl_aio_connect_addr2(ACL_AIO *aio, const char *addr,
		const char *local, int timeout,
		ACL_AIO_CONNECT_ADDR_FN callback, void *context);
ACL_API int acl_aio_connect_addr(ACL_AIO *aio, const char *addr, int timeout,
		ACL_AIO_CONNECT_ADDR_FN callback, void *context);

/*--------------- Common asynchronous stream interface ----------------------*/

/**
 * Stop monitoring read and write events for an asynchronous stream.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 */
ACL_API void acl_aio_disable_readwrite(ACL_ASTREAM *astream);

/**
 * Check if the stream is registered in the asynchronous event's read and write event sets.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @return {int} 0: No; != 0: Yes
 */
ACL_API int acl_aio_isset(ACL_ASTREAM *astream);

/**
 * Get the current asynchronous stream's reference count value.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 * @return {int} >=0 asynchronous stream's reference count value
 */
ACL_API int acl_aio_refer_value(ACL_ASTREAM * astream);

/**
 * Increase the asynchronous stream's reference count value by 1.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 */
ACL_API void acl_aio_refer(ACL_ASTREAM *astream);

/**
 * Decrease the asynchronous stream's reference count value by 1.
 * @param astream {ACL_ASTREAM*} Asynchronous stream
 */
ACL_API void acl_aio_unrefer(ACL_ASTREAM *astream);

/**
 * Request a timer event, this function is a simple wrapper for acl_event_request_timer
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} Timer event callback function.
 * @param context {void*} One of the parameters for timer_fn.
 * @param idle_limit {acl_int64} Timer event timeout time, unit is microseconds.
 * @param keep {int} Whether to repeat the timer event
 * @return {acl_int64} Remaining time, unit is microseconds.
 */
ACL_API acl_int64 acl_aio_request_timer(ACL_AIO *aio,
		ACL_EVENT_NOTIFY_TIME timer_fn, void *context,
		acl_int64 idle_limit, int keep);

/**
 * Cancel a timer event, this function is a simple wrapper for acl_event_cancel_timer.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} Timer event callback function.
 * @param context {void*} One of the parameters for timer_fn.
 * @return {acl_int64} Remaining time, unit is microseconds.
 */
ACL_API acl_int64 acl_aio_cancel_timer(ACL_AIO *aio,
		ACL_EVENT_NOTIFY_TIME timer_fn, void *context);

/**
 * Keep or cancel a timer event created by acl_aio_request_timer
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} Timer event callback function.
 * @param context {void*} One of the parameters for timer_fn.
 * @param onoff {int} Whether to keep the timer event
 */
ACL_API void acl_aio_keep_timer(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME timer_fn,
		void *context, int onoff);

/**
 * Check if a timer event is kept.
 * @param aio {ACL_AIO*} Asynchronous framework object
 * @param timer_fn {ACL_EVENT_NOTIFY_TIME} Timer event callback function.
 * @param context {void*} One of the parameters for timer_fn.
 * @return {int} !0 indicates the timer event is kept, otherwise indicates it is not kept
 */
ACL_API int acl_aio_timer_ifkeep(ACL_AIO *aio, ACL_EVENT_NOTIFY_TIME timer_fn,
		void *context);

#ifdef	__cplusplus
}
#endif

#endif
