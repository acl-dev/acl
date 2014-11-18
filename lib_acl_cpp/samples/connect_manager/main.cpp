#include "stdafx.h"
#include "connect_manager.h"
#include "connect_pool.h"
#include "connect_client.h"

static int __loop_count = 10;
static acl::connect_manager* __conn_manager = NULL;
static acl_pthread_pool_t* __thr_pool = NULL;

static void sleep_while(int n)
{
	for (int i = 0; i < n; i++)
	{
		putchar('.');
		fflush(stdout);
		sleep(1);
	}
	printf("\r\n");
}

static void check_all_connections(void)
{
	std::vector<acl::connect_pool*>& pools = __conn_manager->get_pools();
	std::vector<acl::connect_pool*>::const_iterator cit = pools.begin();
	for (; cit != pools.end(); ++cit)
		printf(">>>addr: %s %s\r\n", (*cit)->get_addr(),
			(*cit)->aliving() ? "alive" : "dead");
}

// 初始化过程
static void init(const char* addrs, int count)
{
	// 创建 HTTP 请求连接池集群管理对象
	__conn_manager = new connect_manager();

	// 添加服务器集群地址
	__conn_manager->init(addrs, addrs, 100);

	printf(">>>start monitor thread\r\n");

	// 启动后台检测线程
	int  check_inter = 1, conn_timeout = 5;
	__conn_manager->start_monitor(check_inter, conn_timeout);


	int   n = 10;
	printf(">>>sleep %d seconds for monitor check\r\n", n);
	sleep_while(n);

	printf(">>>create thread pool\r\n");
	// 创建线程池
	__thr_pool = acl_thread_pool_create(count, 60);
}

// 进程退出前清理资源
static void end(void)
{
	// 销毁线程池
	acl_pthread_pool_destroy(__thr_pool);

	// 打印所有连接池集群的存活状态
	printf("\r\n");

	check_all_connections();

	printf("\r\n>>> STOPPING check thread now\r\n");

	// 停止后台检测线程
	__conn_manager->stop_monitor(true);

	// 销毁连接池
	delete __conn_manager;
}

// 请求过程，向服务器发送请求后从服务器读取响应数据
static bool get(connect_client* conn, int n)
{
	printf(">>>check addr: %s, n: %d\r\n", conn->get_addr(), n);

	return true;
}

// 子线程处理过程
static void thread_main(void*)
{
	for (int i = 0; i < __loop_count; i++)
	{
		connect_pool* pool = (connect_pool*) __conn_manager->peek();
		if (pool == NULL)
		{
			printf("\r\n>>>%lu(%d): peek pool failed<<<\r\n",
				(unsigned long) acl_pthread_self(), __LINE__);
			check_all_connections();
			exit (1);
		}

		// 设置连接的超时时间及读超时时间
		pool->set_timeout(2, 2);

		// 从连接池中获取一个连接
		connect_client* conn = (connect_client*) pool->peek();
		if (conn == NULL)
		{
			printf("\r\n>>>%lu: peek connect failed from %s<<<\r\n",
				(unsigned long) acl_pthread_self(),
				pool->get_addr());
			check_all_connections();
			exit (1);
		}

		// 需要对获得的连接重置状态，以清除上次请求过程的临时数据
		else
			conn->reset();

		// 开始新的 HTTP 请求过程
		if (get(conn, i) == false)
		{
			printf("one request failed, close connection\r\n");
			// 错误连接需要关闭
			pool->put(conn, false);
		}
		else
			pool->put(conn, true);
	}

	printf(">>>>thread: %lu OVER<<<<\r\n", (unsigned long) acl_pthread_self());
}

static void run(int cocurrent)
{
	// 向线程池中添加任务
	for (int i = 0; i < cocurrent; i++)
		acl_pthread_pool_add(__thr_pool, thread_main, NULL);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addrs [www.sina.com.cn:80;www.263.net:80;www.qq.com:80]\r\n"
		"	-c cocurrent [default: 10]\r\n"
		"	-n loop_count[default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 10;
	acl::string addrs("www.sina.com.cn:80;www.263.net:80;www.qq.com:81");

	// 初始化 acl 库
	acl::acl_cpp_init();

	// 日志输出至标准输出
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:n:c:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addrs = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			__loop_count = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	init(addrs, cocurrent);
	run(cocurrent);
	end();

#ifdef WIN32
	printf("enter any key to exit...\r\n");
	getchar();
#endif

	return 0;
}
