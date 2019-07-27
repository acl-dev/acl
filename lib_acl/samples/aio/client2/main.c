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

// 褰撹繛鎺ュ彞鏌勫叧闂椂鐨勫洖璋冨嚱鏁�

static int on_close(ACL_ASTREAM *conn, void *ctx)
{
	const char *server_addr = (const char*) ctx;

	printf("fd=%d, disconnected from %s\r\n",
		ACL_VSTREAM_SOCK(conn->stream), server_addr);
	__nclosed++;
	return 0;
}

// 褰撳啓鎴愬姛鏃剁殑鍥炶皟鍑芥暟

static int on_write(ACL_ASTREAM *conn acl_unused, void *ctx acl_unused)
{
	return 0;
}

// 褰撹鍒版湇鍔＄鐨勬暟鎹椂鐨勫洖璋冨嚱鏁�

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

// 杩炴帴鎴愬姛鎴栧け璐ユ椂鐨勫洖璋冨嚱鏁�

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

	// 娣诲姞璇绘垚鍔熸椂鐨勫洖璋冨嚱鏁�
	acl_aio_add_read_hook(conn, on_read, ctx);

	// 娣诲姞鍐欐垚鍔熸椂鐨勫洖璋冨嚱鏁�
	acl_aio_add_write_hook(conn, on_write, ctx);

	// 娣诲姞杩炴帴鍙ユ焺鍏抽棴鏃剁殑鍥炶皟鍑芥暟
	acl_aio_add_close_hook(conn, on_close, ctx);

	// 娉ㄥ唽璇讳簨浠惰繃绋�
	acl_aio_read(conn);

	// 寰€鏈嶅姟鍣ㄥ啓鏁版嵁锛屼粠鑰屽紑濮嬭Е鍙戞湇鍔″櫒鐨勫洖鏄剧ず杩囩▼
	acl_aio_writen(conn, data, sizeof(data) - 1);

	// 杩斿洖 0 琛ㄧず缁х画
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

	// 鍒涘缓寮傛 IO 鍙ユ焺
	aio = acl_aio_create(ACL_EVENT_KERNEL);

	// 璁剧疆鎸佺画璇绘爣蹇椾綅
	acl_aio_set_keep_read(aio, 1);

	// 璁剧疆鍩熷悕鏈嶅姟鍣ㄥ湴鍧€
	acl_aio_set_dns(aio, dns_addr, 5);

	// 杩炴帴鍥炴樉鏈嶅姟鍣ㄥ湴鍧€锛屽苟娉ㄥ唽杩炴帴鍥炶皟鍑芥暟
	for (i = 0; i < __nconnect; i++) {
		acl_aio_connect_addr(aio, svr_addr, __timeout, on_connect, svr_addr);
	}

	gettimeofday(&begin, NULL);

	while (__nclosed < __nconnect) {
		acl_aio_loop(aio);
	}

	acl_aio_free(aio);

	gettimeofday(&end, NULL);

	// 璁＄畻 IO 澶勭悊閫熷害
	spent = stamp_sub(&end, &begin);
	speed = (__count * 1000) / (spent > 0 ? spent : 1);

	printf("\r\n");
	printf("total=%lld, cocurrent=%d, loop=%lld, spent %.2f ms, speed %.2f\r\n",
		__count, __nconnect, __nloop, spent, speed);

	return 0;
}
