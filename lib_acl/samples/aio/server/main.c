#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#ifdef ACL_MS_WINDOWS
#include <process.h>
#endif
#include "echo_server.h"

static int  __use_slice = 0;
static int  __nrunner = 1;
static int  __accept_auto = 0;
static int  __send_size = 100;
static int  __line_length = 0;
static char *__data;
static int  __echo_src;
static int  __event_mode = ACL_EVENT_KERNEL;

static char *__listen_addr;

static void init(void)
{
#ifdef	USE_LOG
	char logfile[] = "test.log";
	char logpre[] = "thread_test";
#endif

	int   i;

	if (__use_slice)
		acl_mem_slice_init(8, 10240, 100000,
			ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF | ACL_SLICE_FLAG_LP64_ALIGN);

	acl_lib_init();
#ifdef	USE_LOG
	acl_msg_open(logfile, logpre);
#endif

	if (__send_size <= 0)
		__send_size = 100;

	__data = acl_mycalloc(1, __send_size);
	assert(__data);

	for (i = 0; i < __send_size - 2; i++) {
		__data[i] = 'i';
	}

	__data[i++] = '\n';
	__data[i] = 0;

	echo_server_init(__data, (int) strlen(__data),
		__echo_src, __line_length);
}

static void gc_timer(int event_type acl_unused, ACL_EVENT *event acl_unused,
	void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;

	acl_mem_slice_delay_destroy();
	/* 设定定时器定时清理垃圾回收器 */
	acl_aio_request_timer(aio, gc_timer, aio, 2, 0);
}

static void *__runner_loop(void *arg)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) arg;
	ACL_AIO *aio;

	aio = echo_server_start(sstream, __accept_auto, __event_mode);

	printf("start one thread ok, id=%lu, pid=%d\n",
		(unsigned long) acl_pthread_self(), (int) getpid());

	/* 设定定时器定时清理垃圾回收器 */
	if (__use_slice)
		acl_aio_request_timer(aio, gc_timer, aio, 2, 0);

	while (1) {
		acl_aio_loop(aio);
	}

	/* not reached */
	acl_vstream_close(sstream);

	return (NULL);
}

static void __proccess_running(ACL_VSTREAM *sstream, int nrunner)
{
#ifdef ACL_UNIX
	int   i;

	if (nrunner <= 1)
		__runner_loop(sstream);
	else {
		for (i = 0; i < nrunner; i++) {
			switch (fork()) {
			case 0:  /* child */
				__runner_loop(sstream);
				exit (0);
			case -1:
				exit (1);
			default:  /* parent */
				break;
			}
		}
	}

#elif defined(ACL_MS_WINDOWS)
	__runner_loop(sstream);
#endif
}

static void run(void)
{
	ACL_VSTREAM *sstream;

	sstream = acl_vstream_listen(__listen_addr, 256);
	assert(sstream);

	__proccess_running(sstream, __nrunner);
	acl_vstream_close(sstream);
}

static void usage(const char *progname)
{
	printf("usage: %s -h(help)\r\n"
		"	-a(accept auto)\r\n"
		"	-n instances\r\n"
		"	-s listen_addr(ip:port)\r\n"
		"	-m event_type (select|kernel|poll)\r\n"
		"	-P [use mempool]\r\n"
		"	-l echo_size\r\n"
		"	-L max_line_size\r\n"
		"	-e(echo src data)\n", progname);
}

int main(int argc, char *argv[])
{
	char  ch;

#if 0
	int base = 8, nslice = 1024, nalloc_gc = 1000000;
	unsigned int slice_flag = ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF;

	acl_mem_slice_init(base, nslice, nalloc_gc, slice_flag);
#endif

	while ((ch = getopt(argc, argv, "hPeam:s:n:l:L:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			exit (0);
		case 'm':
			if (strcasecmp(optarg, "select") == 0)
				__event_mode = ACL_EVENT_SELECT;
			else if (strcasecmp(optarg, "kernel") == 0)
				__event_mode = ACL_EVENT_KERNEL;
			else if (strcasecmp(optarg, "poll") == 0)
				__event_mode = ACL_EVENT_POLL;
			else
				__event_mode = ACL_EVENT_SELECT;
			break;
		case 'a':
			__accept_auto = 1;
			break;
		case 's':
			__listen_addr = acl_mystrdup(optarg);
			break;
		case 'n':
			__nrunner = atoi(optarg);
			break;
		case 'l':
			__send_size = atoi(optarg);
			break;
		case 'L':
			__line_length = atoi(optarg);
			break;
		case 'e':
			__echo_src = 1;
			break;
		case 'P':
			__use_slice = 1;
			break;
		default:
			break;
		}
	}

	if (__listen_addr == NULL)
		__listen_addr = acl_mystrdup("0.0.0.0:30082");

	printf("listen=%s\n", __listen_addr);
	acl_msg_stdout_enable(1);
	init();
	run();
	exit (0);
}
