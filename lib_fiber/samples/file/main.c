#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include "lib_acl.h"
#include "fiber/libfiber.h"

static int __open_flags = 0;
static int __write_size = 1024;

static void fiber_readfile(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	int   fd = open(path, __open_flags, 0600);
	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		return;
	}

	printf("open %s ok, fd=%d\r\n", path, fd);

	while (1) {
		char buf[1024];
		int  ret = read(fd, buf, sizeof(buf) - 1);
		if (ret <= 0) {
			printf("Read over!\r\n");
			break;
		}

		buf[ret] = 0;
		printf("%s", buf);
		fflush(stdout);
	}

	close(0);
}

static void fiber_writefile(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	int   fd = open(path, __open_flags, 0600), i;
	char  buf[1];
	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		return;
	}

	printf("open %s ok, fd=%d\r\n", path, fd);

	buf[0] = 'x';
	for (i = 0; i < __write_size; i++) {
		if (write(fd, buf, 1) <= 0) {
			printf("write to %s error %s\r\n", path, strerror(errno));
			break;
		}
	}

	(void) write(fd, "\r\n", 2);
	close(fd);
}

static void usage(const char *proc)
{
	printf("usage: %s -h [help]\r\n"
		"  -f filepath\r\n"
		"  -a action[read|write]\r\n"
		"  -n write_size[default: 1024]\r\n"
		"  -o open_flags[O_RDONLY, O_WRONLY, O_RDWR, O_APPEND, O_CREAT, O_EXCL, O_TRUNC]\r\n"
		, proc);
}

int main(int argc, char *argv[])
{
	int ch;
	char buf[256], action[128];

	buf[0]    = 0;
	action[0] = 0;

#define	EQ(x, y)	!strcasecmp((x), (y))

	while ((ch = getopt(argc, argv, "hf:a:o:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(buf, sizeof(buf), "%s", optarg);
			break;
		case 'a':
			snprintf(action, sizeof(action), "%s", optarg);
			break;
		case 'n':
			__write_size = atoi(optarg);
			break;
		case 'o':
			if (EQ(optarg, "O_RDONLY")) {
				__open_flags |= O_RDONLY;
				snprintf(action, sizeof(action), "read");
			} else if (EQ(optarg, "O_WRONLY")) {
				__open_flags |= O_WRONLY;
				snprintf(action, sizeof(action), "write");
			} else if (EQ(optarg, "O_RDWR")) {
				__open_flags |= O_RDWR;
				snprintf(action, sizeof(action), "write");
			} else if (EQ(optarg, "O_APPEND")) {
				__open_flags |= O_APPEND;
				snprintf(action, sizeof(action), "write");
			} else if (EQ(optarg, "O_CREAT")) {
				__open_flags |= O_CREAT;
			} else if (EQ(optarg, "O_EXCL")) {
				__open_flags |= O_EXCL;
			} else if (EQ(optarg, "O_TRUNC")) {
				__open_flags |= O_TRUNC;
			}
		default:
			break;
		}
	}

	if (buf[0] == 0 || action[0] == 0) {
		usage(argv[0]);
		return 1;
	}

	acl_msg_stdout_enable(1);

	if (__open_flags == 0) {
		__open_flags = O_RDONLY;
	}

	if (EQ(action, "read")) {
		acl_fiber_create(fiber_readfile, buf, 320000);
	} else if (EQ(action, "write")) {
		acl_fiber_create(fiber_writefile, buf, 320000);
	} else {
		printf("Unknown action=%s\r\n", action);
		usage(argv[0]);
	}

	acl_fiber_schedule_with(FIBER_EVENT_IO_URING);
	return 0;
}
