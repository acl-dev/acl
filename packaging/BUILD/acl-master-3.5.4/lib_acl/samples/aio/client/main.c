#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "echo_client.h"

static int  __max_threads = 10;
static int  __nconnect = 500;
static int  __nloop = 1000;
static char *__svr_addr;
static int  __dlen = 1024;
static int  __timeout = 120;
static int  __nconn_per_sec = 100;
static int  __event_mode;

static void init(int use_slice)
{
#if 0
	char logfile[] = "test.log";
	char logpre[] = "thread_test";

	acl_msg_open(logfile, logpre);
#endif

	if (use_slice)
		acl_mem_slice_init(8, 10240, 100000, ACL_SLICE_FLAG_GC2
			| ACL_SLICE_FLAG_RTGC_OFF | ACL_SLICE_FLAG_LP64_ALIGN);

	acl_lib_init();
	echo_client_init(ECHO_CTL_SERV_ADDR, __svr_addr,
			ECHO_CTL_MAX_CONNECT, __nconnect,
			ECHO_CTL_MAX_LOOP, __nloop,
			ECHO_CTL_TIMEOUT, __timeout,
			ECHO_CTL_DATA_LEN, __dlen,
			ECHO_CTL_NCONN_PERSEC, __nconn_per_sec,
			ECHO_CTL_EVENT_MODE, __event_mode,
			ECHO_CTL_END);
}

static void run(int use_slice)
{
	printf("server started(max=%d) OK, listen=%s\n",
		__max_threads, __svr_addr);
	echo_client_start(use_slice);
}

static void usage(const char *progname)
{
	printf("usage: %s -h (help)\r\n"
		"	-s server_addr(ip:port)\r\n"
		"	-m event (select|poll|kernel)\r\n"
		"	-P [use mempool]\r\n"
		"	-n nconnect\r\n"
		"	-l nloop\r\n"
		"	-d dlen\r\n"
		"	-c timeout\r\n"
		"	-p nconn_per_sec\r\n", progname);
}

int main(int argc, char *argv[])
{
	char  ch;
	int   use_slice = 0;

	while ((ch = getopt(argc, argv, "hPm:s:n:l:d:c:p:")) > 0) {
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
		case 'n':
			__nconnect = atoi(optarg);
			break;
		case 'l':
			__nloop = atoi(optarg);
			break;
		case 's':
			__svr_addr = acl_mystrdup(optarg);
			break;
		case 'd':
			__dlen = atoi(optarg);
			break;
		case 'c':
			__timeout = atoi(optarg);
			break;
		case 'p':
			__nconn_per_sec = atoi(optarg);
			break;
		case 'P':
			use_slice = 1;
			break;
		default:
			usage(argv[0]);
			exit (0);
		}
	}

	if (__max_threads < 0)
		__max_threads = 0;
	if (__svr_addr == NULL)
		__svr_addr = acl_mystrdup("127.0.0.1:30082");

	init(use_slice);
	run(use_slice);

	acl_myfree(__svr_addr);

	return 0;
}
