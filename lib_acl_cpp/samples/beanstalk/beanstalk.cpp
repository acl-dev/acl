// beanstalk.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "util.h"

static char  __addr[64];
static const char* __tube = "zsxxsz";
static int __max = 100;

// 消息生产者线程
static void* producer(void* ctx)
{
	(void) ctx;

	// beanstalk 客户端连接对象
	acl::beanstalk conn(__addr, 10);

	// 指定消息目标队列
	if (conn.use(__tube) == false)
	{
		printf("use %s error\r\n", __tube);
		return NULL;
	}
	else
		printf("use %s ok\r\n", __tube);

	acl::string data;
	unsigned long long id;
	for (int i = 0; i < __max; i++)
	{
		data.format("hello-%d", i);
		// 向 beanstalkd 消息服务器发送消息
		if ((id = conn.put(data.c_str(), data.length())) == 0)
		{
			printf("put %s failed\r\n", data.c_str());
			return NULL;
		}
		else if (i < 10)
			printf("put %s ok, id: %llu, len: %d\r\n",
				data.c_str(), id, (int) data.length());
	}

	return NULL;
}

// 接收队列消息的消费者线程
static void* consumer(void* ctx)
{
	(void) ctx;

	acl::beanstalk conn(__addr, 10);
	// 从指定消息队列中接收消息
	if (conn.watch(__tube) == false)
	{
		printf("watch %s faile\r\n", __tube);
		return NULL;
	}

	acl::string buf;
	unsigned long long id;
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	ACL_METER_TIME("begin");
	for (int i = 0; i < __max; i++)
	{
		// 接收一条消息
		if ((id = conn.reserve(buf)) == 0)
		{
			printf("reserve failed\r\n");
			return NULL;
		}
		else if (i < 10)
			printf("reserved: %s\r\n", buf.c_str());

		// 删除接收到的消息
		if (conn.delete_id(id) == false)
		{
			printf("delete id %llu failed\r\n", id);
			return NULL;
		}
		else if (i < 10)
			printf("delete id %llu ok\r\n", id);
		if (i % 1000 == 0)
		{
			buf.format_append("; total: %d, curr: %d, id: %lld",
				__max, i, id);
			ACL_METER_TIME(buf.c_str());
		}
	}

	gettimeofday(&end, NULL);
	double n = util::stamp_sub(&end, &begin);
        printf("total get: %d, spent: %0.2f ms, speed: %0.2f\r\n",
		__max, n, (__max * 1000) /(n > 0 ? n : 1));
	return NULL;
}

static void test1()
{
	acl_pthread_attr_t attr;
	acl_pthread_t tid1, tid2;

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 0);

	acl_pthread_create(&tid1, &attr, consumer, NULL);
	acl_pthread_create(&tid2, &attr, producer, NULL);

	acl_pthread_join(tid1, NULL);
	acl_pthread_join(tid2, NULL);
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

	test1();

#ifdef WIN32
	printf("enter any key to exit\r\n");
	getchar();
#endif
	return 0;
}

