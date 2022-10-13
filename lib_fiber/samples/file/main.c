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
	int   fd = open(path, __open_flags, 0600), ret;
	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		return;
	}

	printf("open %s ok, fd=%d\r\n", path, fd);

	while (1) {
		char buf[1024];
		ret = read(fd, buf, sizeof(buf) - 1);
		if (ret <= 0) {
			printf("Read over!\r\n");
			break;
		}

		buf[ret] = 0;
		printf("%s", buf);
		fflush(stdout);
	}

	ret = close(fd);
	printf("close %s %s, fd=%d\r\n", path, ret == 0 ? "ok" : "error", fd);
}

static void fiber_writefile(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	int   fd = open(path, __open_flags, 0600), i, ret;
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
	ret = close(fd);
	printf("close %s %s, fd=%d\r\n", path, ret == 0 ? "ok" : "error", fd);
}

static void fiber_filestat(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	struct stat statbuf;

	if (stat(path, &statbuf) == -1) {
		printf("stat %s error %s\r\n", path, strerror(errno));
		return;
	}

	printf("stat %s ok\r\n", path);
	printf("size=%ld, atime=%ld, mtime=%ld, ctime=%ld\r\n",
		statbuf.st_size, statbuf.st_atime, statbuf.st_mtime,
		statbuf.st_ctime);
}

struct FIBER_CTX {
	const char *oldpath;
	const char *newpath;
};

static void fiber_rename(ACL_FIBER *fiber acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;

	if (rename(fc->oldpath, fc->newpath) == -1) {
		printf("rename from %s to %s error %s\r\n",
			fc->oldpath, fc->newpath, strerror(errno));
		return;
	}

	printf("rename from %s to %s ok\r\n", fc->oldpath, fc->newpath);
}

static void fiber_unlink(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	int ret = unlink(path);
	if (ret == 0) {
		printf("unlink %s ok\r\n", path);
	} else {
		printf("unlink %s error %s\r\n", path, strerror(errno));
	}
}

static void fiber_mkdir(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *path = (const char*) ctx;
	int ret = acl_make_dirs(path, 0755);

	if (ret == -1) {
		printf("acl_make_dirs %s error %s\r\n", path, strerror(errno));
	} else {
		printf("acl_make_dirs %s ok\r\n", path);
	}
}

static void usage(const char *proc)
{
	printf("usage: %s -h [help]\r\n"
		"  -f filepath\r\n"
		"  -t tofilepath\r\n"
		"  -a action[read|write|unlink|stat|mkdir]\r\n"
		"  -n write_size[default: 1024]\r\n"
		"  -o open_flags[O_RDONLY, O_WRONLY, O_RDWR, O_APPEND, O_CREAT, O_EXCL, O_TRUNC]\r\n"
		, proc);
}

int main(int argc, char *argv[])
{
	int ch;
	char buf[256], buf2[256], action[128];
	struct FIBER_CTX ctx;

	buf[0]    = 0;
	action[0] = 0;
	ctx.oldpath = NULL;
	ctx.newpath = NULL;

#define	EQ(x, y)	!strcasecmp((x), (y))

	while ((ch = getopt(argc, argv, "hf:t:a:o:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(buf, sizeof(buf), "%s", optarg);
			ctx.oldpath = buf;
			break;
		case 't':
			snprintf(buf2, sizeof(buf2), "%s", optarg);
			ctx.newpath = buf2;
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
	} else if (EQ(action, "unlink")) {
		acl_fiber_create(fiber_unlink, buf, 320000);
	} else if (EQ(action, "stat")) {
		acl_fiber_create(fiber_filestat, buf, 320000);
	} else if (EQ(action, "rename")) {
		acl_fiber_create(fiber_rename, &ctx, 320000);
	} else if (EQ(action, "mkdir")) {
		acl_fiber_create(fiber_mkdir, buf, 320000);
	} else {
		printf("Unknown action=%s\r\n", action);
		usage(argv[0]);
	}

	acl_fiber_schedule_with(FIBER_EVENT_IO_URING);
	return 0;
}
