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
 * Callback function called before writing log messages to log files, and the log
 * message information is passed to this function. This function is only effective
 * when the user registers it via acl_msg_pre_write.
 * @param ctx {void*} User-defined context
 * @param fmt {const char*} Format string
 * @param ap {va_list} Format parameter list
 */
typedef void (*ACL_MSG_PRE_WRITE_FN)(void *ctx, const char *fmt, va_list ap);

/**
 * Applications can customize log open functions through this callback type. Applications
 * should call this before logging. When acl_msg_register registers a custom open function,
 * when the application calls acl_msg_open, this object will be used to open logs. Log
 * opening will use the default method to open log files.
 * @param file_name {const char*} Parameter passed to the custom log open function,
 *  which is the log file path
 * @param ctx {void*} Parameter passed by the application
 * @return {int} Returns 0 for custom log opening success, -1 to use default log open function
 */
typedef int (*ACL_MSG_OPEN_FN) (const char *file_name, void *ctx);

/**
 * Applications can customize log close functions through this callback type. Applications
 * should call this before logging. When acl_msg_register registers a custom open function,
 * when the application calls acl_msg_close, this object will be used to close logs. Log
 * closing will use the default method to close log files.
 * @param ctx {void*} Parameter passed by the application
 */
typedef void (*ACL_MSG_CLOSE_FN) (void *ctx);

/**
 * Applications can customize log write functions through this callback type. Applications
 * should call this before logging. When acl_msg_register registers a custom write function,
 * when the application writes logs, this custom function will be used to write logs instead
 * of the default log write function.
 * @param ctx {void*} Parameter passed by the application
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
typedef void (*ACL_MSG_WRITE_FN) (void *ctx, const char *fmt, va_list ap);

/**
 * Call this function before logging to register the application's own log open function,
 * log close function, and log write function.
 * @param open_fn {ACL_MSG_OPEN_FN} Custom log open function
 * @param close_fn {ACL_MSG_CLOSE_FN} Custom log close function
 * @param write_fn {ACL_MSG_WRITE_FN} Custom log write function
 * @param ctx {void*} Custom context
 */
ACL_API void acl_msg_register(ACL_MSG_OPEN_FN open_fn, ACL_MSG_CLOSE_FN close_fn,
        ACL_MSG_WRITE_FN write_fn, void *ctx);

/**
 * Unregister custom functions registered via acl_msg_register and restore default log handlers.
 */
ACL_API void acl_msg_unregister(void);

/**
 * Call this function before logging to register the application's private callback function.
 * Before recording logs, log information will be passed to the application through the
 * registered function.
 * @param pre_write {ACL_MSG_PRE_WRITE_FN} Function called before log recording
 * @param ctx {void*} Custom context
 */
ACL_API void acl_msg_pre_write(ACL_MSG_PRE_WRITE_FN pre_write, void *ctx);

/**
 * Global variable indicating verbose level.
 * @DEPRECATED This variable should not be used, only for internal use; external applications should not use it.
 */
extern ACL_API int acl_msg_verbose;

/**
 * When logging is not opened via acl_msg_open, this function controls whether messages
 * from acl_msg_info/error/fatal/warn should be output to standard output (screen). This
 * function can be used to toggle this switch. This switch affects whether messages need
 * to be output to the terminal screen, and does not affect whether to output to log files.
 * @param onoff {int} Non-zero indicates output to screen
 */
ACL_API void acl_msg_stdout_enable(int onoff);

/**
 * When functions like acl_msg_error_xxx/acl_msg_warn_xxx record logs, this function
 * controls whether to record the stack trace of the program/thread. This function can
 * be used to toggle this switch.
 * @param onoff {int} Non-zero indicates recording stack trace of program/thread logs; default is recording
 */
ACL_API void acl_msg_trace_enable(int onoff);

/**
 * Log open function.
 * @param log_file {const char*} Log destination logical string, separated by "|", which can be
 *  local files or remote socket interfaces, e.g.:
 *  /tmp/test.log|UDP:127.0.0.1:12345|TCP:127.0.0.1:12345|UNIX:/tmp/test.sock
 *  If you need to output logs simultaneously to /tmp/test.log, UDP:127.0.0.1:12345,
 *  TCP:127.0.0.1:12345 and UNIX:/tmp/test.sock four log destinations, you can use this format.
 * @param info_pre {const char*} Message displayed before log recording information
 */
ACL_API void acl_msg_open(const char *log_file, const char *info_pre);

/**
 * Log open function.
 * @param fp {ACL_VSTREAM *} Log file stream object
 * @param info_pre {const char*} Message displayed before log recording information
 */
ACL_API void acl_msg_open2(ACL_VSTREAM *fp, const char *info_pre);

/**
 * Close log handler.
 */
ACL_API void acl_msg_close(void);

/**
 * When rotating log files, you need to call this function to rotate log files.
 */

#ifndef	USE_PRINTF_MACRO

/**
 * General level log message recording function.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_info(const char *fmt,...);

/**
 * Warning level log message recording function.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_warn(const char *fmt,...);

/**
 * Error level log message recording function.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_error(const char *fmt,...);

/**
 * Fatal level log message recording function.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_fatal(const char *fmt,...);

/**
 * Fatal level log message recording function.
 * @param status {int} Currently unused
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(2, 3)
	acl_msg_fatal_status(int status, const char *fmt,...);

/**
 * Panic level log message recording function.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_panic(const char *fmt,...);

/**
 * General level log message recording function.
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
ACL_API void acl_msg_info2(const char *fmt, va_list ap);


/**
 * Warning level log message recording function.
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
ACL_API void acl_msg_warn2(const char *fmt, va_list ap);

/**
 * Error level log message recording function.
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
ACL_API void acl_msg_error2(const char *fmt, va_list ap);


/**
 * Fatal level log message recording function.
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
ACL_API void acl_msg_fatal2(const char *fmt, va_list ap);

/**
 * Fatal level log message recording function.
 * @param status {int} Currently unused
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
ACL_API void acl_msg_fatal_status2(int status, const char *fmt, va_list ap);

/**
 * Panic level log message recording function.
 * @param fmt {const char*} Format string
 * @param ap {va_list} Parameter list
 */
ACL_API void acl_msg_panic2(const char *fmt, va_list ap);
#else

/**
 * When rotating log files to standard output, you need to call this function to rotate log files.
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
 * Similar to strerror in standard C, this function is cross-platform and thread-safe,
 * and can get the error message corresponding to a certain error number.
 * @param errnum {unsigned int} Error number
 * @param buffer {char*} Memory buffer to store error message information
 * @param size {int} Size of buffer
 * @return {const char*} Returned address should be the same as buffer
 */
ACL_API const char *acl_strerror(unsigned int errnum, char *buffer, int size);
ACL_API const char *acl_strerror1(unsigned int errnum);

/**
 * Get the error message information when the system last called a function.
 * @param buffer {char*} Memory buffer to store error message information
 * @param size {int} Size of buffer
 * @return {const char*} Returned address should be the same as buffer
 */
ACL_API const char *acl_last_strerror(char *buffer, int size);

/**
 * Get the error message information when the system last called a function. This function
 * internally uses thread-local storage, so it is thread-safe and can be used safely.
 * @return {const char *} Returns error message string 
 */
ACL_API const char *acl_last_serror(void);

/**
 * Get the error number when the system last called a function.
 * @return {int} Error number
 */
ACL_API int acl_last_error(void);

/**
 * Manually set error number.
 * @param errnum {int} Error number
 */
ACL_API void acl_set_error(int errnum);

/**
 * Output message to standard output.
 * @param fmt {const char*} Format string
 * @param ... Variable arguments
 */
ACL_API void ACL_PRINTF(1, 2) acl_msg_printf(const char *fmt,...);

#ifdef  __cplusplus
}
#endif

#endif
