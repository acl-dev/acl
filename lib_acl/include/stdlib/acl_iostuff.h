#ifndef	ACL_IOSTUFF_INCLUDE_H
#define	ACL_IOSTUFF_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstream.h"

#define ACL_CLOSE_ON_EXEC   1  /**< 标志位, 调用 exec 后自动关闭打开的描述字 */
#define ACL_PASS_ON_EXEC    0

#define ACL_BLOCKING        0  /**< 阻塞读写标志位 */
#define ACL_NON_BLOCKING    1  /**< 非阻塞读写标志位 */

/**
 * 设置套接口为阻塞或非阻塞
 * @param fd {ACL_SOCKET} SOCKET 套接字
 * @param on {int} 是否设置该套接字为非阻塞, ACL_BLOCKING 或 ACL_NON_BLOCKING
 * @return {int} >= 0: 成功, 返回值 > 0 表示设置之前的标志位; -1: 失败
 */
ACL_API int acl_non_blocking(ACL_SOCKET fd, int on);

/**
 * 判断所给套按口是否为阻塞模式
 * @param fd {ACL_SOCKET}  SOCKET 套接字
 * @return {int} -1 表示出错或所给参数有误或该平台不支持，1 表示所给套接字为阻塞模式
 *  0 表示所给套接字为非阻塞模式
 */
ACL_API int acl_is_blocking(ACL_SOCKET fd);

/**
 * 写等待操作，直到套接字可写、出错或超时
 * @param fd {ACL_SOCKET} 描述符
 * @param timeout {int} 超时时间，单位为秒，该值分下面三种情形：
 *  > 0  : 表示最大超时时间的秒数，
 *  == 0 : 表示不等待，检测完后立即返回
 *  < 0  : 时表示直接该套接字可读或出错为止
 * @return {int} 0: 可写; -1: 失败或超时
 */
ACL_API int acl_write_wait(ACL_SOCKET fd, int timeout);

/**
 * 读等待操作，直到套接字有数据可读、出错或超时
 * @param fd {ACL_SOCKET} 描述符
 * @param timeout {int} 超时时间，单位为秒，该值分下面三种情形：
 *  > 0  : 表示最大超时时间的秒数，
 *  == 0 : 表示不等待，检测完后立即返回
 *  < 0  : 时表示直接该套接字可读或出错为止
 * @return {int} 0: 有数据可读或描述字出错; -1: 失败或超时
 */
ACL_API int acl_read_wait(ACL_SOCKET fd, int timeout);

#if defined(ACL_LINUX) && !defined(MINGW)
/**
 * 采用 epoll 方式的读等待
 * @param fd {ACL_SOCKET} 描述符
 * @param delay {int} 毫秒级等待时间
 * @return {int} 含义同上
 */
ACL_API int acl_read_epoll_wait(ACL_SOCKET fd, int delay);
#endif

#if defined(ACL_UNIX)
/**
 * 采用 poll 方式的读等待
 * @param fd {ACL_SOCKET} 描述符
 * @param delay {int} 毫秒级等待时间
 * @return {int} 含义同上
 */
ACL_API int acl_read_poll_wait(ACL_SOCKET fd, int delay);
#endif

/**
 * 采用 select 方式的读等待
 * @param fd {ACL_SOCKET} 描述符
 * @param delay {int} 毫秒级等待时间
 * @return {int} 含义同上
 */
ACL_API int acl_read_select_wait(ACL_SOCKET fd, int delay);

/**
 * 毫秒级别睡眠
 * @param delay {unsigned} 毫秒值
 */
ACL_API void acl_doze(unsigned delay);

/**
* 某个描述符是否可读
* @param fd {ACL_SOCKET} 描述符
* @return {int} 0: 不可读; -1: 出错， 1：可读
*/
ACL_API int acl_readable(ACL_SOCKET fd);

/**
 * 超时读数据
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {void*} 存储区，不能为空
 * @param len {unsigned} buf 存储区大小
 * @param timeout {int} 超时时间，单位为秒，该值分下面三种情形：
 *  > 0  : 表示最大超时时间的秒数，
 *  == 0 : 表示不等待，检测完后立即返回
 *  < 0  : 时表示直接该套接字可读或出错为止
 * @return {int} > 0 读的数据; -1: 出错
*/
ACL_API int acl_timed_read(ACL_SOCKET fd, void *buf, unsigned len,
	int timeout, void *unused_context);

/**
 * 超时写数据
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {void*} 数据存储区，不能为空
 * @param len {unsigned} 数据长度大小
 * @param timeout {int} 超时时间，单位为秒，该值分下面三种情形：
 *  > 0  : 表示最大超时时间的秒数，
 *  == 0 : 表示不等待，检测完后立即返回
 *  < 0  : 时表示直接该套接字可读或出错为止
 * @return {int} > 0 成功写入的数据; -1: 出错
 */
ACL_API int acl_timed_write(ACL_SOCKET fd, void *buf, unsigned len,
	int timeout, void *unused_context);

/**
 * 向描述符中循环写入数据，直到写完、出错或超时为止
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {void*} 数据存储区，不能为空
 * @param len {unsigned} 数据长度大小
 * @param timeout {int} 超时时间，单位为秒
 * @return {int} 成功写入的长度
 */
ACL_API int acl_write_buf(ACL_SOCKET fd, const char *buf, int len, int timeout);

/**
 * 探测套接字中系统缓存区的数据长度
 * @param fd {ACL_SOCKET} 描述符
 * @return {int} 系统缓存区数据长度
 */
ACL_API int acl_peekfd(ACL_SOCKET fd);

/**
 * 创建管道
 * @param fds {ACL_FILE_HANDLE [2]} 存储结果
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_pipe(ACL_FILE_HANDLE fds[2]);

/**
 * 关闭管道对
 * @param fds {ACL_FILE_HANDLE[2]} 管道对
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_pipe_close(ACL_FILE_HANDLE fds[2]);

/**
* 产生一个管道对
* @param fds {ACL_FILE_HANDLE[2]} 存储产生的管道对地址，不能为空
* @return 0: ok; -1: error
*/
ACL_API int acl_duplex_pipe(ACL_FILE_HANDLE fds[2]);

#ifdef	ACL_UNIX
/**
 * 设置文件描述符标志位，当调用 exec 后该描述符自动被关闭
 * @param fd {int} 文件描述符
 * @param on {int} ACL_CLOSE_ON_EXEC 或 0
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_close_on_exec(int fd, int on);

/**
 * 从某个文件描述符开始关闭之后的所有打开的文件描述符
 * @param lowfd {int} 被关闭描述符的最低值
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_closefrom(int lowfd);

/**
 * 设定当前进程可以打开最大文件描述符值
 * @param limit {int} 设定的最大值
 * @return {int} >=0: ok; -1: error
 */
ACL_API int acl_open_limit(int limit);

/**
 * 判断给定某个文件描述字是否是套接字
 * @param fd {int} 文件描述符
 * @return {int} != 0: 是; 0: 否
 */
ACL_API int acl_issock(int fd);
#endif

#if defined(_WIN32) || defined(_WIN64)
typedef int (WINAPI *acl_select_fn)(int, fd_set*, fd_set*,
	fd_set*, const struct timeval*);
# if(_WIN32_WINNT >= 0x0600)
#  define ACL_HAS_POLL
typedef int (WINAPI *acl_poll_fn)(struct pollfd*, unsigned long, int);
ACL_API void acl_set_poll(acl_poll_fn fn);
# endif

#else
#include <poll.h>
# define ACL_HAS_POLL

typedef int (*acl_select_fn)(int, fd_set*, fd_set*, fd_set*, struct timeval*);
typedef int (*acl_poll_fn)(struct pollfd*, nfds_t, int);
ACL_API void acl_set_poll(acl_poll_fn fn);
#endif

ACL_API void acl_set_select(acl_select_fn fn);

#ifdef	__cplusplus
}
#endif

#endif
