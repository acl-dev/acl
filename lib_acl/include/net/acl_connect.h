#ifndef	ACL_CONNECT_INCLUDE_H
#define	ACL_CONNECT_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

#ifdef  ACL_UNIX
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif  

/* in acl_sane_connect.c */
/**
 * 远程连接服务器
 * @param sock {ACL_SOCKET} 套接字，在UNIX平台下还可以是域套接字
 * @param sa {const struct sockaddr*} 服务器监听地址
 * @param len {socklen_t} sa 的地址长度
 * @return {int} 0: 连接成功; -1: 连接失败
 */
ACL_API int acl_sane_connect(ACL_SOCKET sock, const struct sockaddr * sa,
		socklen_t len);

/* in acl_timed_connect.c */

/**
 * 带超时时间地远程连接服务器
 * @param fd {ACL_SOCKET} 套接字，在UNIX平台下还可以是域套接字
 * @param sa {const struct sockaddr*} 服务器监听地址
 * @param len {socklen_t} sa 的地址长度
 * @param timeout {int} 连接超时时间
 * @return {int} 0: 连接成功; -1: 连接失败
 */
ACL_API int acl_timed_connect(ACL_SOCKET fd, const struct sockaddr * sa,
		socklen_t len, int timeout);

/* in acl_inet_connect.c */

/**
 * 远程连接网络服务器地址
 * @param addr {const char*} 远程服务器的监听地址，如：192.168.0.1:80, 如果需要绑定本地
 *  的地址，则格式为: {local_ip}@{remote_addr}, 如: 60.28.250.199@www.sina.com:80
 * @param block_mode {int} 阻塞模式还是非阻塞模式, ACL_BLOCKING 或 ACL_NON_BLOCKING
 * @param timeout {int} 连接超时时间，如果 block_mode 为 ACL_NON_BLOCKING 则该值将被忽略
 * @return {ACL_SOCKET} 如果返回 ACL_SOCKET_INVALID 表示连接失败 
 */
ACL_API ACL_SOCKET acl_inet_connect(const char *addr, int block_mode, int timeout);

/**
 * 远程连接网络服务器地址
 * @param addr {const char*} 远程服务器的监听地址，如：192.168.0.1:80，
 *  当本机有多个网卡地址且想通过某个指定网卡连接服务器时的地址格式：
 local_ip@remote_ip:remote_port，如：192.168.1.1@211.150.111.12:80
 * @param block_mode {int} 阻塞模式还是非阻塞模式, ACL_BLOCKING 或 ACL_NON_BLOCKING
 * @param timeout {int} 连接超时时间，如果 block_mode 为 ACL_NON_BLOCKING 则该值将被忽略
 * @param h_error {int*} 当连接失败时存储失败原因错误号
 * @return {ACL_SOCKET} 如果返回 ACL_SOCKET_INVALID 表示连接失败 
 */
ACL_API ACL_SOCKET acl_inet_connect_ex(const char *addr, int block_mode,
			int timeout, int *h_error);

#ifdef	ACL_UNIX

/* in acl_unix_connect.c */

/**
 * 连接监听域套接字服务器
 * @param addr {const char*} 服务器监听的域套接字全路径, 如: /tmp/test.sock
 * @param block_mode {int} 阻塞模式还是非阻塞模式, ACL_BLOCKING 或 ACL_NON_BLOCKING
 * @param timeout {int} 连接超时时间，如果 block_mode 为 ACL_NON_BLOCKING 则该值将被忽略
 * @return {ACL_SOCKET} 如果返回 ACL_SOCKET_INVALID 表示连接失败 
 */
ACL_API ACL_SOCKET acl_unix_connect(const char *addr, int block_mode, int timeout);

/* in acl_stream_connect.c */
#ifdef SUNOS5
ACL_API int acl_stream_connect(const char *path, int block_mode, int unused_timeout);
#endif

#endif

#ifdef	__cplusplus
}
#endif

#endif
