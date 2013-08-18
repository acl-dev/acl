#ifndef __ACL_TCP_CTL_INCLUDE_H__
#define __ACL_TCP_CTL_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

#define ACL_SOCKET_RBUF_SIZE	204800  /**< 缺省读缓冲区大小 */
#define ACL_SOCKET_WBUF_SIZE	204800  /**< 缺少写缓冲区大小 */

/**
 * 设置套接字的读缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @param size {int} 缓冲区设置大小
 */
ACL_API void acl_tcp_set_rcvbuf(ACL_SOCKET fd, int size);

/**
 * 设置套接字的写缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @param size {int} 缓冲区设置大小
 */
ACL_API void acl_tcp_set_sndbuf(ACL_SOCKET fd, int size);

/**
 * 获取套接字的读缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 缓冲区大小
 */
ACL_API int  acl_tcp_get_rcvbuf(ACL_SOCKET fd);

/**
 * 获取套接字的写缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 缓冲区大小
 */
ACL_API int  acl_tcp_get_sndbuf(ACL_SOCKET fd);

/**
 * 打开套接字的 nodelay 功能
 * @param fd {ACL_SOCKET} 套接字
 */
ACL_API void acl_tcp_set_nodelay(ACL_SOCKET fd);

/**
 * 设置套接字的 nodelay 功能
 * @param fd {ACL_SOCKET} 套接字
 * @param onoff {int} 1 表示打开，0 表示关闭
 */
ACL_API void acl_tcp_nodelay(ACL_SOCKET fd, int onoff);

/**
 * 设置监听套接字的延迟接收功能，即当客户端连接上有数据时才将该连接返回
 * 给应用，目前该功能仅支持 Linux
 * @param fd {ACL_SOCKET} 套接字
 * @param timeout {int} 如果客户端连接在规定的时间内未发来数据，也将该连接返回
 *  给应用
 */
ACL_API void acl_tcp_defer_accept(ACL_SOCKET fd, int timeout);

/**
 * 设置套接字的 SO_LINGER 选项
 * @param fd {ACL_SOCKET} 套接字
 * @param onoff {int} 是否启用 SO_LINGER 选项
 * @param timeout {int} 当SO_LINGER打开时，取消 timed_wait 的时间，单位为秒
 */
ACL_API void acl_tcp_so_linger(ACL_SOCKET fd, int onoff, int timeout);

#ifdef __cplusplus
}
#endif

#endif

