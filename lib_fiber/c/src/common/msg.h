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

#else

/**
 * 当记录日志信息至标准输出时，需要调用如下的日志记录函数
 */

#include <stdio.h>

#undef	msg_info
#undef	msg_warn
#undef	msg_error
#undef	msg_fatal

#define	msg_info	msg_printf
#define	msg_warn	msg_printf
#define	msg_error	msg_printf
#define	msg_fatal	msg_printf

#endif

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
 * 输出信息至标准输出
 * @param fmt {const char*} 格式参数
 * @param ... 变参序列
 */
void PRINTF(1, 2) msg_printf(const char *fmt,...);

#ifdef  __cplusplus
}
#endif

#endif
