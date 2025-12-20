#ifndef	ACL_IOSTUFF_INCLUDE_H
#define	ACL_IOSTUFF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstream.h"

#define ACL_CLOSE_ON_EXEC   1  /**< Flag bit, automatically close
				 *   opened file descriptors on exec */
#define ACL_PASS_ON_EXEC    0

#define ACL_BLOCKING        0  /**< Blocking read/write flag */
#define ACL_NON_BLOCKING    1  /**< Non-blocking read/write flag */

/**
 * Set socket to blocking or non-blocking mode.
 * @param fd {ACL_SOCKET} SOCKET descriptor
 * @param on {int} Whether to set socket to non-blocking,
 *  ACL_BLOCKING or ACL_NON_BLOCKING
 * @return {int} >= 0: success, return value > 0 indicates
 *  previous flag bit; -1: failure
 */
ACL_API int acl_non_blocking(ACL_SOCKET fd, int on);

/**
 * Check whether the given socket is in blocking mode.
 * @param fd {ACL_SOCKET}  SOCKET descriptor
 * @return {int} -1 indicates error or platform does not support;
 *  1 indicates socket is in blocking mode
 *  0 indicates socket is in non-blocking mode
 */
ACL_API int acl_is_blocking(ACL_SOCKET fd);

/**
 * Wait for write until socket is writable or timeout.
 * @param fd {ACL_SOCKET} Descriptor
 * @param timeout {int} Timeout time, unit is seconds, value meanings:
 *  > 0  : indicates timeout time in seconds
 *  == 0 : indicates no waiting, returns immediately
 *  < 0  : indicates wait until socket is readable/writable
 * @return {int} 0: writable; -1: failure or timeout
 */
ACL_API int acl_write_wait(ACL_SOCKET fd, int timeout);

/**
 * Wait for write until socket is writable or timeout.
 * @param fd {ACL_SOCKET} Descriptor
 * @param timeout {int} Timeout time, unit is milliseconds, value meanings:
 *  > 0  : indicates timeout time in milliseconds
 *  == 0 : indicates no waiting, returns immediately
 *  < 0  : indicates wait until socket is readable/writable
 * @return {int} 0: writable; -1: failure or timeout
 */
ACL_API int acl_write_wait_ms(ACL_SOCKET fd, int timeout);

/**
 * Wait for read until socket has data readable or timeout.
 * @param fd {ACL_SOCKET} Descriptor
 * @param timeout {int} Timeout time, unit is seconds, value meanings:
 *  > 0  : indicates timeout time in seconds
 *  == 0 : indicates no waiting, returns immediately
 *  < 0  : indicates wait until socket is readable/writable
 * @return {int} 0: data readable or descriptor ready; -1: failure or timeout
 */
ACL_API int acl_read_wait(ACL_SOCKET fd, int timeout);

/**
 * Wait for read until socket has data readable or timeout.
 * @param fd {ACL_SOCKET} Descriptor
 * @param timeout {int} Timeout time, unit is milliseconds, value meanings:
 *  > 0  : indicates timeout time in milliseconds
 *  == 0 : indicates no waiting, returns immediately
 *  < 0  : indicates wait until socket is readable/writable
 * @return {int} 0: data readable or descriptor ready; -1: failure or timeout
 */
ACL_API int acl_read_wait_ms(ACL_SOCKET fd, int timeout);

#if defined(ACL_LINUX) && !defined(MINGW)
/**
 * Read wait using epoll method.
 * @param fd {ACL_SOCKET} Descriptor
 * @param delay {int} Millisecond wait time
 * @return {int} Same as above
 */
ACL_API int acl_read_epoll_wait(ACL_SOCKET fd, int delay);
#endif

#if defined(ACL_UNIX)
/**
 * Read wait using poll method.
 * @param fd {ACL_SOCKET} Descriptor
 * @param delay {int} Millisecond wait time
 * @return {int} Same as above
 */
ACL_API int acl_read_poll_wait(ACL_SOCKET fd, int delay);
#endif

/**
 * Read wait using select method.
 * @param fd {ACL_SOCKET} Descriptor
 * @param delay {int} Millisecond wait time
 * @return {int} Same as above
 */
ACL_API int acl_read_select_wait(ACL_SOCKET fd, int delay);

/**
 * Millisecond sleep.
 * @param delay {unsigned} Millisecond value
 */
ACL_API void acl_doze(unsigned delay);

/**
* Check whether a descriptor is readable.
* @param fd {ACL_SOCKET} Descriptor
* @return {int} 0: not readable; -1: error; 1: readable
*/
ACL_API int acl_readable(ACL_SOCKET fd);

/**
 * Timed read function.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param buf {void*} Storage buffer, must not be NULL
 * @param len {unsigned} buf storage size
 * @param timeout {int} Timeout time, unit is seconds, value meanings:
 *  > 0  : indicates timeout time in seconds
 *  == 0 : indicates no waiting, returns immediately
 *  < 0  : indicates wait until socket is readable/writable
 * @return {int} > 0 bytes read; -1: error
*/
ACL_API int acl_timed_read(ACL_SOCKET fd, void *buf, unsigned len,
	int timeout, void *unused_context);

/**
 * Timed write function.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param buf {void*} Data storage buffer, must not be NULL
 * @param len {unsigned} Data length size
 * @param timeout {int} Timeout time, unit is seconds, value meanings:
 *  > 0  : indicates timeout time in seconds
 *  == 0 : indicates no waiting, returns immediately
 *  < 0  : indicates wait until socket is readable/writable
 * @return {int} > 0 successfully written bytes; -1: error
 */
ACL_API int acl_timed_write(ACL_SOCKET fd, void *buf, unsigned len,
	int timeout, void *unused_context);

/**
 * Loop write data until all written, error, or timeout.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param buf {void*} Data storage buffer, must not be NULL
 * @param len {unsigned} Data length size
 * @param timeout {int} Timeout time, unit is seconds
 * @return {int} Successfully written length
 */
ACL_API int acl_write_buf(ACL_SOCKET fd, const char *buf, int len, int timeout);

/**
 * Probe socket system buffer readable data length.
 * @param fd {ACL_SOCKET} Descriptor
 * @return {int} System buffer readable data length
 */
ACL_API int acl_peekfd(ACL_SOCKET fd);

/**
 * Create a pipe.
 * @param fds {ACL_FILE_HANDLE [2]} Storage array
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_pipe(ACL_FILE_HANDLE fds[2]);

/**
 * Close pipe handles.
 * @param fds {ACL_FILE_HANDLE[2]} Pipe handles
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_pipe_close(ACL_FILE_HANDLE fds[2]);

/**
 * Create a duplex pipe.
 * @param fds {ACL_FILE_HANDLE[2]} Storage array for pipe endpoints,
 *  must not be NULL
 * @return 0: ok; -1: error
*/
ACL_API int acl_duplex_pipe(ACL_FILE_HANDLE fds[2]);

#ifdef	ACL_UNIX
/**
 * Set file descriptor close-on-exec flag, automatically close on exec call.
 * @param fd {int} File descriptor
 * @param on {int} ACL_CLOSE_ON_EXEC or 0
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_close_on_exec(int fd, int on);

/**
 * Close all file descriptors starting from a certain file descriptor number.
 * @param lowfd {int} Minimum descriptor number to close
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_closefrom(int lowfd);

/**
 * Set the maximum number of file descriptors the current process can open.
 * @param limit {int} Maximum value to set
 * @return {int} >=0: ok; -1: error
 */
ACL_API int acl_open_limit(int limit);

/**
 * Check whether a given file descriptor is a socket.
 * @param fd {int} File descriptor
 * @return {int} != 0: yes; 0: no
 */
ACL_API int acl_issock(int fd);
#endif

ACL_API void acl_set_delay_slice(int n);

#if defined(_WIN32) || defined(_WIN64)
typedef int (WINAPI *acl_select_fn)(int, fd_set*, fd_set*,
	fd_set*, const struct timeval*);
# if(_WIN32_WINNT >= 0x0600)
#  define ACL_HAS_POLL
typedef int (WINAPI *acl_poll_fn)(struct pollfd*, unsigned long, int);
ACL_API void acl_set_poll(acl_poll_fn fn);
# endif

#else
#include <poll.h>
# define ACL_HAS_POLL

typedef int (*acl_select_fn)(int, fd_set*, fd_set*, fd_set*, struct timeval*);
typedef int (*acl_poll_fn)(struct pollfd*, nfds_t, int);
ACL_API void acl_set_poll(acl_poll_fn fn);
#endif

ACL_API void acl_set_select(acl_select_fn fn);

#ifdef	__cplusplus
}
#endif

#endif
