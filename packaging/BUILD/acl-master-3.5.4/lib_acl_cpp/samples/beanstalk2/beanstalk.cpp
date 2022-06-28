// beanstalk.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "util.h"

static char  __addr[64];
static const char* __tube = "zsxxsz";
static int __max = 100;

// 接收队列消息的消费者线程
static void consumer(void)
{
	acl::beanstalk conn(__addr, 10);
	// 从指定消息队列中接收消息
	if (conn.watch(__tube) == false)
	{
		printf("watch %s faile\r\n", __tube);
		return;
	}

	acl::string buf;
	unsigned long long id;

	// 超时接收一条消息
	if ((id = conn.reserve(buf)) == 0)
		printf("reserve failed, error: %s\r\n", conn.get_error());
	else
	{
		printf("id: %llu, buf: %s\r\n", id, buf.c_str());
		conn.delete_id(id);
	}

	// 阻塞接收一条消息
	if ((id = conn.reserve(buf, 1)) == 0)
		printf("reserve failed, error: %s\r\n", conn.get_error());
	else
		printf("id: %llu, buf: %s\r\n", id, buf.c_str());

}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s beanstalk_addr [127.0.0.1:11300] -n max_count\r\n", procname);
}

int main(int argc, char* argv[])
{
#if WIN32
	acl::acl_cpp_init();
#endif
	acl::log::stdout_open(true);

	snprintf(__addr, sizeof(__addr), "127.0.0.1:11300");
	int   ch;
	while ((ch = getopt(argc, argv, "hs:n:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(__addr, sizeof(__addr), "%s", optarg);
			break;
		case 'n':
			__max = atoi(optarg);
			if (__max <= 0)
				__max = 1;
			break;
		default:
			break;
		}
	}

	consumer();

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}

