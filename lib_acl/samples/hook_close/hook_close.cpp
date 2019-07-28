#include "lib_acl.h"
#include <unistd.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <dlfcn.h>

#include "hook_close.h"

static acl_pthread_key_t __trace_key;
static acl_pthread_once_t __trace_once = ACL_PTHREAD_ONCE_INIT;
static unsigned int *__main_buf = NULL;

static void trace_buf_free(void *buf)
{
	if ((unsigned long) acl_pthread_self() != acl_main_thread_self())
		free(buf);
}

static void main_buf_free(void)
{
	if (__main_buf)
		free(__main_buf);
}

static void trace_buf_init(void)
{
	acl_assert(acl_pthread_key_create(&__trace_key, trace_buf_free) == 0);
}

static void trace_info(const char* prompt)
{
	void *buffer[1000];
	size_t n, i;
	char **results;
	unsigned int *intbuf;

	if (acl_pthread_once(&__trace_once, trace_buf_init) != 0)
		return;
	intbuf = (unsigned int *) acl_pthread_getspecific(__trace_key);
	if (intbuf == NULL) {
		intbuf = (unsigned int *) malloc(sizeof(int));
		*intbuf = 0;
		acl_assert(acl_pthread_setspecific(__trace_key, intbuf) == 0);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
			__main_buf = intbuf;
			atexit(main_buf_free);
		}
	}

	if ((*intbuf) > 0)
		return;

	n = backtrace(buffer, 1000);
	if (n == 0)
		return;

	(*intbuf)++;

	results = backtrace_symbols(buffer, n);

	for (i = 0; i < n; i++)
		acl_msg_info("%s->backtrace: %s", prompt, results[i]);

	if (results != NULL)
		free(results);

	(*intbuf)--;
}

/****************************************************************************/

typedef int     (*close_fn)(int);
static close_fn    __sys_close    = NULL;

void hook_setup(void) __attribute__ ((constructor));
void hook_setup(void)
{
	__sys_close    = (close_fn) dlsym(RTLD_NEXT, "close");
}

static int __monitor_ports[100];
static int __monitor_ports_max = sizeof(__monitor_ports);
static int __monitor_ports_cur = 0;

int monitor_port_add(int port)
{
	if (port <= 0) {
		acl_msg_error("%s(%d), %s: invalid port: %d",
			__FILE__, __LINE__, __FUNCTION__, port);
		return -1;
	}

	if (__monitor_ports_cur >= __monitor_ports_max) {
		acl_msg_error("too many ports added, cur: %d, max: %d",
			__monitor_ports_cur, __monitor_ports_max);
		return __monitor_ports_cur;
	}

	__monitor_ports[__monitor_ports_cur] = port;
	__monitor_ports_cur++;
	return __monitor_ports_cur;
}

static int should_check(int port)
{
	int i;

	if (port <= 0) {
		acl_msg_error("%s(%d), %s: invalid port: %d",
			__FILE__, __LINE__, __FUNCTION__, port);
		return 0;
	}

	for (i = 0; i < __monitor_ports_cur; i++) {
		if (__monitor_ports[i] == port)
			return 1;
	}

	return 0;
}

int close(int fd)
{
	char peer[64], *at, buf[256];

	if (fd < 0) {
		acl_msg_error("%s(%d), %s: invalid fd: %d",
			__FILE__, __LINE__, __FUNCTION__, fd);
		trace_info("invalid fd");
		return -1;
	}

	if (__monitor_ports_cur == 0
		|| acl_getpeername(fd, peer, sizeof(peer)) == -1)
	{
		peer[0] = 0;
	} else if ((at = strchr(peer, ':')) && *++at && should_check(atoi(at))) {
		snprintf(buf, sizeof(buf), "closing %s", peer);
		trace_info(buf);
	}

	if (__sys_close(fd) == 0)
		return 0;

	snprintf(buf, sizeof(buf), "close error %s, fd=%d, peer=%s",
		acl_last_serror(), fd, peer);
	trace_info(buf);

	return -1;
}
