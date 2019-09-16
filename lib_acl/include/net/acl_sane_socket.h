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
 * 取得套接字连接对方的网络地址, 地址格式为: IP:PORT
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {char*} 存储地址的缓冲区，不能为空
 * @param bsize {size_t} buf 空间大小
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_getpeername(ACL_SOCKET fd, char *buf, size_t bsize);

/**
 * 取得套接字连接本地的网络地址, 地址格式为: IP:PORT
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {char*} 存储地址的缓冲区，不能为空
 * @param bsize {size_t} buf 空间大小
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_getsockname(ACL_SOCKET fd, char *buf, size_t bsize);

/**
 * 取得套接字的类型：是网络套接字还是域套接字
 * @param fd {ACL_SOCKET} 网络套接字
 * @return {int} -1: 表示出错或输入非法或非套接字; >= 0 表示成功获得套接字
 *  类型，返回值有 AF_INET、AF_INET6 或 AF_UNIX(仅限 UNIX 平台)
 */
ACL_API int acl_getsocktype(ACL_SOCKET fd);

/**
 * 检查套接字：是监听套接字还是网络套接字
 * @param fd {ACL_SOCKET} 套接字句柄
 * @return {int} 返回 -1 表示该句柄非套接字，1 为监听套接字，0 为非监听套接字
 */
ACL_API int acl_check_socket(ACL_SOCKET fd);

/**
 * 判断套接字是否为监听套接字
 * @param fd {ACL_SOCKET} 套接字句柄
 * @return {int} 返回值 0 表示非监听套接字，非 0 表示为监听套接字
 */
ACL_API int acl_is_listening_socket(ACL_SOCKET fd);

#ifdef	__cplusplus
}
#endif

#endif
