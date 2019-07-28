#ifndef	ACL_MYLOG_INCLUDE_H
#define	ACL_MYLOG_INCLUDE_H

#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstream.h"

#ifdef	ACL_UNIX
#include <netinet/in.h>
#endif

typedef struct ACL_LOG ACL_LOG;

/**
 * 将当前的时间转换成日志记录格式的时间格式
 * @param buf {char*} 内存存储区
 * @param size {size_t} buf 的空间大小
 */
ACL_API void acl_logtime_fmt(char *buf, size_t size);

/**
 * 设置是否记录线程ID号，默认情况下是不记录的
 * @param onoff {int} 非 0 表示记录线程ID，否则不记录
 */
ACL_API void acl_log_add_tid(int onoff);

/**
 * 设置日志的文件流句柄
 * @param fp {ACL_VSTREAM *} 文件流句柄
 * @param plog_pre {const char*} 日志记录信息前的提示信息，建议用进程
 */
ACL_API void acl_log_fp_set(ACL_VSTREAM *fp, const char *plog_pre);

/**
 * 打开日志文件
 * @param recipients {const char*} 日志接收器列表，由 "|" 分隔，接收器
 *  可以是本地文件或远程套接口，如:
 *  /tmp/test.log|UDP:127.0.0.1:12345|TCP:127.0.0.1:12345|UNIX:/tmp/test.sock
 *  该配置要求将所有日志同时发给 /tmp/test.log, UDP:127.0.0.1:12345,
 *  TCP:127.0.0.1:12345 和 UNIX:/tmp/test.sock 四个日志接收器对象
 * @param plog_pre {const char*} 日志记录信息前的提示信息，建议用进程
 *  名填写此值
 */
ACL_API int acl_open_log(const char *recipients, const char *plog_pre);

/**
 * 在调用 acl_open_log 前，可以调用本函数用来设定针对日志 fd 是否调用
 * acl_close_on_exec，缺省情况下会自动调用 acl_close_on_exec
 * @param yes {int} 非 0 表示调用 acl_close_on_exec，否则表示不调用
 */
ACL_API void acl_log_close_onexec(int yes);

/**
 * 写日志
 * @param fmt {const char*} 格式参数
 * @param ... 参数序列
 * @return {int} 写入日志文件的数据量
 */
ACL_API int ACL_PRINTF(1, 2) acl_write_to_log(const char *fmt, ...);

/**
 * 写日志
 * @param info {const char*} 日志信息的提示信息
 * @param fmt {const char*} 格式参数
 * @param ap {va_list} 参数列表
 * @return {int} 写入日志文件的数据量
 */
ACL_API int acl_write_to_log2(const char *info, const char *fmt, va_list ap);

/**
 * 关闭日志文件句柄
 */
ACL_API void acl_close_log(void);

#ifdef  __cplusplus
}
#endif

#endif


