
#define	_GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#ifdef	__APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#elif	defined(__linux__)
#include <sys/sendfile.h>
#endif
#include "lib_acl.h"
#include "fiber/libfiber.h"

static int __open_flags = 0;
static long long __write_size = 1024;

struct FIBER_CTX {
	const char *frompath;
	const char *topath;
	int   fd;
	off_t off;
	long long len;
	char  addr[256];
	int   check;
};

static void build_dummy(char *buf, size_t size)
{
	const char ss[] = "0123456789abcdefghijklmnopqrstuvwxyz\r\n";
	size_t len = sizeof(ss) - 1;
	char *ptr;

	ptr = buf;
	while (size > 0) {
		size_t n = len > size ? size : len;
		memcpy(ptr, ss, n);
		ptr  += n;
		size -= n;
	}
}

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
	int   fd = open(path, __open_flags, 0600);
	long long i, n;
	char  buf[10];
	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		exit(1);
	}

	build_dummy(buf, sizeof(buf));

	printf("open %s ok, fd=%d\r\n", path, fd);

	for (i = 0; i < __write_size; i++) {
		n = i % 10;
		if (write(fd, buf, 1) <= 0) {
			printf("write to %s error %s\r\n", path, strerror(errno));
			break;
		}
	}

	int ret = write(fd, "\r\n", 2);
	if (ret <= 0) {
		printf("write CRLF error %s\r\n", strerror(errno));
	}

	printf("write over!\r\n");
	n = close(fd);
	printf("close %s %s, fd=%d\r\n", path, n == 0 ? "ok" : "error", fd);
}

static void fiber_pread(ACL_FIBER *fiber acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	char  buf[4096];
	int len = (int) sizeof(buf) - 1 > fc->len
		? fc->len : (int) sizeof(buf) - 1;
	int fd, ret;

	assert(fc->len > 0);

	fd = open(fc->frompath, O_RDONLY, 0600);
	if (fd == -1) {
		printf("open %s for read error %s\r\n",
			fc->frompath, strerror(errno));
		return;
	}

	printf(">>%s: begin to call pread from fd=%d\r\n", __FUNCTION__, fd);

	ret = pread(fd, buf, len, fc->off);
	if (ret <= 0) {
		printf("pread from %s %d over %s, ret=%d\r\n",
			fc->frompath, fd, strerror(errno), ret);
	} else {
		buf[ret] = 0;
		printf("%s\r\n", buf);
		printf("pread from %s, ret=%d\r\n", fc->frompath, ret);
	}
	fc->off += ret;

	printf(">>>%s: begin to close fd=%d\r\n", __FUNCTION__, fd);
	close(fd);
}

static void fiber_pwrite(ACL_FIBER *fiber acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	int fd, ret;
	char *buf;

	assert(fc->len > 0);

	fd = open(fc->frompath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd == -1) {
		printf("open %s for write error %s\r\n",
			fc->frompath, strerror(errno));
		exit(1);
	}

	buf = malloc(fc->len);
	build_dummy(buf, fc->len);

	printf(">>%s: begin to call pwrite to fd=%d\r\rn", __FUNCTION__, fd);

	ret = pwrite(fd, buf, fc->len, fc->off);
	printf(">>pwrite ret=%d, file=%s, fd=%d\r\n", ret, fc->frompath, fd);

	close(fd);
}

static void fiber_sleep(ACL_FIBER *fb acl_unused, void *ctx acl_unused)
{
	time_t last, now, delay;
	while (1) {
		time(&last);
		sleep(1);
		time(&now);
		delay = now - last;
		printf(">>fiber-%d wakeup, delay=%ld seconds\r\n",
			acl_fiber_self(), delay);
		assert(delay == 1);
	}
}

struct IO_CTX {
	int   fd;
	off_t off;
	int   len;
	ACL_FIBER_SEM *sem;
};

#define	MB	1000000

static void fiber_one_preader(ACL_FIBER *fb acl_unused, void *ctx)
{
	struct IO_CTX *ic = (struct IO_CTX*) ctx;
	long long count = ic->len / MB, left = ic->len % MB, i;
	char *buf = malloc(MB);
	int   ret;

	for (i = 0; i < count; i++) {
		ret = pread(ic->fd, buf, MB, ic->off);

		if (ret <= 0) {
			printf("pread over ret=%d, wrror=%s\r\n", ret, strerror(errno));
			break;
		}

		ic->off += ret;
	}

	if (left > 0) {
		ret = pread(ic->fd, buf, left, ic->off);
		if (ret <= 0) {
			printf("pread over ret=%d, wrror=%s\r\n", ret, strerror(errno));
		}
	}

	printf("fiber=%d: pread ok, ret=%d, off=%ld, len=%d\r\n",
		acl_fiber_self(), ret, (long) ic->off, ic->len);

	acl_fiber_sem_post(ic->sem);
	free(buf);
	free(ic);
}

static void co_readers(struct FIBER_CTX *fc, int fd)
{
#define	COUNT	10
	long long i, step = fc->len / COUNT, cnt = 0;
	off_t off = 0;
	ACL_FIBER_SEM *sem = acl_fiber_sem_create(0);

	assert(fc->len > 0);
	assert(step > 0);

	for (i = 0; i < COUNT - 1; i++) {
		struct IO_CTX *ic = malloc(sizeof(struct IO_CTX));
		ic->fd  = fd;
		ic->sem = sem;
		ic->len = step;
		ic->off = off;
		off    += step;
		cnt++;

		acl_fiber_create(fiber_one_preader, ic, 320000);
	}

	if (off < fc->len) {
		struct IO_CTX *ic = malloc(sizeof(struct IO_CTX));
		ic->fd  = fd;
		ic->sem = sem;
		ic->len = fc->len - off;
		ic->off = off;
		cnt++;

		acl_fiber_create(fiber_one_preader, ic, 320000);
	}

	for (i = 0; i < cnt; i++) {
		acl_fiber_sem_wait(sem);
	}

	printf("All pread fibers finished!\r\n");
	acl_fiber_sem_free(sem);
}

static void fiber_co_pread(ACL_FIBER *fb acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	int fd, i;

	fd = open(fc->frompath, O_RDONLY, 0600);
	if (fd == -1) {
		printf("open %s for write error %s\r\n",
			fc->frompath, strerror(errno));
		exit(1);
	}

	if (fc->check) {
		acl_fiber_create(fiber_sleep, NULL, 32000);
	}

	for (i = 0; i < 10; i++) {
		co_readers(fc, fd);
	}
	close(fd);
}

static void fiber_one_pwriter(ACL_FIBER *fb acl_unused, void *ctx)
{
	struct IO_CTX *ic = (struct IO_CTX*) ctx;
	int   count = ic->len / MB, left = ic->len % MB;
	char *buf = malloc(MB);
	int   ret, i;
	size_t size = MB;

	build_dummy(buf, size);

	for (i = 0; i < count; i++) {
		ret = pwrite(ic->fd, buf, MB, ic->off);

		if (ret <= 0) {
			printf("pwrite ret=%d, wrror=%s\r\n", ret, strerror(errno));
			exit(1);
		}
		ic->off += ret;
	}

	if (left > 0) {
		ret = pwrite(ic->fd, buf, left, ic->off);
		if (ret <= 0) {
			printf("pwrite ret=%d, wrror=%s\r\n", ret, strerror(errno));
			exit(1);
		}
	}

	printf("fiber=%d: pwrite ok, ret=%d, off=%ld, len=%d\r\n",
		acl_fiber_self(), ret, (long) ic->off, ic->len);

	acl_fiber_sem_post(ic->sem);
	free(buf);
	free(ic);
}

static void co_writers(struct FIBER_CTX *fc, int fd)
{
#define	COUNT	10
	int i, step = fc->len / COUNT, cnt = 0;
	off_t off = 0;
	ACL_FIBER_SEM *sem = acl_fiber_sem_create(0);

	assert(fc->len > 0);
	assert(step > 0);

	for (i = 0; i < COUNT - 1; i++) {
		struct IO_CTX *ic = malloc(sizeof(struct IO_CTX));
		ic->fd  = fd;
		ic->sem = sem;
		ic->len = step;
		ic->off = off;
		off    += step;
		cnt++;

		acl_fiber_create(fiber_one_pwriter, ic, 320000);
	}

	if (off < fc->len) {
		struct IO_CTX *ic = malloc(sizeof(struct IO_CTX));
		ic->fd  = fd;
		ic->sem = sem;
		ic->len = fc->len - off;
		ic->off = off;
		cnt++;

		acl_fiber_create(fiber_one_pwriter, ic, 320000);
	}

	for (i = 0; i < cnt; i++) {
		acl_fiber_sem_wait(sem);
	}

	printf("All pwrite fibers finished!\r\n");
	acl_fiber_sem_free(sem);
}

static void fiber_co_pwrite(ACL_FIBER *fb acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	int fd, i;

	fd = open(fc->frompath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd == -1) {
		printf("open %s for write error %s\r\n",
			fc->frompath, strerror(errno));
		exit(1);
	}

	if (fc->check) {
		acl_fiber_create(fiber_sleep, NULL, 32000);
	}

	for (i = 0; i < 10; i++) {
		co_writers(fc, fd);
	}
	close(fd);
}

static void fiber_reader(ACL_FIBER *fb acl_unused, void *ctx)
{
	struct FIBER_CTX *fc = (struct FIBER_CTX*) ctx;
	ACL_VSTREAM *conn = acl_vstream_connect(fc->addr, ACL_BLOCKING, 10, 10, 1024);
	char buf[1024];

	while (1) {
		int ret = read(ACL_VSTREAM_SOCK(conn), buf, sizeof(buf) - 1);
		if (ret <= 0) {
			break;
		}

		printf(">>read count=%d\r\n", ret);

		buf[ret] = 0;
		printf("%s", buf);
		fflush(stdout);
	}

	acl_vstream_close(conn);
}

static void wait_and_sendfile(int in, struct FIBER_CTX *fc)
{
	ACL_VSTREAM *ln = acl_vstream_listen(fc->addr, 128), *conn;
	ssize_t ret;
	off_t off_saved = fc->off;
	int cfd;
	if (ln == NULL) {
		printf("listen %s error %s\r\n", fc->addr, strerror(errno));
		exit(1);
	}

	acl_fiber_create(fiber_reader, fc, 320000);

	while (1) {
		printf("Waiting for accept from %s ...\r\n", fc->addr);

		conn = acl_vstream_accept(ln, NULL, 0);
		if (conn == NULL) {
			printf("accept from %s error %s\r\n",
				fc->addr, strerror(errno));
			exit(1);
		}

		cfd = ACL_VSTREAM_SOCK(conn);
		acl_vstream_free(conn);

		printf(">>>begin sendfile64 to fd=%d, off=%d, len=%d\r\n",
			cfd, (int) fc->off, (int) fc->len);

#ifdef	__linux__
		ret = sendfile64(cfd, in, &fc->off, fc->len);
#elif	defined(__APPLE__)
#error	"not support now!"
#endif
		printf(">>>begin to close cfd=%d\r\n", cfd);
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

	printf("open %s ok\r\n", fc->frompath);

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
		"  -a action[read|write|rename|unlink|stat|mkdir|splice|pread|pwrite|sendfile|co_pwrite|co_pread]\r\n"
		"  -n size[default: 1024]\r\n"
		"  -o open_flags[O_RDONLY, O_WRONLY, O_RDWR, O_APPEND, O_CREAT, O_EXCL, O_TRUNC]\r\n"
		"  -p offset\r\n"
		"  -C [if check the process been blocked by disk IO]\r\n"
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
	snprintf(ctx.addr, sizeof(ctx.addr), "127.0.0.1|8080");
	ctx.check    = 0;

#define	EQ(x, y)	!strcasecmp((x), (y))

	while ((ch = getopt(argc, argv, "hf:t:a:o:n:p:C")) > 0) {
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
			ctx.len = atoll(optarg);
			break;
		case 'C':
			ctx.check = 1;
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

	acl_fiber_msg_stdout_enable(1);
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
	} else if (EQ(action, "pwrite")) {
		acl_fiber_create(fiber_pwrite, &ctx, 320000);
	} else if (EQ(action, "co_pwrite")) {
		acl_fiber_create(fiber_co_pwrite, &ctx, 320000);
	} else if (EQ(action, "co_pread")) {
		acl_fiber_create(fiber_co_pread, &ctx, 320000);
	} else if (EQ(action, "sendfile")) {
		acl_fiber_create(fiber_sendfile, &ctx, 320000);
	} else {
		printf("Unknown action=%s\r\n", action);
		usage(argv[0]);
	}

	acl_fiber_schedule_with(FIBER_EVENT_IO_URING);

	return 0;
}

