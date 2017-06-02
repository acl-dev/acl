#ifndef ACL_TCP_CTL_INCLUDE_H
#define ACL_TCP_CTL_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../stdlib/acl_define.h"

#define ACL_SOCKET_RBUF_SIZE	204800  /**< 缺省读缓冲区大小 */
#define ACL_SOCKET_WBUF_SIZE	204800  /**< 缺少写缓冲区大小 */

/**
 * 设置 TCP 套接字的读缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @param size {int} 缓冲区设置大小
 */
ACL_API void acl_tcp_set_rcvbuf(ACL_SOCKET fd, int size);

/**
 * 设置 TCP 套接字的写缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @param size {int} 缓冲区设置大小
 */
ACL_API void acl_tcp_set_sndbuf(ACL_SOCKET fd, int size);

/**
 * 获取 TCP 套接字的读缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 缓冲区大小
 */
ACL_API int  acl_tcp_get_rcvbuf(ACL_SOCKET fd);

/**
 * 获取 TCP 套接字的写缓冲区大小
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 缓冲区大小
 */
ACL_API int  acl_tcp_get_sndbuf(ACL_SOCKET fd);

/**
 * 打开 TCP 套接字的 nodelay 功能
 * @param fd {ACL_SOCKET} 套接字
 */
ACL_API void acl_tcp_set_nodelay(ACL_SOCKET fd);

/**
 * 设置 TCP 套接字的 nodelay 功能
 * @param fd {ACL_SOCKET} 套接字
 * @param onoff {int} 1 表示打开，0 表示关闭
 */
ACL_API void acl_tcp_nodelay(ACL_SOCKET fd, int onoff);

/**
 * 获得 TCP 套接字是否设置了 nodelay 选项
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 1 表示打开，0 表示关闭
 */
ACL_API int acl_get_tcp_nodelay(ACL_SOCKET fd);

/**
 * 设置 TCP 套接字的 SO_LINGER 选项
 * @param fd {ACL_SOCKET} 套接字
 * @param onoff {int} 是否启用 SO_LINGER 选项
 * @param timeout {int} 当SO_LINGER打开时，取消 timed_wait 的时间，单位为秒
 */
ACL_API void acl_tcp_so_linger(ACL_SOCKET fd, int onoff, int timeout);

/**
 * 获得 TCP 套接字的 linger 值
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 返回 -1 表示未设置 linger 选项或内部出错，>= 0 表示设置了
 *  linger 选项且该值表示套接字关闭后该 TCP 连接在内核中维持 TIME_WAIT 状态
 *  的逗留时间(秒)
 */
ACL_API int acl_get_tcp_solinger(ACL_SOCKET fd);

/**
 * 设置监听套接字的延迟接收功能，即当客户端连接上有数据时才将该连接返回
 * 给应用，目前该功能仅支持 Linux
 * @param fd {ACL_SOCKET} 套接字
 * @param timeout {int} 如果客户端连接在规定的时间内未发来数据，也将该连接返回
 *  给应用
 */
ACL_API void acl_tcp_defer_accept(ACL_SOCKET fd, int timeout);

/**
 * 设置监听套接字的快速建立 TCP 连接过程(需要内核支持)
 * @param fd {ACL_SOCKET}
 * @param on {int} 非 0 时打开此功能，否则关闭此功能
 */
ACL_API void acl_tcp_fastopen(ACL_SOCKET fd, int on);

#ifdef __cplusplus
}
#endif

#endif

