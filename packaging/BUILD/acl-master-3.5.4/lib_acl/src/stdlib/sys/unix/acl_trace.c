#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#include "thread/acl_pthread.h"
#include "init/acl_init.h"
#include "stdlib/unix/acl_trace.h"
#endif

#if defined(HAS_TRACE)

#include <unistd.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

void acl_trace_save(const char *filepath)
{
	const char *myname = "acl_trace_save";
	int   fd;
	void *buffer[1000];
	size_t n;

	n = backtrace(buffer, 1000);
	if (n == 0)
		return;

	fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
	if (fd == -1) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return;
	}

	backtrace_symbols_fd(buffer, n, fd);
	close(fd);
}

static acl_pthread_key_t __trace_key;
static acl_pthread_once_t __trace_once = ACL_PTHREAD_ONCE_INIT;
static unsigned int *__main_buf = NULL;

static void trace_buf_free(void *buf)
{
	if ((unsigned long) acl_pthread_self() != acl_main_thread_self())
		free(buf);
}

#ifndef HAVE_NO_ATEXIT
static void main_buf_free(void)
{
	if (__main_buf)
		free(__main_buf);
}
#endif

static void trace_buf_init(void)
{
	if (acl_pthread_key_create(&__trace_key, trace_buf_free) != 0)
        abort();
}

void acl_trace_info(void)
{
	void *buffer[1000];
	size_t n, i;
	char **results;
	unsigned int *intbuf;

	/* 初始化线程局部变量 */
	if (acl_pthread_once(&__trace_once, trace_buf_init) != 0)
		return;
	intbuf = acl_pthread_getspecific(__trace_key);
	if (intbuf == NULL) {
		intbuf = malloc(sizeof(int));
		*intbuf = 0;
		if (acl_pthread_setspecific(__trace_key, intbuf) != 0)
            abort();
		if ((unsigned long) acl_pthread_self()
			== acl_main_thread_self())
		{
			__main_buf = intbuf;
#ifndef HAVE_NO_ATEXIT
			atexit(main_buf_free);
#endif
		}
	}

	/* 如果产生递归嵌套，则直接返回 */
	if ((*intbuf) > 0)
		return;

	n = backtrace(buffer, 1000);
	if (n == 0)
		return;

	/* 防止递归嵌套标志自增 */
	(*intbuf)++;

	results = backtrace_symbols(buffer, n);

	/* 记录下所有的堆栈信息 */
	for (i = 0; i < n; i++)
		acl_msg_info("backtrace: %s", results[i]);

	if (results != NULL)
		free(results);

	/* 防止递归嵌套标志自减 */
	(*intbuf)--;
}

#else

void acl_trace_save(const char *filepath acl_unused)
{
}

void acl_trace_info(void)
{
}

#endif
