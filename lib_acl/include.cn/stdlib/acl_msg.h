#ifndef ACL_MSG_INCLUDE_H
#define ACL_MSG_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include <stdarg.h>
#include "acl_vstream.h"

#undef	USE_PRINTF_MACRO

/**
 * 在将写日志至日志文件前回调用户自定义的函数，且将日志信息传递给该函数，
 * 只有当用户通过 acl_msg_pre_write 进行设置后才生效
 * @param ctx {void*} 用户的自定义参数
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 格式参数列表
 */
typedef void (*ACL_MSG_PRE_WRITE_FN)(void *ctx, const char *fmt, va_list ap);

/**
 * 应用通过此函数类型可以自定义日志打开函数，当应用在打开日志前调用
 * acl_msg_register 注册了自定义打开函数，则当应用调用 acl_msg_open
 * 时会调用此定义打开日志函数打开日志，否则则用缺省的方法打开日志文件
 * @param file_name {const char*} 回传给自定义日志打开函数的参数，即
 *  将日志文件回传
 * @param ctx {void*} 应用传递进去的参数
 * @return {int} 如果自定义打开日志函数返回 -1 则调用缺省的日志打开函数
 */
typedef int (*ACL_MSG_OPEN_FN) (const char *file_name, void *ctx);

/**
 * 应用通过此函数类型可以自定义日志关闭函数，当应用在打开日志前调用
 * acl_msg_register 注册了自定义打开函数，则当应用调用 acl_msg_close
 * 时会调用此定义关闭日志函数关闭日志，否则则用缺省的方法关闭日志文件
 * @param ctx {void*} 应用传递进去的参数
 */
typedef void (*ACL_MSG_CLOSE_FN) (void *ctx);

/**
 * 应用通过此函数类型可以自定义日志记录函数，当应用在打开日志前调用
 * acl_msg_register 注册了自定义记录函数，则当应用写日志时便用此自定义
 * 函数记录日志，否则用缺省的日志记录函数
 * @param ctx {void*} 应用传递进去的参数
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 参数列表
 */
typedef void (*ACL_MSG_WRITE_FN) (void *ctx, const char *fmt, va_list ap);

/**
 * 在打开日志前调用此函数注册应用自己的日志打开函数、日志关闭函数、日志记录函数
 * @param open_fn {ACL_MSG_OPEN_FN} 自定义日志打开函数
 * @param close_fn {ACL_MSG_CLOSE_FN} 自定义日志关闭函数
 * @param write_fn {ACL_MSG_WRITE_FN} 自定义日志记录函数
 * @param ctx {void*} 自定义参数
 */
ACL_API void acl_msg_register(ACL_MSG_OPEN_FN open_fn, ACL_MSG_CLOSE_FN close_fn,
        ACL_MSG_WRITE_FN write_fn, void *ctx);

/**
 * 将 acl_msg_register 注册自定义函数清除，采用缺省的日志函数集
 */
ACL_API void acl_msg_unregister(void);

/**
 * 在打开日志前调用此函数注册应用的私有函数，在记录日志前会先记录信息通过
 * 此注册的函数传递给应用
 * @param pre_write {ACL_MSG_PRE_WRITE_FN} 日志记录前调用的函数
 * @param ctx {void*} 自定义参数
 */
ACL_API void acl_msg_pre_write(ACL_MSG_PRE_WRITE_FN pre_write, void *ctx);

/**
 * 全局变量，表示调试级别
 * @DEPRECATED 将来该参数将只会内部使用，外部应用不应用它
 */
extern ACL_API int acl_msg_verbose;

/**
 * 当未调用 acl_msg_open 方式打开日志时，调用了 acl_msg_info/error/fatal/warn
 * 的操作，是否允许信息输出至标准输出屏幕上，通过此函数来设置该开关，该开关
 * 仅影响是否需要将信息输出至终端屏幕而不影响是否输出至文件中
 * @param onoff {int} 非 0 表示允许输出至屏幕
 */
ACL_API void acl_msg_stdout_enable(int onoff);

/**
 * 当调用 acl_msg_error_xxx/acl_msg_warn_xxx 等函数记录出错或警告类型的日志时
 * 是否需要记录调用堆栈，可由该函数进行设置
 * @param onoff {int} 非 0 表示允许记录调用出错/警告日志的堆栈，缺省不记录
 */
ACL_API void acl_msg_trace_enable(int onoff);

/**
 * 日志打开函数
 * @param log_file {const char*} 日志接收者集合，由 "|" 分隔，接收器
 *  可以是本地文件或远程套接口，如:
 *  /tmp/test.log|UDP:127.0.0.1:12345|TCP:127.0.0.1:12345|UNIX:/tmp/test.sock
 *  该配置要求将所有日志同时发给 /tmp/test.log, UDP:127.0.0.1:12345,
 *  TCP:127.0.0.1:12345 和 UNIX:/tmp/test.sock 四个日志接收器对象
 * @param info_pre {const char*} 日志记录信息前的提示信息
 */
ACL_API void acl_msg_open(const char *log_file, const char *info_pre);

/**
 * 日志打开函数
 * @param fp {ACL_VSTREAM *} 日志文件流句柄
 * @param info_pre {const char*} 日志记录信息前的提示信息
 */
ACL_API void acl_msg_open2(ACL_VSTREAM *fp, const char *info_pre);

/**
 * 关闭日志函数
 */
ACL_API void acl_msg_close(void);

/**
 * 当记录日志信息至日志文件时，需要调用如下的日志记录函数
 */

#ifndef	USE_PRINTF_MACRO

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_info(const char *fmt,...);

/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_warn(const char *fmt,...);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_error(const char *fmt,...);

/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_fatal(const char *fmt,...);

/**
 * 致命级别日志信息记录函数
 * @param status {int} 当前未用
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(2, 3)
	acl_msg_fatal_status(int status, const char *fmt,...);

/**
 * 恐慌级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_panic(const char *fmt,...);

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
ACL_API void acl_msg_info2(const char *fmt, va_list ap);


/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
ACL_API void acl_msg_warn2(const char *fmt, va_list ap);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
ACL_API void acl_msg_error2(const char *fmt, va_list ap);


/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
ACL_API void acl_msg_fatal2(const char *fmt, va_list ap);

/**
 * 致命级别日志信息记录函数
 * @param status {int} 当前未用
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
ACL_API void acl_msg_fatal_status2(int status, const char *fmt, va_list ap);

/**
 * 恐慌级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
ACL_API void acl_msg_panic2(const char *fmt, va_list ap);
#else

/**
 * 当记录日志信息至标准输出时，需要调用如下的日志记录函数
 */

#include <stdio.h>

#undef	acl_msg_info
#undef	acl_msg_warn
#undef	acl_msg_error
#undef	acl_msg_fatal
#undef	acl_msg_panic

#define	acl_msg_info	acl_msg_printf
#define	acl_msg_warn	acl_msg_printf
#define	acl_msg_error	acl_msg_printf
#define	acl_msg_fatal	acl_msg_printf
#define	acl_msg_panic	acl_msg_printf

#endif

/**
 * 类似于标准C的 strerror, 但该函数是跨平台且是线程安全的，获得对应某个错误
 * 号的错误描述信息
 * @param errnum {unsigned int} 错误号
 * @param buffer {char*} 存储错误描述信息的内存缓冲区
 * @param size {int} buffer 缓冲区的大小
 * @return {const char*} 返回的地址应与 buffer 相同
 */
ACL_API const char *acl_strerror(unsigned int errnum, char *buffer, int size);
ACL_API const char *acl_strerror1(unsigned int errnum);

/**
 * 获得上次系统调用出错时的错误描述信息
 * @param buffer {char*} 存储错误描述信息的内存缓冲区
 * @param size {int} buffer 的空间大小
 * @return {const char*} 返回的地址应与 buffer 相同
 */
ACL_API const char *acl_last_strerror(char *buffer, int size);

/**
 * 获得上次系统调用出错时的错误描述信息，该函数内部采用了线程局部变量，所以是线程
 * 安全的，但使用起来更简单些
 * @return {const char *} 返回错误提示信息 
 */
ACL_API const char *acl_last_serror(void);

/**
 * 获得上次系统调用出错时的错误号
 * @return {int} 错误号
 */
ACL_API int acl_last_error(void);

/**
 * 手工设置错误号
 * @param errnum {int} 错误号
 */
ACL_API void acl_set_error(int errnum);

/**
 * 输出信息至标准输出
 * @param fmt {const char*} 格式参数
 * @param ... 变参序列
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_printf(const char *fmt,...);

#ifdef  __cplusplus
}
#endif

#endif

