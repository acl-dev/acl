#ifndef	ACL_SANE_SOCKET_INCLUDE_H
#define	ACL_SANE_SOCKET_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

/**
 * 取得套接字连接对方的网络地址, 地址格式为: IP:PORT
 * @param sockfd {ACL_SOCKET} 网络套接字
 * @param buf {char*} 存储地址的缓冲区，不能为空
 * @param bsize {size_t} buf 空间大小
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_getpeername(ACL_SOCKET sockfd, char *buf, size_t bsize);

/**
 * 取得套接字连接本地的网络地址, 地址格式为: IP:PORT
 * @param sockfd {ACL_SOCKET} 网络套接字
 * @param buf {char*} 存储地址的缓冲区，不能为空
 * @param bsize {size_t} buf 空间大小
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_getsockname(ACL_SOCKET sockfd, char *buf, size_t bsize);

/**
 * 取得套接字的类型
 * @param sockfd {ACL_SOCKET} 网络套接字
 * @return {int} -1: 表示出错或输入非法或非套接字; >= 0 表示成功获得套接字
 *  类型，返回值有 AF_INET 或 AF_UNIX(仅限 UNIX 平台)
 */
ACL_API int acl_getsocktype(ACL_SOCKET sockfd);

#ifdef	__cplusplus
}
#endif

#endif

