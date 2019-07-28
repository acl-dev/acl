#include "stdafx.h"
#include "util.h"

using namespace acl;

static int __loop_count = 10;
static connect_pool* __conn_pool = NULL;
static string __action("get");
static char __data[256];
static acl_pthread_pool_t* __thr_pool = NULL;

// 初始化过程
static void init(const char* addr, int count)
{
	// 创建 HTTP 请求连接池对象
	__conn_pool = new memcache_pool(addr, count);

	// 创建线程池
	__thr_pool = acl_thread_pool_create(count, 60);
}

// 进程退出前清理资源
static void end(void)
{
	// 销毁线程池
	acl_pthread_pool_destroy(__thr_pool);

	// 销毁连接池
	delete __conn_pool;
}

static const char* __keypre = "test";

// MEMCACHE 请求过程，向服务器发送请求后从服务器读取响应数据
static bool memcache_get(memcache* conn, const char* key)
{
	string buf;

	conn->set_prefix(__keypre);
	if (conn->get(key, buf) == false)
	{
		printf("get key: %s error\r\n", key);
		return false;
	}
	else
		return true;
}

// 向 MEMCACHE 服务器添加数据
static bool memcache_set(memcache* conn, const char* key)
{
	string buf;

	conn->set_prefix(__keypre);
	if (conn->set(key, __data, sizeof(__data) - 1) == false)
	{
		printf("set key: %s error\r\n", key);
		return false;
	}
	else
		return true;
}

// 线程处理过程
static void thread_main(void* ctx)
{
	char* key = (char*) ctx;
	bool (*action_fn)(memcache* conn, const char* key);
	string keybuf;
	char   buf[256];

	if (__action == "set")
	{
		action_fn = memcache_set;
		memset(__data, 'X', sizeof(__data));
		__data[sizeof(__data) - 1] = 0;
	}
	else
		action_fn = memcache_get;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int   i = 0;
	for (; i < __loop_count; i++)
	{
		// 从连接池中获取一个 HTTP 连接
		memcache* conn = (memcache*) __conn_pool->peek();
		if (conn == NULL)
		{
			printf("peek connect failed\r\n");
			break;
		}

		keybuf.format("%s_%d", key, i);

		// 开始新的 HTTP 请求过程
		if (action_fn(conn, keybuf.c_str()) == false)
		{
			printf("one request failed, close connection\r\n");
			// 错误连接需要关闭
			__conn_pool->put(conn, false);
			break;
		}
		else
			__conn_pool->put(conn, true);
		if (i % 1000 == 0)
		{
			acl::safe_snprintf(buf, sizeof(buf), "key: %s, action(%s) ok",
				keybuf.c_str(), __action.c_str());
			ACL_METER_TIME(buf);
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double spent = util::stamp_sub(&end, &begin);
	printf("total: %d, curr: %d, spent: %.2f, speed: %.2f\r\n",
		__loop_count, i, spent, (i * 1000) / (spent > 1 ? spent : 1));
}

static void run(int cocurrent, char* key)
{
	// 向线程池中添加任务
	for (int i = 0; i < cocurrent; i++)
		acl_pthread_pool_add(__thr_pool, thread_main, key);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s memcache_server_addr [127.0.0.1:11211]\r\n"
		"	-k key\r\n"
		"	-c cocurrent [default: 10]\r\n"
		"	-a action [get|set, default: get]\r\n"
		"	-n loop_count[default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 10;
	string addr("127.0.0.1:11211");
	string  key;

	// 初始化 acl 库
	acl::acl_cpp_init();

	while ((ch = getopt(argc, argv, "hs:n:c:k:a:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			__loop_count = atoi(optarg);
			break;
		case 'k':
			key = optarg;
			break;
		case 'a':
			__action = optarg;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (key.empty())
	{
		usage(argv[0]);
		return 0;
	}

	init(addr, cocurrent);
	run(cocurrent, key.c_str());
	end();

#ifdef WIN32
	printf("enter any key to exit...\r\n");
	getchar();
#endif

	return 0;
}
