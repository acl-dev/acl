#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"

static char __server_ip[64];
static int  __server_port = 9001;

static int __total_clients         = 0;
static int __total_error_clients   = 0;

static int __fiber_delay  = 10; 
static int __max_fibers   = 100;
static int __left_fibers  = 100;
static int __stack_size   = 32000;

static void echo_client(int fd)
{
	char  buf[8192];
	int   ret = read(fd, buf, sizeof(buf));

	if (ret <= 0)
		printf("read error: %s, ret: %d\r\n", strerror(errno), ret);
	else
		printf("read some data, ret: %d\r\n", ret);

	close(fd);
}

static void fiber_connect(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int  fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sa;
	socklen_t len = (socklen_t) sizeof(sa);

	assert(fd >= 0);

	if (__fiber_delay > 0)
		acl_fiber_delay(__fiber_delay);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(__server_port);
	sa.sin_addr.s_addr = inet_addr(__server_ip);

	if (connect(fd, (const struct sockaddr *) &sa, len) < 0) {
		printf("fiber-%d: connect %s:%d error %s\r\n",
			acl_fiber_self(), __server_ip, __server_port,
			strerror(errno));

		close(fd);
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

	if (__left_fibers == 0)
		acl_fiber_stop();
}

static void create_fibers(void)
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
		" -d fiber_delay_ms\r\n"
		" -z stack_size\r\n"
		" -c max_fibers\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;
       
	acl_msg_stdout_enable(1);
	signal(SIGPIPE, SIG_IGN);

	snprintf(__server_ip, sizeof(__server_ip), "%s", "127.0.0.1");

	while ((ch = getopt(argc, argv, "hc:s:p:d:z:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__max_fibers = atoi(optarg);
			__left_fibers = __max_fibers;
			break;
		case 's':
			snprintf(__server_ip, sizeof(__server_ip), "%s", optarg);
			break;
		case 'p':
			__server_port = atoi(optarg);
			break;
		case 'd':
			__fiber_delay = atoi(optarg);
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		default:
			break;
		}
	}

	create_fibers();

	printf("call fiber_schedule\r\n");

	acl_fiber_schedule();

	return 0;
}
