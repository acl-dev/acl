#include "stdafx.h"

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "init.h"
#include "msg.h"


#ifndef	USE_PRINTF_MACRO

static int __log_open_flag = 0;
static int __stdout_enable = 0;

static MSG_OPEN_FN __open_fn           = NULL;
static MSG_CLOSE_FN __close_fn         = NULL;
static MSG_WRITE_FN __write_fn         = NULL;
static MSG_PRE_WRITE_FN __pre_write_fn = NULL;
static void *__pre_write_ctx           = NULL;
static void *__msg_ctx                 = NULL;

void msg_register(MSG_OPEN_FN open_fn, MSG_CLOSE_FN close_fn,
	MSG_WRITE_FN write_fn, void *ctx)
{
	if (open_fn == NULL || write_fn == NULL)
		return;

	__open_fn  = open_fn;
	__close_fn = close_fn,
	__write_fn = write_fn;
	__msg_ctx  = ctx;
}

void msg_unregister(void)
{
	__open_fn  = NULL;
	__close_fn = NULL;
	__write_fn = NULL;
	__msg_ctx  = NULL;
}

void msg_pre_write(MSG_PRE_WRITE_FN pre_write, void *ctx)
{
	__pre_write_fn  = pre_write;
	__pre_write_ctx = ctx;
}

void msg_stdout_enable(int onoff)
{
	__stdout_enable = onoff;
}

void msg_info(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}

	if (__stdout_enable) {
		printf("msg_info->pid(%d), ", getpid());
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
}

void msg_info2(const char *fmt, va_list ap)
{
	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
#ifdef LINUX
		printf("msg_info->pid(%d), ", getpid());
#elif defined(SOLARIS)
		printf("msg_info->pid(%ld), ", getpid());
#endif
		vprintf(fmt, ap);
		printf("\r\n");
	}
}

void msg_warn(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
		printf("msg_warn->pid(%d), ", getpid());
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
}

void msg_warn2(const char *fmt, va_list ap)
{
	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
		printf("msg_warn->pid(%d), ", getpid());
		vprintf(fmt, ap);
		printf("\r\n");
	}
}

void msg_error(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
		printf("msg_error->pid(%d), ", getpid());
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
}

void msg_error2(const char *fmt, va_list ap)
{
	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
		printf("msg_error->pid(%d), ", getpid());
		vprintf(fmt, ap);
		printf("\r\n");
	}
}

void msg_fatal(const char *fmt,...)
{
	va_list ap;

	va_start (ap, fmt);

	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
		printf("msg_fatal->pid(%d), ", getpid());
		printf("fatal:");
		vprintf(fmt, ap);
		printf("\r\n");
	}

	va_end (ap);
	assert(0);
}

void msg_fatal2(const char *fmt, va_list ap)
{
	if (__pre_write_fn) {
		__pre_write_fn(__pre_write_ctx, fmt, ap);
	}

	if (__log_open_flag) {
		if (__write_fn != NULL) {
			__write_fn(__msg_ctx, fmt, ap);
		}
	}
	
	if (__stdout_enable) {
		printf("msg_fatal->pid(%d), ", getpid());
		vprintf(fmt, ap);
		printf("\r\n");
	}

	assert(0);
}

#endif  /* USE_PRINTF_MACRO */

const char *msg_strerror(int errnum, char *buffer, size_t size)
{
	if (buffer == NULL || size <= 0) {
		msg_error("%s, %s(%d): input error",
			__FILE__, __FUNCTION__, __LINE__);
		return NULL;
	}

	snprintf(buffer, size, "%s", strerror(errnum));

	return buffer;
}

const char *last_strerror(char *buffer, size_t size)
{
	return msg_strerror(last_error(), buffer, size);
}

static pthread_key_t __errbuf_key;
static char *__main_buf = NULL;

static void thread_free_buf(void *buf)
{
	if ((unsigned long) pthread_self() != main_thread_self())
		free(buf);
}

static void main_free_buf(void)
{
	if (__main_buf)
		free(__main_buf);
}

static void thread_buf_init(void)
{
	if (pthread_key_create(&__errbuf_key, thread_free_buf) != 0)
		abort();
}

static pthread_once_t once_control = PTHREAD_ONCE_INIT;

const char *last_serror(void)
{
	char *buf;
	int   error = last_error();
	static size_t __buf_size = 4096;

	if (pthread_once(&once_control, thread_buf_init) != 0)
		abort();

	buf = pthread_getspecific(__errbuf_key);
	if (buf == NULL) {
		buf = malloc(__buf_size);
		assert(pthread_setspecific(__errbuf_key, buf) == 0);
		if ((unsigned long) pthread_self()
			== main_thread_self())
		{
			__main_buf = buf;
			atexit(main_free_buf);
		}
	}
	return msg_strerror(error, buf, __buf_size);
}

int last_error(void)
{
	return errno;
}

void set_error(int errnum)
{
	errno = errnum;
}

void msg_printf(const char *fmt,...)
{
	char  buf[2048];
	va_list ap;

	va_start (ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	printf("%s\r\n", buf);

	va_end (ap);
}
