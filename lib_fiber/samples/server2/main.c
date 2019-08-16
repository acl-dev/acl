#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#if !defined(_WIN32) && !defined(_WIN64)
#include <unistd.h>
#include <poll.h>
#endif
#include "lib_acl.h"
#include "fiber/libfiber.h"

#if defined(_WIN32) || defined(_WIN64)
# define POLL	WSAPoll
# define CLOSE	acl_fiber_close
# define LISTEN	acl_fiber_listen
# define ACCEPT	acl_fiber_accept
# define snprintf _snprintf
#else
# define SOCKET	int
# define INVALID_SOCKET -1
# define POLL	poll
# define CLOSE	close
# define LISTEN	listen
# define ACCEPT accept
#endif

static int  __nconnect = 0;
static int  __count = 0;
static int  __socket_count = 0;
static char __listen_ip[64];
static int  __listen_port = 9001;
static int  __listen_qlen = 64;
static int  __rw_timeout = 0;
static int  __echo_data  = 1;
static int  __stack_size = 320000;

static int check_read(int fd, int timeout)
{
	struct pollfd pfd;
	int n;

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLIN;

	n = POLL(&pfd, 1, timeout);
	if (n < 0) {
		printf("poll error: %s\r\n", acl_last_serror());
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
	SOCKET  *pfd = (SOCKET *) ctx;
	SOCKET   fd  = *pfd;
	char  buf[8192];
	int   ret;

	__socket_count++;
	//printf("client fiber-%d: fd: %d\r\n", acl_fiber_self(), fd);

	while (1) {
		if (__rw_timeout > 0) {
			ret = check_read(fd, __rw_timeout * 1000);
			if (ret < 0)
				break;
			if (ret == 0) {
				printf("read timeout fd=%u\r\n", fd);
				break;
			}
		}

#if defined(_WIN32) || defined(_WIN64)
		ret = acl_fiber_recv(fd, buf, sizeof(buf) - 1, 0);
#else
		ret = read(fd, buf, sizeof(buf) - 1);
#endif
		if (ret == 0) {
			printf("read close by peer fd: %d, %s\r\n",
				fd, acl_last_serror());
			break;
		} else if (ret < 0) {
			if (acl_last_error() == EINTR) {
				printf("catch a EINTR signal\r\n");
				continue;
			}

			printf("read error %s, fd: %u\n", acl_last_serror(), fd);
			break;
		}

		// buf[ret] = 0; printf("buf=%s\r\n", buf);
		__count++;

		if (!__echo_data)
			continue;

#if defined(_WIN32) || defined(_WIN64)
		if (acl_fiber_send(fd, buf, ret, 0) < 0) {
#else
		if (write(fd, buf, ret) < 0) {
#endif
			if (errno == EINTR)
				continue;
			printf("write error, fd: %d\r\n", fd);
			break;
		}
	}

	__socket_count--;
	printf("%s: close %d, socket_count=%d\r\n",
		__FUNCTION__, fd, __socket_count);
	CLOSE(fd);
	free(pfd);

	if (--__nconnect == 0) {
		printf("\r\n----total read/write: %d----\r\n", __count);
		__count = 0;
	}
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	SOCKET lfd;
	int on = 1;
	struct sockaddr_in sa;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons(__listen_port);
	sa.sin_addr.s_addr = inet_addr(__listen_ip);

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == INVALID_SOCKET)
		abort();

#if defined(_WIN32) || defined(_WIN64)
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (const char *) &on, sizeof(on))) {
#else
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
#endif
		printf("setsockopt error %s\r\n", acl_last_serror());
		exit (1);
	}

	if (bind(lfd, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
		printf("bind error %s\r\n", acl_last_serror());
		exit (1);
	}

	if (LISTEN(lfd, 128) < 0) {
		printf("listen error %s\r\n", acl_last_serror());
		exit (1);
	}

	printf("fiber-%d listen %s:%d ok\r\n",
		acl_fiber_self(), __listen_ip, __listen_port);

	for (;;) {
		int len = sizeof(sa);
		SOCKET *pfd;
		SOCKET cfd = ACCEPT(lfd, (struct sockaddr *)& sa, (socklen_t *) &len);
		if (cfd == INVALID_SOCKET) {
			printf("accept error %s\r\n", acl_last_serror());
			break;
		}

		pfd = malloc(sizeof(SOCKET));
		assert(pfd != NULL);
		*pfd = cfd;

		__nconnect++;
		printf("accept one, fd: %u, %p\r\n", cfd, pfd);
		acl_fiber_create(echo_client, pfd, __stack_size);
	}

	CLOSE(lfd);
	exit(0);
}

//#define SCHEDULE_AUTO

#ifndef	SCHEDULE_AUTO
static void fiber_memcheck(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	while (1) {
#if defined(_WIN32) || defined(_WIN64)
		acl_fiber_delay(1000);
#else
		sleep(1);
#endif
		acl_default_meminfo();
		acl_fiber_memstat();
	}
}
#endif

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -e event_mode [kernel|select|poll]\r\n"
		" -s listen_ip\r\n"
		" -p listen_port\r\n"
		" -r rw_timeout\r\n"
		" -q listen_queue\r\n"
		" -z stack_size\r\n"
		" -S [if using single IO, default: no]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, event_mode = FIBER_EVENT_KERNEL;

	snprintf(__listen_ip, sizeof(__listen_ip), "%s", "127.0.0.1");

	while ((ch = getopt(argc, argv, "hs:p:r:q:Sz:e:")) > 0) {
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
		case 'e':
			if (strcasecmp(optarg, "select") == 0)
				event_mode = FIBER_EVENT_SELECT;
			else if (strcasecmp(optarg, "poll") == 0)
				event_mode = FIBER_EVENT_POLL;
			break;
		default:
			break;
		}
	}

#if !defined(_WIN32) && !defined(_WIN64)
	signal(SIGPIPE, SIG_IGN);
#endif
	acl_lib_init();
	acl_msg_stdout_enable(1);
	acl_fiber_msg_stdout_enable(1);

#ifdef	SCHEDULE_AUTO
	acl_fiber_schedule_init(1);
	acl_fiber_schedule_set_event(event_mode);
#endif

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_accept, NULL, 327680);

#ifndef	SCHEDULE_AUTO
	acl_fiber_create(fiber_memcheck, NULL, 640000);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule_with(event_mode);
#endif

	return 0;
}
