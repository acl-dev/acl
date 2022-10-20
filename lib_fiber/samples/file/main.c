
#define	_GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <sys/sendfile.h>
#include "lib_acl.h"
#include "fiber/libfiber.h"

static int __open_flags = 0;
static int __write_size = 1024;

struct FIBER_CTX {
	const char *frompath;
	const char *topath;
	off_t off;
	int   len;
};

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
		char buf[8192];
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
	int   fd = open(path, __open_flags, 0600), i, n;
	char  buf[10];
	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		return;
	}

	printf("open %s ok, fd=%d\r\n", path, fd);

	for (i = 0; i < __write_size; i++) {
		n = i % 10;
		snprintf(buf, sizeof(buf), "%d", n);
		if (write(fd, buf, 1) <= 0) {
			printf("write to %s error %s\r\n", path, strerror(errno));
			break;
		}
	}

	(void) write(fd, "\r\n", 2);
	printf("write over!\r\n");
	n = close(fd);
	printf("close %s %s, fd=%d\r\n", path, n == 0 ? "ok" : "error", fd);
}

static void fiber_pread(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const struct FIBER_CTX *fc = (const struct FIBER_CTX*)ctx;
	int   fd = open(fc->frompath, O_RDONLY, 0600), ret;
	char  buf[4096];
	int len = (int) sizeof(buf) - 1 > fc->len ? fc->len : (int) sizeof(buf) - 1;

	if (fd == -1) {
		printf("open %s error %s\r\n", fc->frompath, strerror(errno));
		return;
	}

	ret = pread(fd, buf, len, fc->off);
	if (ret < 0) {
		printf("pread from %s %d error %s\r\n",
			fc->frompath, fd, strerror(errno));
	} else {
		buf[ret] = 0;
		printf("%s\r\n", buf);
	}

	close(fd);
}

static void wait_and_sendfile(int in, struct FIBER_CTX *fc)
{
	const char *addr = "127.0.0.1:8080";
	ACL_VSTREAM *ln = acl_vstream_listen(addr, 128), *conn;
	ssize_t ret;
	off_t off_saved = fc->off;
	int cfd;
	if (ln == NULL) {
		printf("listen %s error %s\r\n", addr, strerror(errno));
		exit(1);
	}

	while (1) {
		printf("Waiting for accept from %s ...\r\n", addr);

		conn = acl_vstream_accept(ln, NULL, 0);
		if (conn == NULL) {
			printf("accept from %s error %s\r\n", addr, strerror(errno));
			exit(1);
		}

		cfd = ACL_VSTREAM_SOCK(conn);
		acl_vstream_free(conn);

		printf(">>>begin call sendfile64 to fd=%d\r\n", cfd);

		ret = sendfile64(cfd, in, &fc->off, fc->len);
		close(cfd);

		printf(">>>sendfile ret=%zd, off=%d\r\n", ret, (int) fc->off);

		fc->off = off_saved;
	}

	acl_vstream_close(ln);
}

static void fiber_sendfile(ACL_FIBER *fiber acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	int in = open(fc->frompath, O_RDONLY, 0600);

	if (in == -1) {
		printf("open %s error %s\r\n", fc->frompath, strerror(errno));
		return;
	}

	wait_and_sendfile(in, fc);
	close(in);
}

static void fiber_splice(ACL_FIBER *fiber acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	int in = open(fc->frompath, O_RDONLY, 0600), out, total = 0, loop = 0;
	int pipefd[2];

	if (in == -1) {
		printf("open %s error %s\r\n", fc->frompath, strerror(errno));
		return;
	}

	if (pipe(pipefd) == -1) {
		printf("create pipe error %s\r\n", strerror(errno));
		exit(1);
	}
	out = pipefd[1];

	while (1) {
		char buf[1024];
		int flags = SPLICE_F_MOVE | SPLICE_F_MORE;
		ssize_t ret = splice(in, &fc->off, out, NULL, fc->len, flags);

		if (ret <= 0) {
			printf("splice over, ret=%zd: %s\r\n", ret, strerror(errno));
			break;
		}

		while (ret > 0) {
			size_t size = sizeof(buf) - 1 > (size_t) ret
				? (size_t) ret : sizeof(buf) - 1;
			ssize_t n = read(pipefd[0], buf, size);
			if (n <= 0) {
				printf("pipe over, ret=%zd: %s\r\n",
					ret, strerror(errno));
				break;
			}

			buf[n] = 0;
			printf("%s", buf);
			fflush(stdout);
			ret -= n;
			total += n;
			loop++;
		}
	}

	printf("Total read %d bytes, loop=%d\r\n", total, loop);
	close(in);
	close(pipefd[0]);
	close(pipefd[1]);
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

static void fiber_rename(ACL_FIBER *fiber acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;

	if (rename(fc->frompath, fc->topath) == -1) {
		printf("rename from %s to %s error %s\r\n",
			fc->frompath, fc->topath, strerror(errno));
		return;
	}

	printf("rename from %s to %s ok\r\n", fc->frompath, fc->topath);
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
		"  -a action[read|write|rename|unlink|stat|mkdir|splice|pread|pwrite|sendfile]\r\n"
		"  -n size[default: 1024]\r\n"
		"  -o open_flags[O_RDONLY, O_WRONLY, O_RDWR, O_APPEND, O_CREAT, O_EXCL, O_TRUNC]\r\n"
		"  -p offset\r\n"
		, proc);
}

int main(int argc, char *argv[])
{
	int ch;
	char buf[256], buf2[256], action[128];
	struct FIBER_CTX ctx;

	snprintf(buf, sizeof(buf), "from.txt");
	snprintf(buf2, sizeof(buf2), "to.txt");

	action[0]    = 0;
	ctx.frompath = buf;
	ctx.topath   = buf2;
	ctx.off      = 0;
	ctx.len      = 100;

#define	EQ(x, y)	!strcasecmp((x), (y))

	while ((ch = getopt(argc, argv, "hf:t:a:o:n:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(buf, sizeof(buf), "%s", optarg);
			ctx.frompath = buf;
			break;
		case 't':
			snprintf(buf2, sizeof(buf2), "%s", optarg);
			ctx.topath = buf2;
			break;
		case 'p':
			ctx.off = atoi(optarg);
			break;
		case 'a':
			snprintf(action, sizeof(action), "%s", optarg);
			break;
		case 'n':
			__write_size = atoi(optarg);
			ctx.len = atoi(optarg);
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

	//acl_fiber_msg_stdout_enable(1);
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
	} else if (EQ(action, "splice")) {
		acl_fiber_create(fiber_splice, &ctx, 320000);
	} else if (EQ(action, "pread")) {
		acl_fiber_create(fiber_pread, &ctx, 320000);
	} else if (EQ(action, "sendfile")) {
		acl_fiber_create(fiber_sendfile, &ctx, 320000);
	} else {
		printf("Unknown action=%s\r\n", action);
		usage(argv[0]);
	}

	acl_fiber_schedule_with(FIBER_EVENT_IO_URING);

	printf("Enter any key to exit ...\r\n");
	getchar();
	return 0;
}
