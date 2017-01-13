#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"

static int  __nconnect = 0;
static int  __count = 0;
static char __listen_ip[64];
static int  __listen_port = 9001;
static int  __listen_qlen = 64;
static int  __rw_timeout = 0;
static int  __echo_data  = 1;
static int  __stack_size = 32000;

static int check_read(int fd, int timeout)
{
	struct pollfd pfd;
	int n;

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLIN;

	n = poll(&pfd, 1, timeout);
	if (n < 0) {
		printf("poll error: %s\r\n", strerror(errno));
		return -1;
	}

	if (n == 0)
		return 0;
	if (pfd.revents & POLLIN)
		return 1;
	else
		return 0;
}

static void echo_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	int  *cfd = (int *) ctx;
	char  buf[8192];
	int   ret;

	printf("client fiber-%d: fd: %d\r\n", acl_fiber_self(), *cfd);

	while (1) {
		if (__rw_timeout > 0) {
			ret = check_read(*cfd, 10000);
			if (ret < 0)
				break;
			if (ret == 0)
				continue;
		}

		ret = read(*cfd, buf, sizeof(buf));
		if (ret == 0) {
			printf("read close by peer fd: %d, %s\r\n",
				*cfd, strerror(errno));
			break;
		} else if (ret < 0) {
			if (errno == EINTR) {
				printf("catch a EINTR signal\r\n");
				continue;
			}

			printf("read error %s, fd: %d\n", strerror(errno), *cfd);
			break;
		}

		__count++;

		if (!__echo_data)
			continue;

		if (write(*cfd, buf, ret) < 0) {
			if (errno == EINTR)
				continue;
			printf("write error, fd: %d\r\n", *cfd);
			break;
		}
	}

	printf("close %d\r\n", *cfd);
	close(*cfd);
	free(cfd);

	if (--__nconnect == 0) {
		printf("\r\n----total read/write: %d----\r\n", __count);
		__count = 0;
	}
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	int  lfd, on = 1;
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(__listen_port);
	sa.sin_addr.s_addr = inet_addr(__listen_ip);

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd < 0)
		abort();

	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		printf("setsockopt error %s\r\n", strerror(errno));
		exit (1);
	}

	if (bind(lfd, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
		printf("bind error %s\r\n", strerror(errno));
		exit (1);
	}

	if (listen(lfd, 128) < 0) {
		printf("listen error %s\r\n", strerror(errno));
		exit (1);
	}

	printf("fiber-%d listen %s:%d ok\r\n",
		acl_fiber_self(), __listen_ip, __listen_port);

	for (;;) {
		int len = sizeof(sa), *fd;
		int cfd = accept(lfd, (struct sockaddr *)& sa, (socklen_t *) &len);
		if (cfd < 0) {
			printf("accept error %s\r\n", strerror(errno));
			break;
		}

		fd = malloc(sizeof(int));
		assert(fd != NULL);
		*fd = cfd;

		__nconnect++;
		printf("accept one, fd: %d\r\n", cfd);
		acl_fiber_create(echo_client, fd, __stack_size);
	}

	close(lfd);
	exit(0);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"  -s listen_ip\r\n"
		"  -p listen_port\r\n"
		"  -r rw_timeout\r\n"
		"  -q listen_queue\r\n"
		"  -z stack_size\r\n"
		"  -S [if using single IO, default: no]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch;

	snprintf(__listen_ip, sizeof(__listen_ip), "%s", "127.0.0.1");

	while ((ch = getopt(argc, argv, "hs:p:r:q:Sz:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__listen_ip, sizeof(__listen_ip), "%s", optarg);
			break;
		case 'p':
			__listen_port = atoi(optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'q':
			__listen_qlen = atoi(optarg);
			break;
		case 'S':
			__echo_data = 0;
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		default:
			break;
		}
	}

	signal(SIGPIPE, SIG_IGN);
	acl_msg_stdout_enable(1);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_accept, NULL, 32768);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule();

	return 0;
}
