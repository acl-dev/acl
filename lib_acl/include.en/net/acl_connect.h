#ifndef	ACL_CONNECT_INCLUDE_H
#define	ACL_CONNECT_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

#ifdef  ACL_UNIX
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#define ACL_CONNECT_F_NONE		0		/* Nothing */
#define ACL_CONNECT_F_SYS_ERR		(1 << 0)	/* System errno occurred */
#define ACL_CONNECT_F_CREATE_SOCKET_ERR	(1 << 1)	/* Failed to create socket */
#define ACL_CONNECT_F_REUSE_ADDR_ERR	(1 << 2)	/* Failed to set SO_REUSEADDR */
#define ACL_CONNECT_F_BIND_IP_OK	(1 << 3)	/* Bind to local IP succeeded */
#define ACL_CONNECT_F_BIND_IP_ERR	(1 << 4)	/* Bind to local IP failed */
#define ACL_CONNECT_F_BIND_IFACE_OK	(1 << 5)	/* Bind to local interface succeeded */
#define ACL_CONNECT_F_BIND_IFACE_ERR	(1 << 6)	/* Bind to local interface failed */
#define ACL_CONNECT_F_SO_ERROR		(1 << 7)	/* Socket error via getsockopt(SO_ERROR) */
#define ACL_CONNECT_F_INPROGRESS	(1 << 8);	/* Connection is in progress */
#define ACL_CONNECT_F_WAIT_ERR		(1 << 9)	/* Waiting for connection completion failed */

/* in acl_sane_connect.c */
/**
 * Connect to a remote server in a "sane" (well-defined) way.
 * @param sock {ACL_SOCKET} Socket descriptor; on UNIX this is a regular socket
 * @param sa {const struct sockaddr*} Remote socket address
 * @param len {socklen_t} Length of the address structure sa
 * @return {int} 0 on success; -1 on error
 */
ACL_API int acl_sane_connect(ACL_SOCKET sock, const struct sockaddr * sa,
		socklen_t len);

/* in acl_timed_connect.c */

/**
 * Connect to a remote server with a timeout.
 * @param fd {ACL_SOCKET} Socket descriptor; on UNIX this is a regular socket
 * @param sa {const struct sockaddr*} Remote socket address
 * @param len {socklen_t} Length of the address structure sa
 * @param timeout {int} Connection timeout in seconds
 * @return {int} 0 on success; -1 on error
 */
ACL_API int acl_timed_connect(ACL_SOCKET fd, const struct sockaddr * sa,
		socklen_t len, int timeout);

/**
 * Connect to a remote server with a timeout (millisecond granularity).
 * @param fd {ACL_SOCKET} Socket descriptor; on UNIX this is a regular socket
 * @param sa {const struct sockaddr*} Remote socket address
 * @param len {socklen_t} Length of the address structure sa
 * @param timeout {int} Connection timeout in milliseconds
 * @param flags {unsigned*} Optional bit mask used to receive
 *  detailed error information on failure
 * @return {int} 0 on success; -1 on error
 */
ACL_API int acl_timed_connect_ms2(ACL_SOCKET fd, const struct sockaddr * sa,
		socklen_t len, int timeout, unsigned *flags);
ACL_API int acl_timed_connect_ms(ACL_SOCKET fd, const struct sockaddr * sa,
		socklen_t len, int timeout);

/* in acl_inet_connect.c */

/**
 * Connect to a remote host using an "ip|port" style address string.
 * @param addr {const char*} Remote address string, for example: "192.168.0.1|80".
 *        If you want to bind a local address as well, use the form: "remote_addr@local_ip",
 *        for example: "www.sina.com|80@60.28.250.199".
 * @param blocking {int} Blocking mode: ACL_BLOCKING or ACL_NON_BLOCKING
 * @param timeout {int} Connection timeout; when blocking is
 *  ACL_NON_BLOCKING this value is ignored
 * @return {ACL_SOCKET} Valid socket descriptor on success;
 *  ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_inet_connect(const char *addr, int blocking, int timeout);

/**
 * Connect to a remote host using an "ip|port" style address string.
 * @param addr {const char*} Remote address string, for example: "192.168.0.1|80".
 *        When multiple local addresses exist, you can specify the local address to use
 *        via the form: "remote_ip|remote_port@local_ip", for
 *        example: "211.150.111.12|80@192.168.1.1".
 * @param blocking {int} Blocking mode: ACL_BLOCKING or ACL_NON_BLOCKING
 * @param timeout {int} Connection timeout in seconds; when blocking is
 *  ACL_NON_BLOCKING this value is ignored
 * @param flags {unsigned*} Optional bit mask used to receive
 *  detailed error information on failure
 * @return {ACL_SOCKET} Valid socket descriptor on success;
 *  ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_inet_connect2(const char *addr, int blocking,
		int timeout, unsigned *flags);
#define acl_inet_connect_ex	acl_inet_connect2

/**
 * Connect to a remote host using an "ip|port" style address string, with timeout support.
 * @param addr {const char*} Remote address string, for example: "192.168.0.1|80".
 *        When multiple local addresses or interfaces exist, you can specify which one
 *        to use with either "remote_ip|remote_port@local_ip" or
 *        "remote_ip|remote_port#local_interface", e.g.:
 *          "211.150.111.12|80@192.168.1.1"
 *          "211.150.111.12|80#interface"
 * @param blocking {int} Blocking mode: ACL_BLOCKING or ACL_NON_BLOCKING
 * @param timeout {int} Connection timeout in seconds; when blocking is
 *  ACL_NON_BLOCKING this value is ignored
 * @param flags {unsigned*} Optional bit mask used to receive
 *  detailed error information on failure
 * @return {ACL_SOCKET} Valid socket descriptor on success;
 *  ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_inet_timed_connect(const char *addr, int blocking,
		int timeout, unsigned *flags);

#ifdef	ACL_UNIX

/* in acl_unix_connect.c */

/**
 * Connect to a UNIX domain socket.
 * @param addr {const char*} Full path of the UNIX domain socket, e.g.: "/tmp/test.sock".
 *        On Linux, when the first character is '@', an abstract UNIX domain socket
 *        (not bound to the filesystem namespace) will be used.
 * @param blocking {int} Blocking mode: ACL_BLOCKING or ACL_NON_BLOCKING
 * @param timeout {int} Connection timeout; when blocking is
 *  ACL_NON_BLOCKING this value is ignored
 * @return {ACL_SOCKET} Valid socket descriptor on success;
 *  ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_unix_connect(const char *addr, int blocking, int timeout);

/* in acl_stream_connect.c */
#ifdef SUNOS5
ACL_API int acl_stream_connect(const char *path, int blocking, int timeout);
#endif

#endif

#if defined(_WIN32) || defined(_WIN64)
typedef int (WINAPI *acl_connect_fn)(SOCKET, const struct sockaddr*, socklen_t);
#else
typedef int (*acl_connect_fn)(int, const struct sockaddr*, socklen_t);
#endif

ACL_API void acl_set_connect(acl_connect_fn fn);

#ifdef	__cplusplus
}
#endif

#endif
