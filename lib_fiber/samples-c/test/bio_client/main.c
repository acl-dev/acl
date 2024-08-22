#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"
#include "fiber_client.h"
#include "stamp.h"

static char __server_ip[64];
static int  __server_port = 9001;

static long long int __total_count = 0;
static int __total_clients         = 0;
static int __total_error_clients   = 0;

static int __fiber_delay  = 0;
static int __conn_timeout = 0;
static int __max_loop     = 10000;
static int __max_fibers   = 1;
static int __left_fibers  = 1;
static int __read_data    = 1;
static struct timeval __begin;

static void fiber_connect(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int  fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sa;
	socklen_t len = (socklen_t) sizeof(sa);

	assert(fd >= 0);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(__server_port);
	sa.sin_addr.s_addr = inet_addr(__server_ip);

	if (__fiber_delay > 0)
		acl_fiber_delay(__fiber_delay);

	if (connect(fd, (const struct sockaddr *) &sa, len) < 0) {
		close(fd);

		printf("fiber-%d: connect %s:%d error %s\r\n",
			acl_fiber_self(), __server_ip, __server_port,
			strerror(errno));

		__total_error_clients++;
	} else {
		__total_clients++;
		printf("fiber-%d: connect %s:%d ok, clients: %d, fd: %d\r\n",
			acl_fiber_self(), __server_ip, __server_port,
			__total_clients, fd);

		echo_client(fd);
	}

	--__left_fibers;
	printf("max_fibers: %d, left: %d\r\n", __max_fibers, __left_fibers);

	if (__left_fibers == 0) {
		double spent;
		struct timeval end;

		gettimeofday(&end, NULL);
		spent = stamp_sub(&end, &__begin);

		printf("fibers: %d, clients: %d, error: %d, count: %lld, "
			"spent: %.2f ms, speed: %.2f tps\r\n", __max_fibers,
			__total_clients, __total_error_clients,
			__total_count, spent,
			(__total_count * 1000) / (spent > 0 ? spent : 1));

		acl_fiber_schedule_stop();
	}
}

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int i;

	for (i = 0; i < __max_fibers; i++)
		acl_fiber_create(fiber_connect, NULL, __stack_size);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s server_ip\r\n"
		" -p server_port\r\n"
		" -t connt_timeout\r\n"
		" -r rw_timeout\r\n"
		" -c max_fibers\r\n"
		" -S [if using single IO, dafault: no]\r\n"
		" -d fiber_delay_ms\r\n"
		" -z stack_size\r\n"
		" -l max_length\r\n"
		" -n max_loop\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
       
	acl_msg_stdout_enable(1);
	signal(SIGPIPE, SIG_IGN);

	snprintf(__server_ip, sizeof(__server_ip), "%s", "127.0.0.1");

	while ((ch = getopt(argc, argv, "hc:n:s:p:t:r:Sd:z:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__max_fibers = atoi(optarg);
			__left_fibers = __max_fibers;
			break;
		case 't':
			__conn_timeout = atoi(optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 's':
			snprintf(__server_ip, sizeof(__server_ip), "%s", optarg);
			break;
		case 'p':
			__server_port = atoi(optarg);
			break;
		case 'S':
			__read_data = 0;
			break;
		case 'd':
			__fiber_delay = atoi(optarg);
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		case 'l':
			__max_length = atoi(optarg);
			break;
		default:
			break;
		}
	}

	gettimeofday(&__begin, NULL);

	acl_fiber_create(fiber_main, NULL, 32768);

	printf("call fiber_schedule\r\n");

	acl_fiber_schedule();

	return 0;
}
