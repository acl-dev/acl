#ifndef _MSG_INCLUDE_H_
#define _MSG_INCLUDE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include "define.h"

#undef	USE_PRINTF_MACRO

/**
 * 在将写日志至日志文件前回调用户自定义的函数，且将日志信息传递给该函数，
 * 只有当用户通过 msg_pre_write 进行设置后才生效
 * @param ctx {void*} 用户的自定义参数
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 格式参数列表
 */
typedef void (*MSG_PRE_WRITE_FN)(void *ctx, const char *fmt, va_list ap);

/**
 * 应用通过此函数类型可以自定义日志打开函数，当应用在打开日志前调用
 * msg_register 注册了自定义打开函数，则当应用调用 msg_open
 * 时会调用此定义打开日志函数打开日志，否则则用缺省的方法打开日志文件
 * @param file_name {const char*} 回传给自定义日志打开函数的参数，即
 *  将日志文件回传
 * @param ctx {void*} 应用传递进去的参数
 * @return {int} 如果自定义打开日志函数返回 -1 则调用缺省的日志打开函数
 */
typedef int (*MSG_OPEN_FN) (const char *file_name, void *ctx);

/**
 * 应用通过此函数类型可以自定义日志关闭函数，当应用在打开日志前调用
 * msg_register 注册了自定义打开函数
 * @param ctx {void*} 应用传递进去的参数
 */
typedef void (*MSG_CLOSE_FN) (void *ctx);

/**
 * 应用通过此函数类型可以自定义日志记录函数，当应用在打开日志前调用
 * msg_register 注册了自定义记录函数，则当应用写日志时便用此自定义
 * 函数记录日志，否则用缺省的日志记录函数
 * @param ctx {void*} 应用传递进去的参数
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 参数列表
 */
typedef void (*MSG_WRITE_FN) (void *ctx, const char *fmt, va_list ap);

/**
 * 在打开日志前调用此函数注册应用自己的日志打开函数、日志关闭函数、日志记录函数
 * @param open_fn {MSG_OPEN_FN} 自定义日志打开函数
 * @param close_fn {MSG_CLOSE_FN} 自定义日志关闭函数
 * @param write_fn {MSG_WRITE_FN} 自定义日志记录函数
 * @param ctx {void*} 自定义参数
 */
void msg_register(MSG_OPEN_FN open_fn, MSG_CLOSE_FN close_fn,
        MSG_WRITE_FN write_fn, void *ctx);

/**
 * 将 msg_register 注册自定义函数清除，采用缺省的日志函数集
 */
void msg_unregister(void);

/**
 * 在打开日志前调用此函数注册应用的私有函数，在记录日志前会先记录信息通过
 * 此注册的函数传递给应用
 * @param pre_write {MSG_PRE_WRITE_FN} 日志记录前调用的函数
 * @param ctx {void*} 自定义参数
 */
void msg_pre_write(MSG_PRE_WRITE_FN pre_write, void *ctx);

/**
 * 当未调用 msg_open 方式打开日志时，调用了 msg_info/error/fatal/warn
 * 的操作，是否允许信息输出至标准输出屏幕上，通过此函数来设置该开关，该开关
 * 仅影响是否需要将信息输出至终端屏幕而不影响是否输出至文件中
 * @param onoff {int} 非 0 表示允许输出至屏幕
 */
void msg_stdout_enable(int onoff);

/**
 * 当记录日志信息至日志文件时，需要调用如下的日志记录函数
 */

#ifndef	USE_PRINTF_MACRO

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_info(const char *fmt,...);

/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_warn(const char *fmt,...);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_error(const char *fmt,...);

/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_fatal(const char *fmt,...);

/**
 * 一般级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
void msg_info2(const char *fmt, va_list ap);

/**
 * 警告级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
void msg_warn2(const char *fmt, va_list ap);

/**
 * 错误级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
void msg_error2(const char *fmt, va_list ap);


/**
 * 致命级别日志信息记录函数
 * @param fmt {const char*} 参数格式
 * @param ap {va_list} 变参列表
 */
void msg_fatal2(const char *fmt, va_list ap);

#else

/**
 * 当记录日志信息至标准输出时，需要调用如下的日志记录函数
 */

#include <stdio.h>

#undef	msg_info
#undef	msg_warn
#undef	msg_error
#undef	msg_fatal
#undef	msg_panic

#define	msg_info	msg_printf
#define	msg_warn	msg_printf
#define	msg_error	msg_printf
#define	msg_fatal	msg_printf
#define	msg_panic	msg_printf

#endif

const char *msg_strerror(int errnum, char *buffer, size_t size);

/**
 * 获得上次系统调用出错时的错误描述信息
 * @param buffer {char*} 存储错误描述信息的内存缓冲区
 * @param size {size_t} buffer 的空间大小
 * @return {const char*} 返回的地址应与 buffer 相同
 */
const char *last_strerror(char *buffer, size_t size);

/**
 * 获得上次系统调用出错时的错误描述信息，该函数内部采用了线程局部变量，所以是
 * 线程安全的，但使用起来更简单些
 * @return {const char *} 返回错误提示信息 
 */
const char *last_serror(void);

/**
 * 获得上次系统调用出错时的错误号
 * @return {int} 错误号
 */
int last_error(void);

/**
 * 手工设置错误号
 * @param errnum {int} 错误号
 */
void set_error(int errnum);

/**
 * 输出信息至标准输出
 * @param fmt {const char*} 格式参数
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_printf(const char *fmt,...);

#ifdef  __cplusplus
}
#endif

#endif
