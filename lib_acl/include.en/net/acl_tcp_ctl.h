#ifndef ACL_TCP_CTL_INCLUDE_H
#define ACL_TCP_CTL_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

#define ACL_SOCKET_RBUF_SIZE	204800  /**< Default receive buffer size */
#define ACL_SOCKET_WBUF_SIZE	204800  /**< Default send buffer size */

/**
 * Set TCP socket's receive buffer size.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param size {int} Receive buffer size to set
 */
ACL_API void acl_tcp_set_rcvbuf(ACL_SOCKET fd, int size);

/**
 * Set TCP socket's send buffer size.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param size {int} Send buffer size to set
 */
ACL_API void acl_tcp_set_sndbuf(ACL_SOCKET fd, int size);

/**
 * Get TCP socket's receive buffer size.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} Receive buffer size
 */
ACL_API int  acl_tcp_get_rcvbuf(ACL_SOCKET fd);

/**
 * Get TCP socket's send buffer size.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} Send buffer size
 */
ACL_API int  acl_tcp_get_sndbuf(ACL_SOCKET fd);

/**
 * Enable TCP socket's nodelay option.
 * @param fd {ACL_SOCKET} Socket descriptor
 */
ACL_API void acl_tcp_set_nodelay(ACL_SOCKET fd);

/**
 * Set TCP socket's nodelay option.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param onoff {int} 1 indicates enable, 0 indicates disable
 */
ACL_API void acl_tcp_nodelay(ACL_SOCKET fd, int onoff);

/**
 * Check whether TCP socket has enabled nodelay option.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} 1 indicates enabled, 0 indicates disabled
 */
ACL_API int acl_get_tcp_nodelay(ACL_SOCKET fd);

/**
 * Set TCP socket's SO_LINGER option.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param onoff {int} Whether to enable SO_LINGER option
 * @param timeout {int} When SO_LINGER is enabled, timeout for
 *  TIME_WAIT state, unit is seconds
 */
ACL_API void acl_tcp_so_linger(ACL_SOCKET fd, int onoff, int timeout);

/**
 * Get TCP socket's linger value.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} Return -1 indicates SO_LINGER option not set or
 *  internal error; >= 0 indicates SO_LINGER option is set and the
 *  value indicates the timeout (seconds) for TCP connection to
 *  maintain TIME_WAIT state after socket is closed
 */
ACL_API int acl_get_tcp_solinger(ACL_SOCKET fd);

/**
 * Set listening socket's defer accept function, so that client
 * connections are only accepted when data arrives. Note:
 * Currently this function only supports Linux.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param timeout {int} If client connection does not send data
 *  within the specified timeout, the connection will also be
 *  accepted and responded to
 */
ACL_API void acl_tcp_defer_accept(ACL_SOCKET fd, int timeout);

/**
 * Set listening socket's fast open TCP connection function
 * (requires kernel support).
 * @param fd {ACL_SOCKET}
 * @param on {int} Non-zero enables this function, zero disables this function
 */
ACL_API void acl_tcp_fastopen(ACL_SOCKET fd, int on);

#ifdef __cplusplus
}
#endif

#endif
