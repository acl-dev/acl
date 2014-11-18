#ifndef ACL_LISTEN_INCLUDE_H
#define ACL_LISTEN_INCLUDE_H
#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef	ACL_UNIX
#include <sys/socket.h>
#endif

/**
 * 监听套接字接收外来客户端连接
 * @param sock {ACL_SOCKET} 监听套接字
 * @param sa {struct sockaddr*} 存储客户端的网络地址，不能为空
 * @param len {socklen_t*} sa 内存空间大小，不能为空
 * @return {ACL_SOCKET} 如果返回 ACL_SOCKET_INVALID 表示接收失败
 */
ACL_API ACL_SOCKET acl_sane_accept(ACL_SOCKET sock, struct sockaddr * sa, socklen_t *len);

/**
 * 方便通用的监听套接字的函数，用来接收客户端连接
 * @param sock {ACL_SOCKET} 监听套接字
 * @param buf {char*} 当成功接收一个客户端连接后，如果该 buf 非空则存放客户端地址，格式：
 *  ip:port (针对 TCP 套接口), file_path (针对 UNIX 域套接口)
 * @param size {size_t} buf 缓冲区大小
 * @param sock_type {int*} 该指针非空时，用来存放客户端连接的 SOCKET 类型，AF_INET/AF_UNIX
 * @return {ACL_SOCKET} 客户端连接句柄, 返回值 != ACL_SOCKET_INVALID 则表明成功收到一个客户端连接
 */
ACL_API ACL_SOCKET acl_accept(ACL_SOCKET sock, char *buf, size_t size, int* sock_type);

/* in acl_inet_listen.c */

/**
 * 监听某个网络地址
 * @param addr {const char*} 网络地址, 格式如：127.0.0.1:8080
 * @param backlog {int} 监听套接字系统接收区的队列大小
 * @param block_mode {int} 阻塞模式还是非阻塞模式, ACL_BLOCKING 或 ACL_NON_BLOCKING
 * @return {ACL_SOCKET} 返回监听套接字，如果为 ACL_SOCKET_INVALID 表示无法监听该网络地址
 */
ACL_API ACL_SOCKET acl_inet_listen(const char *addr, int backlog, int block_mode);

/**
 * 接收外来客户端网络连接
 * @param listen_fd {ACL_SOCKET} 监听套接字
 * @return {ACL_SOCKET} 客户端连接，如果返回 ACL_SOCKET_INVALID 表示接收客户端连接出错
 */
ACL_API ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd);

/**
 * 接收外来客户端网络连接
 * @param listen_fd {ACL_SOCKET} 监听套接字
 * @param ipbuf {char*} 如果该指针不为空且接收客户端连接成功，则其存储客户端的网络地址
 * @param size {size_t} 如果 ipbuf 不为空则表示 ipbuf 的内存空间大小
 * @return {ACL_SOCKET} 客户端连接，如果返回 ACL_SOCKET_INVALID 表示接收客户端连接出错
 */
ACL_API ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf, size_t size);

#ifdef	ACL_UNIX

/* in acl_unix_listen.c */
/**
 * 监听域套接字
 * @param addr {const char*} 监听域套接字时所用的全路径
 * @param backlog {int} 监听队列大小
 * @param block_mode {int} 阻塞模式还是非阻塞模式, ACL_BLOCKING 或 ACL_NON_BLOCKING
 * @return {ACL_SOCKET} 返回监听套接字，如果为 ACL_SOCKET_INVALID 表示无法监听该网络地址
 */
ACL_API ACL_SOCKET acl_unix_listen(const char *addr, int backlog, int block_mode);

/**
 * 从域套接字上接收一个客户端连接
 * @param listen_fd {ACL_SOCKET} 监听套接字
 * @return {ACL_SOCKET} 客户端连接，如果返回 ACL_SOCKET_INVALID 表示接收客户端连接出错
 */
ACL_API ACL_SOCKET acl_unix_accept(ACL_SOCKET fd);

/* in acl_fifo_listen.c */

ACL_API int acl_fifo_listen(const char *path, int permissions, int block_mode);

#endif

#ifdef __cplusplus
}
#endif
#endif

