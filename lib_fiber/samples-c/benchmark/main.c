#define	_GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/time.h>
#include <liburing.h>

static double stamp_sub(const struct timeval *from, const struct timeval *sub)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}

	res.tv_sec -= sub->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec / 1000.0);
}

//////////////////////////////////////////////////////////////////////////////

static int __open_flags = O_WRONLY | O_APPEND | O_CREAT;
static long long __write_count = 100000;

//////////////////////////////////////////////////////////////////////////////

static void sys_write(const char *path)
{
	int   fd = open(path, __open_flags, 0600), ret;
	char  buf[1] = { '0' };
	long long i;

	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		exit(1);
	}

	printf("open %s ok, fd=%d\r\n", path, fd);

	for (i = 0; i < __write_count; i++) {
		if (write(fd, buf, 1) <= 0) {
			printf("write to %s error %s\r\n", path, strerror(errno));
			exit(1);
		}
	}

	ret = write(fd, "\r\n", 2);
	if (ret <= 0) {
		printf("write CRLF error %s\r\n", strerror(errno));
	}

	ret = close(fd);
	printf("close %s %s, fd=%d\r\n", path, ret == 0 ? "ok" : "error", fd);
}

//////////////////////////////////////////////////////////////////////////////

static void uring_write(const char *path)
{
	int   fd = open(path, __open_flags, 0600), ret;
	long long i;
	char  buf[1] = { '0' };
	struct io_uring uring;

	if (fd < 0) {
		printf("open %s error %s\r\n", path, strerror(errno));
		exit(1);
	}

	struct io_uring_params params;
	memset(&params, 0, sizeof(params));

	ret = io_uring_queue_init_params(100, &uring, &params);
	if (ret < 0) {
		printf("init io_uring error=%s\r\n", strerror(errno));
		exit(1);
	}

	struct io_uring_sqe *sqe;
	struct io_uring_cqe *cqe;

	printf("open %s ok, fd=%d\r\n", path, fd);

	for (i = 0; i < __write_count; i++) {
		sqe = io_uring_get_sqe(&uring);
		io_uring_prep_write(sqe, fd, buf, 1, i);
		io_uring_submit(&uring);
		ret = io_uring_wait_cqes(&uring, &cqe, 1, NULL, NULL);
		if (ret < 0) {
			printf("io_uring_wait_cqe error=%s\r\n", strerror(-ret));
			exit(1);
		}

		io_uring_cqe_seen(&uring, cqe);
	}

	ret = write(fd, "\r\n", 2);
	if (ret <= 0) {
		printf("write CRLF error %s\r\n", strerror(errno));
		exit(1);
	}

	ret = close(fd);
	printf("close %s %s, fd=%d\r\n", path, ret == 0 ? "ok" : "error", fd);
	io_uring_queue_exit(&uring);
}

//////////////////////////////////////////////////////////////////////////////

static void test_write(const char *filepath)
{
	struct timeval begin, end;
	double cost, speed;

	//////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);

	uring_write(filepath);

	gettimeofday(&end, NULL);

	cost = stamp_sub(&end, &begin);
	speed = (__write_count * 1000) / (cost > 0 ? cost : 0.001);

	printf("uring write, total write=%lld, cost=%.2f ms, speed=%.2f\r\n",
		__write_count, cost, speed);

	//////////////////////////////////////////////////////////////////////

	printf("-------------------------------------------------------\r\n");

	gettimeofday(&begin, NULL);

	sys_write(filepath);

	gettimeofday(&end, NULL);

	cost = stamp_sub(&end, &begin);
	speed = (__write_count * 1000) / (cost > 0 ? cost : 0.001);

	printf("sys write, total write=%lld, cost=%.2f ms, speed=%.2f\r\n",
		__write_count, cost, speed);
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char *proc)
{
	printf("usage: %s -h [help]\r\n"
		"  -f filepath\r\n"
		"  -n size[default: 1024]\r\n"
		, proc);
}

int main(int argc, char *argv[])
{
	int  ch;
	char path[256];

	snprintf(path, sizeof(path), "file.txt");

	while ((ch = getopt(argc, argv, "hf:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			snprintf(path, sizeof(path), "%s", optarg);
			break;
		case 'n':
			__write_count = atoi(optarg);
			break;
		default:
			break;
		}
	}

	test_write(path);
	return 0;
}
