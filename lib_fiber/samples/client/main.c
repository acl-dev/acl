#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"

static int __conn_timeout = 0;
static int __rw_timeout   = 0;
static int __max_loop     = 10000;
static int __max_fibers   = 100;
static int __left_fibers  = 100;
static struct timeval __begin, __end;

static void echo_client(ACL_VSTREAM *cstream)
{
	char  buf[8192];
	int   ret, i;
	const char *str = "hello world\r\n";

	for (i = 0; i < __max_loop; i++) {
		if (acl_vstream_writen(cstream, str, strlen(str))
			== ACL_VSTREAM_EOF)
		{
			printf("write error: %s\r\n", acl_last_serror());
			break;
		}

		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s, i: %d\r\n", acl_last_serror(), i);
			break;
		}
		buf[ret] = 0;
		//printf("gets line: %s", buf);
	}

	acl_vstream_close(cstream);
}

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static void fiber_connect(FIBER *fiber acl_unused, void *ctx)
{
	const char *addr = (const char *) ctx;
	ACL_VSTREAM *cstream = acl_vstream_connect(addr, ACL_BLOCKING,
			__conn_timeout, __rw_timeout, 4096);
	if (cstream == NULL)
		printf("connect %s error %s\r\n", addr, acl_last_serror());
	else
		echo_client(cstream);

	--__left_fibers;
	printf("max_fibers: %d, left: %d\r\n", __max_fibers, __left_fibers);

	if (__left_fibers == 0) {
		double spent;
		long long count = __max_loop * __max_fibers;

		gettimeofday(&__end, NULL);
		spent = stamp_sub(&__end, &__begin);
		printf("fibers: %d, count: %lld, spent: %.2f, speed: %.2f\r\n",
			__max_fibers, count, spent,
			(count * 1000) / (spent > 0 ? spent : 1));

		fiber_io_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s addr\r\n"
		" -t connt_timeout\r\n"
		" -r rw_timeout\r\n"
		" -c max_fibers\r\n"
		" -n max_loop\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, i;
	char  addr[256];
       
	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "0.0.0.0:9002");

	while ((ch = getopt(argc, argv, "hc:n:s:t:r:")) > 0) {
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
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		default:
			break;
		}
	}

	gettimeofday(&__begin, NULL);

	for (i = 0; i < __max_fibers; i++)
		fiber_create(fiber_connect, addr, 32768);

	printf("call fiber_schedule\r\n");

	fiber_schedule();

	return 0;
}
