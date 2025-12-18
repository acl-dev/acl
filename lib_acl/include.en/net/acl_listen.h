#ifndef ACL_LISTEN_INCLUDE_H
#define ACL_LISTEN_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netdb.h>
#endif

#define ACL_INET_FLAG_NONE		0
#define ACL_INET_FLAG_NBLOCK		1
#define ACL_INET_FLAG_REUSEPORT		(1 << 1)
#define ACL_INET_FLAG_FASTOPEN		(1 << 2)
#define ACL_INET_FLAG_EXCLUSIVE		(1 << 3)
#define	ACL_INET_FLAG_MULTILOOP_ON	(1 << 4)

/**
 * Accept client connection on listening socket.
 * @param sock {ACL_SOCKET} Listening socket
 * @param sa {struct sockaddr*} Storage for client's remote
 *  address, must not be NULL
 * @param len {socklen_t*} sa memory space size, must not be NULL
 * @return {ACL_SOCKET} Valid socket, ACL_SOCKET_INVALID indicates accept failed
 */
ACL_API ACL_SOCKET acl_sane_accept(ACL_SOCKET sock, struct sockaddr * sa,
		socklen_t *len);

/**
 * Common function for accepting client connections on listening socket.
 * @param sock {ACL_SOCKET} Listening socket
 * @param buf {char*} If successfully accepting a client
 *  connection, buf can store client's address string format:
 *  ip:port (for TCP socket), file_path (for UNIX domain socket)
 * @param size {size_t} buf buffer size
 * @param sock_type {int*} If not NULL, stores client's SOCKET type: AF_INET/AF_UNIX
 * @return {ACL_SOCKET} Client connection socket, return value
 *  != ACL_SOCKET_INVALID indicates successfully accepted a
 *  client connection
 */
ACL_API ACL_SOCKET acl_accept(ACL_SOCKET sock, char *buf, size_t size,
		int* sock_type);

/* in acl_inet_listen.c */

/**
 * Listen on a certain network address.
 * @param addr {const char*} Listening address, format:
 *  127.0.0.1:8080. When the address is ip:0, the system can
 *  automatically assign a port number. After success, you can
 *  call acl_getsockname to get the actual bound address
 * @param backlog {int} Listening socket system connection queue size
 * @param flag {unsigned} Listening flag bits, see ACL_INET_FLAG_XXX
 * @return {ACL_SOCKET} Returns listening socket,
 *  ACL_SOCKET_INVALID indicates cannot bind to the address
 */
ACL_API ACL_SOCKET acl_inet_listen(const char *addr, int backlog, unsigned flag);

/**
 * Accept client connection on listening socket.
 * @param listen_fd {ACL_SOCKET} Listening socket
 * @return {ACL_SOCKET} Client connection socket,
 *  ACL_SOCKET_INVALID indicates accepting client connection
 *  failed
 */
ACL_API ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd);

/**
 * Accept client connection on listening socket.
 * @param listen_fd {ACL_SOCKET} Listening socket
 * @param ipbuf {char*} If not NULL and accepting client
 *  connection successfully, stores client's remote address
 * @param size {size_t} If ipbuf is not NULL, indicates ipbuf's memory space size
 * @return {ACL_SOCKET} Client connection socket,
 *  ACL_SOCKET_INVALID indicates accepting client connection
 *  failed
 */
ACL_API ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf,
		size_t size);

/* in acl_sane_bind.c */

/**
 * Bind address and create listening TCP/UDP socket.
 * @param res {const struct addrinfo*} Address information structure obtained
 *  from getaddrinfo
 * @param flag {unsigned int} Flag bits
 * @return {ACL_SOCKET} Returns ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_inet_bind(const struct addrinfo *res, unsigned flag);

/**
 * Bind address and create listening TCP/UDP socket.
 * @param addr {const char*} Address string obtained from getaddrinfo
 * @param flag {unsigned int} Flag bits
 * @param socktype {int} Socket type to bind: SOCK_STREAM, SOCK_DGRAM
 * @param family {int*} If binding succeeds and this address is not NULL,
 *  stores address type: AF_INET, AF_INET6, AF_UNIX
 * @return {ACL_SOCKET} Returns ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_sane_bind(const char *addr, unsigned flag,
	int socktype, int *family);

#ifdef ACL_UNIX

/**
 * Bind local UNIX domain socket in UDP mode.
 * @param addr {const char*} UNIX domain socket address path
 *  string. On Linux platform, if the first character is '@', it
 *  is treated as an abstract unix domain path on Linux.
 * @param flag {unsigned} Flag bits
 * @return {ACL_SOCKET} Returns socket, ACL_SOCKET_INVALID indicates failure
 */
ACL_API ACL_SOCKET acl_unix_dgram_bind(const char *addr, unsigned flag);
#endif

/**
 * Bind specified UDP address.
 * @param addr {const char*} UDP address string, format: IP:PORT
 *  or UNIX domain socket. When it is a UNIX domain socket, the
 *  format is: {domain_path}@udp, where @udp indicates UDP socket
 *  suffix. Internally automatically distinguishes between TCP
 *  socket and UNIX domain socket.
 *  UNIX domain socket is only supported on UNIX platforms
 * @param flag {unsigned int} Flag bits
 * @param family {int*} If binding succeeds and this address is not NULL,
 * 	stores address type: AF_INET, AF_INET6, AF_UNIX
 * @return {ACL_SOCKET} Returns ACL_SOCKET_INVALID on failure
 */
ACL_API ACL_SOCKET acl_udp_bind3(const char *addr, unsigned flag, int *family);
ACL_API ACL_SOCKET acl_udp_bind(const char *addr, unsigned flag);

#ifdef ACL_UNIX

/* in acl_unix_listen.c */
/**
 * Listen on UNIX domain socket.
 * @param addr {const char*} Full path used when creating UNIX domain socket
 * @param backlog {int} Connection queue size
 * @param flag {unsigned} Listening flag bits, see ACL_INET_FLAG_XXX
 * @return {ACL_SOCKET} Listening socket, ACL_SOCKET_INVALID
 *  indicates cannot bind to the address
 */
ACL_API ACL_SOCKET acl_unix_listen(const char *addr, int backlog, unsigned flag);

/**
 * Accept a client connection on listening socket.
 * @param fd {ACL_SOCKET} Listening socket
 * @return {ACL_SOCKET} Client connection socket, ACL_SOCKET_INVALID indicates
 * 	accepting client connection failed
 */
ACL_API ACL_SOCKET acl_unix_accept(ACL_SOCKET fd);

/* in acl_fifo_listen.c */

ACL_API int acl_fifo_listen(const char *path, int permissions, int mode);

#endif

#if defined(_WIN32) || defined(_WIN64)
# ifdef USE_WSASOCK
/* The WSAAccept prototype */
/* */typedef SOCKET (WSAAPI *acl_accept_fn)(SOCKET, struct sockaddr FAR *,
    LPINT, LPCONDITIONPROC, DWORD_PTR);
# else
/* The accept prototype */
typedef SOCKET (WINAPI *acl_accept_fn)(SOCKET, struct sockaddr*, socklen_t*);
# endif
#else
typedef int (*acl_accept_fn)(int, struct sockaddr*, socklen_t*);
#endif

ACL_API void acl_set_accept(acl_accept_fn fn);

#ifdef __cplusplus
}
#endif

#endif
