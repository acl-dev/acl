#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef  ACL_UNIX
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mylog.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "thread/acl_pthread.h"
#include "stdlib/unix/acl_trace.h"
#include "init/acl_init.h"

#endif

int acl_msg_verbose = 0;

#ifndef	USE_PRINTF_MACRO

static int __log_open_flag = 0;
static int __stdout_enable = 0;
static int __trace_enable = 0;

static ACL_MSG_OPEN_FN __open_fn = NULL;
static ACL_MSG_CLOSE_FN __close_fn = NULL;
static ACL_MSG_WRITE_FN __write_fn = NULL;
static ACL_MSG_PRE_WRITE_FN __pre_write_fn = NULL;
static void *__pre_write_ctx = NULL;
static void *__msg_ctx = NULL;

void acl_msg_register(ACL_MSG_OPEN_FN open_fn, ACL_MSG_CLOSE_FN close_fn,
	ACL_MSG_WRITE_FN write_fn, void *ctx)
{
	if (open_fn == NULL || write_fn == NULL)
		return;

	__open_fn = open_fn;
    __close_fn = close_fn;
	__write_fn = write_fn;
	__msg_ctx = ctx;
}

void acl_msg_unregister(void)
{
	__open_fn = NULL;
	__close_fn = NULL;
	__write_fn = NULL;
	__msg_ctx = NULL;
}

void acl_msg_pre_write(ACL_MSG_PRE_WRITE_FN pre_write, void *ctx)
{
	__pre_write_fn = pre_write;
	__pre_write_ctx = ctx;
}

void acl_msg_stdout_enable(int onoff)
{
	__stdout_enable = onoff;
}

void acl_msg_trace_enable(int onoff)
{
	__trace_enable = onoff;
}

void acl_msg_open2(ACL_VSTREAM *fp, const char *info_pre)
{
	if (fp == NULL || info_pre == NULL || *info_pre == 0)
		return;

	if (__open_fn != NULL) {
		int ret = __open_fn(ACL_VSTREAM_PATH(fp), __msg_ctx);
		/* if return < 0, use the default log */
		if (ret < 0) {
			__open_fn = NULL;
			__close_fn = NULL;
			__write_fn = NULL;
			__msg_ctx = NULL;
			acl_log_fp_set(fp, info_pre);
		}
	} else
		acl_log_fp_set(fp, info_pre);

	__log_open_flag = 1;
}

void acl_msg_open(const char *log_file, const char *info_pre)
{
	const char *myname = "acl_msg_open";
	int  ret;

	if (log_file == NULL || *log_file == 0
		|| info_pre == NULL || *info_pre == 0)
	{
		return;
	}
	
	if (__open_fn != NULL) {
		ret = __open_fn(log_file, __msg_ctx);
		/* if return < 0, use the default log */
		if (ret < 0) {
			__open_fn = NULL;
			__close_fn = NULL;
			__write_fn = NULL;
			__msg_ctx = NULL;
			ret = acl_open_log(log_file, info_pre);
		}
	} else
		ret = acl_open_log(log_file, info_pre);

	if (ret < 0) {
		printf("%s: can't open log file=%s, info_pre=%s,%s\n",
			myname, log_file, info_pre, acl_last_serror());
		__log_open_flag = 0;
		__open_fn = NULL;
		__write_fn = NULL;
		__msg_ctx = NULL;
		return;
	}
	__log_open_flag = 1;
}

void acl_msg_close(void)
{
	if (__log_open_flag == 0)
		return;
	acl_close_log();
	__log_open_flag = 0;
	if (__close_fn)
		__close_fn(__msg_ctx);
}

void acl_msg_info(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("info", fmt, ap);
	}

	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_info->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_info->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
}

void acl_msg_info2(const char *fmt, va_list ap)
{
	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("info", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_info->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_info->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}
}

void acl_msg_warn(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("warn", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_warn->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_warn->pid(%ld), ", getpid());
#endif

		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);

	if (__trace_enable)
		acl_trace_info();
}

void acl_msg_warn2(const char *fmt, va_list ap)
{
	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("warn", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_warn->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_warn->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	if (__trace_enable)
		acl_trace_info();
}

void acl_msg_error(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("error", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_error->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_error->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);

	if (__trace_enable)
		acl_trace_info();
}

void acl_msg_error2(const char *fmt, va_list ap)
{
	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("error", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_error->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_error->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	if (__trace_enable)
		acl_trace_info();
}

void acl_msg_fatal(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("fatal", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_fatal->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_fatal->pid(%ld), ", getpid());
#endif
		printf("fatal:");
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
	acl_trace_info();
	acl_close_log();
	abort();
}

void acl_msg_fatal2(const char *fmt, va_list ap)
{
	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("fatal", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_fatal->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_fatal->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	acl_trace_info();
	acl_close_log();
	abort();
}

void acl_msg_fatal_status(int status, const char *fmt,...)
{
	va_list ap;

	(void) status;

	va_start (ap, fmt);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("fatal", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_fatal_status->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_fatal_status->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
	acl_trace_info();
	acl_close_log();
	abort();
}

void acl_msg_fatal_status2(int status, const char *fmt, va_list ap)
{
	(void) status;

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("fatal", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_fatal_status->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_fatal_status->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	acl_trace_info();
	acl_close_log();
	abort();
}

void acl_msg_panic(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("panic", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_panic->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_panic->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
	acl_trace_info();
	acl_close_log();
	abort();
}

void acl_msg_panic2(const char *fmt, va_list ap)
{
	if (__pre_write_fn)
		__pre_write_fn(__pre_write_ctx, fmt, ap);

	if (__log_open_flag) {
		if (__write_fn != NULL)
			__write_fn(__msg_ctx, fmt, ap);
		else
			acl_write_to_log2("panic", fmt, ap);
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("acl_msg_panic->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("acl_msg_panic->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}

	acl_trace_info();
	acl_close_log();
	abort();
}
#endif  /* USE_PRINTF_MACRO */

const char *acl_strerror(unsigned int errnum, char *buffer, int size)
{
	const char *myname = "acl_strerror";
#ifdef	ACL_WINDOWS
        int   L;

	if (buffer == NULL || size <= 0) {
		acl_msg_error("%s, %s(%d): input error",
				__FILE__, myname, __LINE__);
		return NULL;
	}

	L = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM
			| FORMAT_MESSAGE_ARGUMENT_ARRAY,
			NULL,
			errnum,
			0,
			buffer,
			size,
			NULL);
	while ((L > 0) && ((buffer[L - 1] >= 0 && buffer[L - 1] <= 32) || 
		 (buffer[L - 1] == '.')))
	{
		buffer[L - 1] = '\0';
		L--;
	}

	/* resave the error of WIN SYS */
	WSASetLastError(errnum);
#elif	defined(ACL_UNIX)
	if (buffer == NULL || size <= 0) {
		acl_msg_error("%s, %s(%d): input error",
				__FILE__, myname, __LINE__);
		return NULL;
	}

	snprintf(buffer, size, "%s", strerror(errnum));
#else
# error "unknown OS type"
#endif

	return buffer;
}

const char *acl_last_strerror(char *buffer, int size)
{
	return acl_strerror(acl_last_error(), buffer, size);
}

static acl_pthread_key_t __errbuf_key;

static void thread_free_buf(void *buf)
{
	if ((unsigned long) acl_pthread_self() != acl_main_thread_self())
		acl_myfree(buf);
}

#if !defined(HAVE_NO_ATEXIT)
static char *__main_buf = NULL;
static void main_free_buf(void)
{
	if (__main_buf)
		acl_myfree(__main_buf);
}
#endif

static void thread_buf_init(void)
{
	if (acl_pthread_key_create(&__errbuf_key, thread_free_buf) != 0)
		abort();
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

const char *acl_strerror1(unsigned int errnum)
{
	char *buf;
	static int __buf_size = 4096;

	if (acl_pthread_once(&once_control, thread_buf_init) != 0) {
		abort();
	}

	buf = acl_pthread_getspecific(__errbuf_key);
	if (buf == NULL) {
		buf = acl_mymalloc(__buf_size);
		if (acl_pthread_setspecific(__errbuf_key, buf) != 0) {
			abort();
		}
#if !defined(HAVE_NO_ATEXIT)
		if ((unsigned long) acl_pthread_self()
			== acl_main_thread_self()) {
			__main_buf = buf;
			atexit(main_free_buf);
		}
#endif
	}
	return acl_strerror(errnum, buf, __buf_size);
}

const char *acl_last_serror(void)
{
	int errnum = acl_last_error();
	return acl_strerror1(errnum);
}

int acl_last_error(void)
{
#ifdef	ACL_WINDOWS
	int   error;

	error = WSAGetLastError();
	WSASetLastError(error);
	return error;
#else
	return errno;
#endif
}

void acl_set_error(int errnum)
{
#ifdef	ACL_WINDOWS
	WSASetLastError(errnum);
#endif
	errno = errnum;
}

void acl_msg_printf(const char *fmt,...)
{
	char  buf[2048];
	va_list ap;

	va_start (ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	printf("%s\r\n", buf);

	va_end (ap);
}
