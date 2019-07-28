#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

static double stamp_sub(const struct timeval* from, const struct timeval* sub)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}

	res.tv_sec -= sub->tv_sec;
	return res.tv_sec * 1000.0 + res.tv_usec / 1000.0;
}

static int  __nconnect   = 100;
static int  __nclosed    = 0;
static int  __timeout    = 120;
static long long __nloop = 1000;
static long long __count = 0;

// 当连接句柄关闭时的回调函数

static int on_close(ACL_ASTREAM *conn, void *ctx)
{
	const char *server_addr = (const char*) ctx;

	printf("fd=%d, disconnected from %s\r\n",
		ACL_VSTREAM_SOCK(conn->stream), server_addr);
	__nclosed++;
	return 0;
}

// 当写成功时的回调函数

static int on_write(ACL_ASTREAM *conn acl_unused, void *ctx acl_unused)
{
	return 0;
}

// 当读到服务端的数据时的回调函数

static int on_read(ACL_ASTREAM *conn, void *ctx acl_unused,
		char *data, int dlen)
{
	if (__count % 50000 == 0) {
		char buf[128];
		snprintf(buf, sizeof(buf), "fd=%d, count=%lld",
			ACL_VSTREAM_SOCK(conn->stream), __count);
		acl_meter_time(__FUNCTION__, __LINE__, buf);
	}

	if (++__count < __nloop) {
		acl_aio_writen(conn, data, dlen);
		return 0;
	}

	printf("begin close fd %d\r\n", ACL_VSTREAM_SOCK(conn->stream));
	return -1;
}

// 连接成功或失败时的回调函数

static int on_connect(ACL_ASTREAM *conn, void *ctx)
{
	const char *server_addr = (const char *) ctx;
	const char data[] = "hello world!\r\n";

	if (conn == NULL) {
		int err = acl_last_error();
		printf("connect %s failed, errno=%d, %s\r\n",
			server_addr, acl_last_error(),
			err < 0 ? acl_dns_serror(err) : acl_last_serror());
		__nclosed++;
		return -1;
	}

	printf("connect %s ok\r\n", server_addr);

	// 添加读成功时的回调函数
	acl_aio_add_read_hook(conn, on_read, ctx);

	// 添加写成功时的回调函数
	acl_aio_add_write_hook(conn, on_write, ctx);

	// 添加连接句柄关闭时的回调函数
	acl_aio_add_close_hook(conn, on_close, ctx);

	// 注册读事件过程
	acl_aio_read(conn);

	// 往服务器写数据，从而开始触发服务器的回显示过程
	acl_aio_writen(conn, data, sizeof(data) - 1);

	// 返回 0 表示继续
	return 0;
}

static void usage(const char *progname)
{
	printf("usage: %s -h (help)\r\n"
		"	-N dns_addr\r\n"
		"	-s server_addr(ip:port)\r\n"
		"	-c nconnect\r\n"
		"	-n nloop\r\n"
		"	-t timeout\r\n"
		, progname);
}

int main(int argc, char *argv[])
{
	ACL_AIO *aio;
	char svr_addr[128], dns_addr[128];
	int  ch, i;
	struct timeval begin, end;
	double spent, speed;

	svr_addr[0] = 0;
	dns_addr[0] = 0;

	while ((ch = getopt(argc, argv, "hN:s:c:n:t:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			__nconnect = atoi(optarg);
			break;
		case 'n':
			__nloop = atoll(optarg);
			break;
		case 'N':
			snprintf(dns_addr, sizeof(dns_addr), "%s", optarg);
			break;
		case 's':
			snprintf(svr_addr, sizeof(svr_addr), "%s", optarg);
			break;
		case 't':
			__timeout = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (svr_addr[0] == 0 || dns_addr[0] == 0) {
		usage(argv[0]);
		return 0;
	}

	// 创建异步 IO 句柄
	aio = acl_aio_create(ACL_EVENT_KERNEL);

	// 设置持续读标志位
	acl_aio_set_keep_read(aio, 1);

	// 设置域名服务器地址
	acl_aio_set_dns(aio, dns_addr, 5);

	// 连接回显服务器地址，并注册连接回调函数
	for (i = 0; i < __nconnect; i++) {
		acl_aio_connect_addr(aio, svr_addr, __timeout, on_connect, svr_addr);
	}

	gettimeofday(&begin, NULL);

	while (__nclosed < __nconnect) {
		acl_aio_loop(aio);
	}

	acl_aio_free(aio);

	gettimeofday(&end, NULL);

	// 计算 IO 处理速度
	spent = stamp_sub(&end, &begin);
	speed = (__count * 1000) / (spent > 0 ? spent : 1);

	printf("\r\n");
	printf("total=%lld, cocurrent=%d, loop=%lld, spent %.2f ms, speed %.2f\r\n",
		__count, __nconnect, __nloop, spent, speed);

	return 0;
}
