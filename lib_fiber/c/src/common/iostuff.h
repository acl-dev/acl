#ifndef	IOSTUFF_INCLUDE_H
#define	IOSTUFF_INCLUDE_H
#include <fiber/fiber_define.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define CLOSE_ON_EXEC   1  /**< 标志位, 调用 exec 后自动关闭打开的描述字 */
#define PASS_ON_EXEC    0

#define BLOCKING        0  /**< 阻塞读写标志位 */
#define NON_BLOCKING    1  /**< 非阻塞读写标志位 */

/**
 * 设置套接口为阻塞或非阻塞
 * @param fd {int} 套接字
 * @param on {int} 是否设置该套接字为非阻塞, BLOCKING 或 NON_BLOCKING
 * @return {int} >= 0: 成功, 返回值 > 0 表示设置之前的标志位; -1: 失败
 */
int non_blocking(socket_t fd, int on);

/**
 * 毫秒级别睡眠
 * @param delay {unsigned} 毫秒值
 */
void doze(unsigned delay);

/**
 * 设置文件描述符标志位，当调用 exec 后该描述符自动被关闭
 * @param fd {int} 文件描述符
 * @param on {int} 1 或 0
 * @return {int} 0: ok; -1: error
 */
int close_on_exec(int fd, int on);

/**
 * 设定当前进程可以打开最大文件描述符值
 * @param limit {int} 设定的最大值
 * @return {int} >=0: ok; -1: error
 */
int open_limit(int limit);

/**
 * 判断给定某个文件描述字是否是套接字
 * @param fd {int} 文件描述符
 * @return {int} != 0: 是; 0: 否
 */
int issock(int fd);

//int read_wait(int fd, int timeout);
void tcp_nodelay(socket_t fd, int onoff);

// in read_wait.c
int read_wait(socket_t fd, int delay);

#ifdef SYS_UNIX
int sane_socketpair(int domain, int type, int protocol, int result[2]);
#endif

#ifdef	__cplusplus
}
#endif

#endif
