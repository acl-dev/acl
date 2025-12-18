#ifndef	ACL_SANE_SOCKET_INCLUDE_H
#define	ACL_SANE_SOCKET_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <arpa/inet.h>
#endif

/**
 * Get socket's connected peer's remote address, address format: IP:PORT
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param buf {char*} Buffer to store address, must not be NULL
 * @param bsize {size_t} buf space size
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_getpeername(ACL_SOCKET fd, char *buf, size_t bsize);

/**
 * Get socket's bound local address, address format: IP:PORT
 * @param fd {ACL_SOCKET} Socket descriptor
 * @param buf {char*} Buffer to store address, must not be NULL
 * @param bsize {size_t} buf space size
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_getsockname(ACL_SOCKET fd, char *buf, size_t bsize);

/**
 * Get socket type.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} -1 indicates error or invalid socket; if TCP,
 *  returns SOCK_STREAM, if UDP, returns SOCK_DGRAM
 */
ACL_API int acl_getsocktype(ACL_SOCKET fd);

/**
 * Get socket address family type, can be used for both listening
 * sockets and connected sockets.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} -1: indicates error or invalid socket; >= 0
 *  indicates success, socket family type, return value: AF_INET,
 *  AF_INET6, or AF_UNIX (only on UNIX platforms)
 */
ACL_API int acl_getsockfamily(ACL_SOCKET fd);

/**
 * Check socket, whether it is a listening socket or connected socket.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} Return -1 indicates invalid socket; 1 is
 *  listening socket; 0 is non-listening socket
 */
ACL_API int acl_check_socket(ACL_SOCKET fd);

/**
 * Check whether socket is a listening socket.
 * @param fd {ACL_SOCKET} Socket descriptor
 * @return {int} Return value 0 indicates non-listening socket; non-zero
 *  indicates listening socket
 */
ACL_API int acl_is_listening_socket(ACL_SOCKET fd);

/**
 * Bind interface to specified network interface, should be
 * called before calling connect, bind system APIs.
 * @param sock {ACL_SOCKET}
 * @param iface {const char*} Interface name
 * @return {int} Return 0 indicates binding successful; -1 indicates failure
 */
ACL_API int acl_bind_interface(ACL_SOCKET sock, const char *iface);

#ifdef	__cplusplus
}
#endif

#endif
