#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"
#include "fiber_client.h"

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
		int cfd = accept(lfd, (struct sockaddr *)& sa,
				(socklen_t *) &len);
		if (cfd < 0) {
			printf("accept error %s\r\n", strerror(errno));
			break;
		}

		fd = malloc(sizeof(int));
		assert(fd != NULL);
		*fd = cfd;

		__nconnect++;
		printf("accept one, fd: %d\r\n", cfd);
		acl_fiber_create(fiber_client, fd, __stack_size);
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
	printf("fiber_schedule stopped\r\n");

	return 0;
}
