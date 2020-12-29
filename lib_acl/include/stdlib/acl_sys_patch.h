#ifndef	ACL_SYS_PATCH_INCLUDE_H
#define	ACL_SYS_PATCH_INCLUDE_H

# ifdef	__cplusplus
extern "C" {
# endif

#include "acl_define.h"
#include "acl_vstream.h"

#if defined(_WIN32) || defined(_WIN64)
struct iovec {
	void *iov_base;   /**< Starting address */
	size_t iov_len;   /**< Number of bytes */
};

#ifdef	HAVE_NO_TIMEVAL
struct timeval {
	long tv_sec;      /**< seconds */
	long tv_usec;     /**< microseconds */
};
#endif

struct timezone {
	int tz_minuteswest; /**< minutes W of Greenwich */
	int tz_dsttime;     /**< type of dst correction */
};

/**
 * 睡眠几秒
 * @param sec {int} 睡眠的秒数
 */
ACL_API void sleep(int sec);

/**
 * 获得当前时间
 * @param tv {struct timeval*} 存储当前时间结果
 * @param tz {struct timezone*} 时区
 */
ACL_API int gettimeofday(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday1(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday2(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday3(struct timeval *tv, struct timezone *tz);
ACL_API int gettimeofday4(struct timeval *tv, struct timezone *tz);

#endif  /* _WIN32 */
#ifdef	ACL_UNIX
# include <sys/uio.h>
#endif

/**
 * 套接字初始化，对于_WIN32平台：需要调用WSAStartup来初始化SOCKET，
 * 而对于UNIX平台：需要通过 signal(SIGPIPE, SIG_IGN) 来忽略信号
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_init(void);

/**
 * 程序退出前调用此函数释放全局套接字资源（仅_WIN32下有效）
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_end(void);

/**
 * 关闭套接字
 * @param fd {ACL_SOCKET} 套接字
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_close(ACL_SOCKET fd);

/**
 * 禁止套接口的发送与接收
 * @param fd {ACL_SOCKET} 套接字
 * @param how {int}
 * @return {int} 返回 0 表示操作成功，否则表示出错
 */
#if defined(_WIN32) || defined(_WIN64)
# ifndef SHUT_RD
#  define SHUT_RD SD_RECEIVE
# endif
# ifndef SHUT_WR
#  define SHUT_WR SD_SEND
# endif
# ifndef SHUT_RDWR
#  define SHUT_RDWR SD_BOTH
# endif
#endif
ACL_API int acl_socket_shutdown(ACL_SOCKET fd, int how);

/**
 * 从套接字读数据
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {void*} 内存缓冲区地址
 * @param size {size_t} buf 缓冲区大小
 * @param timeout {size_t} 读超时时间(秒)
 * @param fp {ACL_VSTREAM*} 网络流, 可以为空
 * @param arg {void*} 用户自已的参数，在回调方式时有用
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * 向套接字写数据
 * @param fd {ACL_SOCKET} 网络套接字
 * @param buf {void*} 数据地址
 * @param size {size_t} buf 数据大小
 * @param timeout {int} 写超时时间(秒)
 * @param fp {ACL_VSTREAM*} 网络流, 可以为空
 * @param arg {void*} 用户自已的参数，在回调方式时有用
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_write(ACL_SOCKET fd, const void *buf,
	size_t size, int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * 向套接字写数据
 * @param fd {ACL_SOCKET} 网络套接字
 * @param vec {const struct iovec*} 数据数组地址
 * @param count {int} vec 数组长度
 * @param timeout {int} 写超时时间(秒)
 * @param fp {ACL_VSTREAM*} 网络流, 可以为空
 * @param arg {void*} 用户自已的参数，在回调方式时有用
 * @return {int} 0: OK; -1: error
 */
ACL_API int acl_socket_writev(ACL_SOCKET fd, const struct iovec *vec,
	int count, int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * 判断套接字是否正常
 * @param fd {ACL_SOCKET}
 * @return {int} 返回值 1 表示正常，返回 0 表示异常
 */
ACL_API	int acl_socket_alive(ACL_SOCKET fd);

/**
 * 打开文件句柄
 * @param filepath {cosnt char*} 文件路径
 * @param flags {int} 打开标志位, O_RDONLY | O_WRONLY | O_RDWR, 
 *  O_CREAT | O_EXCL | O_TRUNC, O_APPEND(for UNIX)
 * @param mode {int} 打开权限位, 仅对UNIX有效, 如：0700, 0755
 * @return {ACL_FILE_HANDLE} 打开的文件句柄，返回 ACL_FILE_INVALID 表示打开失败
 */
ACL_API ACL_FILE_HANDLE acl_file_open(const char *filepath, int flags, int mode);

/**
 * 关闭打开的文件句柄
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_file_close(ACL_FILE_HANDLE fh);

/**
 * 定位文件位置
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param offset {acl_off_t} 偏移位置
 * @param whence {int} 位置标志位：SEEK_CUR, SEEK_SET, SEEK_END
 * @return {acl_off_t} 当前的文件偏移位置
 */
ACL_API acl_off_t acl_lseek(ACL_FILE_HANDLE fh, acl_off_t offset, int whence);

/**
 * 从文件中读数据
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param buf {void*} 存储缓冲区
 * @param size {size_t} buf 缓冲区大小
 * @param timeout {int} 读超时时间(秒)
 * @param fp {ACL_VSTREAM*} 对应的文件流句柄, 可以为空
 * @param arg {void*} 用户传递的参数, 以回调方式使用时此参数有效
 * @return {int} 读到的实际数据, 如果返回 ACL_VSTREAM_EOF 表示读结束或出错
 */
ACL_API int acl_file_read(ACL_FILE_HANDLE fh, void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * 向文件中写数据
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param buf {void*} 数据存储缓冲区
 * @param size {size_t} buf 缓冲区中数据长度大小
 * @param timeout {int} 写超时时间(秒)
 * @param fp {ACL_VSTREAM*} 对应的文件流句柄, 可以为空
 * @param arg {void*} 用户传递的参数, 以回调方式使用时此参数有效
 * @return {int} 成功写的数据量, 如果返回 ACL_VSTREAM_EOF 表示写出错
 */
ACL_API int acl_file_write(ACL_FILE_HANDLE fh, const void *buf, size_t size,
	int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * 向文件中写一组数据
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param vec {const struct iovec*} 数据存储数组
 * @param count {int} vec 数组中元素个数
 * @param timeout {int} 写超时时间(秒)
 * @param fp {ACL_VSTREAM*} 对应的文件流句柄, 可以为空
 * @param arg {void*} 用户传递的参数, 以回调方式使用时此参数有效
 * @return {int} 成功写的数据量, 如果返回 ACL_VSTREAM_EOF 表示写出错
 */
ACL_API int acl_file_writev(ACL_FILE_HANDLE fh, const struct iovec *vec,
	int count, int timeout, ACL_VSTREAM *fp, void *arg);

/**
 * 将文件缓冲区中的数据全部写入硬盘
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param fp {ACL_VSTREAM*} 对应的文件流句柄, 可以为空
 * @param arg {void*} 用户传递的参数, 以回调方式使用时此参数有效
 * @return {int} 0: ok; -1: error
 */
ACL_API int acl_file_fflush(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp, void *arg);

/**
 * 根据文件名取得该文件的大小
 * @param filename {const char*} 文件名
 * @return {acl_int64} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_file_size(const char *filename);

/**
 * 根据文件句柄取得该文件的大小
 * @param fh {ACL_FILE_HANDLE} 文件句柄
 * @param fp {ACL_VSTREAM*} 对应的文件流句柄, 可以为空
 * @param arg {void*} 用户传递的参数, 以回调方式使用时此参数有效
 * @return {acl_int64} >= 0: ok;  -1: error
 */
ACL_API acl_int64 acl_file_fsize(ACL_FILE_HANDLE fh, ACL_VSTREAM *fp, void *arg);

/**
 * 创建 SOCKET 对
 * @param domain {int}
 * @param type {int}
 * @param protocol {int}
 * @param result {ACL_SOCKET [2]} 存储结果
 * @return {int} 成功返回 0，失败返回 -1
 */
ACL_API int acl_sane_socketpair(int domain, int type, int protocol,
		ACL_SOCKET result[2]);

/* in acl_sys_socket.c */

#if defined(_WIN32) || defined(_WIN64)
typedef int (WINAPI *acl_close_socket_fn)(ACL_SOCKET);
typedef int (WINAPI *acl_recv_fn)(ACL_SOCKET, char *, int, int);
typedef int (WINAPI *acl_send_fn)(ACL_SOCKET, const char *, int, int);
#else
typedef int (*acl_close_socket_fn)(ACL_SOCKET);
typedef ssize_t  (*acl_read_fn)(ACL_SOCKET, void *, size_t);
typedef ssize_t  (*acl_recv_fn)(ACL_SOCKET, void *, size_t, int);
typedef ssize_t  (*acl_write_fn)(ACL_SOCKET, const void *, size_t);
typedef ssize_t  (*acl_writev_fn)(ACL_SOCKET, const struct iovec *, int);
typedef ssize_t  (*acl_send_fn)(ACL_SOCKET, const void *, size_t, int);
#endif

#if !defined(_WIN32) && !defined(_WIN64)
ACL_API void acl_set_read(acl_read_fn fn);
ACL_API void acl_set_write(acl_write_fn fn);
ACL_API void acl_set_writev(acl_writev_fn fn);
#endif

ACL_API void acl_set_close_socket(acl_close_socket_fn fn);
ACL_API void acl_set_recv(acl_recv_fn fn);
ACL_API void acl_set_send(acl_send_fn fn);

# ifdef	__cplusplus
}
# endif

#endif

