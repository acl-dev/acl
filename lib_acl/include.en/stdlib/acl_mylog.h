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
 * Convert current time to log recording format time string format.
 * @param buf {char*} Memory buffer
 * @param size {size_t} buf space size
 */
ACL_API void acl_logtime_fmt(char *buf, size_t size);

/**
 * Set whether to record thread ID number, default is not to record.
 * @param onoff {int} Non-zero indicates record thread ID, zero indicates do not record
 */
ACL_API void acl_log_add_tid(int onoff);

/**
 * Set log file stream object.
 * @param fp {ACL_VSTREAM *} File stream object
 * @param plog_pre {const char*} Message displayed before log recording information, can be NULL
 */
ACL_API void acl_log_fp_set(ACL_VSTREAM *fp, const char *plog_pre);

/**
 * Open log file.
 * @param recipients {const char*} Log destination list, separated by "|", which can be
 *  local files or remote socket interfaces, e.g.:
 *  /tmp/test.log|UDP:127.0.0.1:12345|TCP:127.0.0.1:12345|UNIX:/tmp/test.sock
 *  If you need to output logs simultaneously to /tmp/test.log, UDP:127.0.0.1:12345,
 *  TCP:127.0.0.1:12345 and UNIX:/tmp/test.sock four log destinations, you can use this format.
 * @param plog_pre {const char*} Message displayed before log recording information, can be NULL
 *  or empty value
 */
ACL_API int acl_open_log(const char *recipients, const char *plog_pre);

/**
 * Before calling acl_open_log, you can call this function to set whether log fd should
 * set acl_close_on_exec. By default, acl_close_on_exec will be automatically set.
 * @param yes {int} Non-zero indicates set acl_close_on_exec, zero indicates do not set
 */
ACL_API void acl_log_close_onexec(int yes);

/**
 * Write log.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 * @return {int} Number of bytes written to log file
 */
ACL_API int ACL_PRINTF(1, 2) acl_write_to_log(const char *fmt, ...);

/**
 * Write log.
 * @param info {const char*} Log message prefix information
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 * @return {int} Number of bytes written to log file
 */
ACL_API int acl_write_to_log2(const char *info, const char *fmt, va_list ap);

/**
 * Close log file stream.
 */
ACL_API void acl_close_log(void);

ACL_API ACL_ARRAY *acl_log_get_streams(void);
ACL_API void acl_log_free_streams(ACL_ARRAY *a);

#ifdef  __cplusplus
}
#endif

#endif
