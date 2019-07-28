#include "stdafx.h"
#include "util.h"

static int   __max_count = 10;
static int   __dat_length = 100;
static char  __local_addr[64];
static char  __server_addr[64];
static bool  __server_fixed = true;

static void run(void)
{
	acl::socket_stream stream;
	char* buf;

	// 绑定本地地址
	if (stream.bind_udp(__local_addr) == false)
	{
		printf("bind addr %s error %s\r\n",
			__server_addr, acl::last_serror());
		return;
	}

	// 设置远程服务地址
	else
		stream.set_peer(__server_addr);

	stream.set_rw_timeout(1);

	// 分配内存
	buf = (char*) malloc(__dat_length + 1);
	memset(buf, 'X', __dat_length);
	buf[__dat_length] = 0;

	char res[4096];

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int   i = 0, ret;
	for (; i < __max_count; i++)
	{
		// 服务端可能会用别的地址回复数据，所以需要重新设置远程服务地址
		if (!__server_fixed)
			stream.set_peer(__server_addr);

		// 发送数据
		if (stream.write(buf, __dat_length) == -1)
		{
			printf("write error %s\r\n", acl::last_serror());
			break;
		}

		// 接收数据
		else if ((ret = stream.read(res, sizeof(res) - 1, false)) == -1)
		{
			printf("read error %s\r\n", acl::last_serror());
			break;
		}

		if (i % 1000 == 0)
		{
			res[ret] = 0;
			printf("read >>> %s\r\n", res);
		}

		if (i % 1000 == 0)
		{
			snprintf(res, sizeof(res), "total: %d, curr: %d",
				__max_count, i);
			ACL_METER_TIME(res);
		}
	}

	free(buf);

	struct timeval end;
	gettimeofday(&end, NULL);

	// 计算速度
	double spent = util::stamp_sub(&end, &begin);
	printf("total: %d, curr: %d, spent: %.2f, speed: %.2f\r\n",
		__max_count, i, spent, (i * 1000) / (spent > 1 ? spent : 1));
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addr [default: 127.0.0.1:8888]\r\n"
		"	-l local_addr [default: 127.0.0.1:18888]\r\n"
		"	-o [server reply in other addr, default: no]\r\n"
		"	-n max_count [default: 10]\r\n"
		"	-N data_length [default: 100]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int   ch;

#ifdef	WIN32
	acl::acl_cpp_init();
#endif
	acl::log::stdout_open(true);

	snprintf(__server_addr, sizeof(__server_addr), "127.0.0.1:8888");
	snprintf(__local_addr, sizeof(__local_addr), "127.0.0.1:18888");

	while ((ch = getopt(argc, argv, "hs:l:on:N:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__server_addr, sizeof(__server_addr), "%s", optarg);
			break;
		case 'l':
			snprintf(__local_addr, sizeof(__local_addr), "%s", optarg);
			break;
		case 'o':
			__server_fixed = false;
			break;
		case 'n':
			__max_count = atoi(optarg);
			break;
		case 'N':
			__dat_length = atoi(optarg);
			break;
		default:
			break;
		}
	}

	run();
	return 0;
}
