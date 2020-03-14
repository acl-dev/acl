#include "stdafx.h"

using namespace acl;

static int __loop_count = 10;
static connect_manager* __conn_manager = NULL;
static acl_pthread_pool_t* __thr_pool = NULL;
static bool __unzip = false;
static bool __debug = false;

static void sleep_while(int n)
{
	for (int i = 0; i < n; i++) {
		putchar('.');
		fflush(stdout);
		sleep(1);
	}
	printf("\r\n");
}

// 初始化过程
static void init(const char* addrs, int count)
{
	int  cocurrent = 100, conn_timeout = 100, rw_timeout = 200;

	// 创建 HTTP 请求连接池集群管理对象
	__conn_manager = new http_request_manager();

	// 添加服务器集群地址
	__conn_manager->init(addrs, addrs, cocurrent, conn_timeout, rw_timeout);

	printf(">>>start monitor thread\r\n");

	int  check_inter = 1, check_conn_timeout = 5;
	acl::connect_monitor* monitor = new acl::connect_monitor(*__conn_manager);
	(*monitor).set_check_inter(check_inter).set_conn_timeout(check_conn_timeout);

	// 启动后台检测线程
	__conn_manager->start_monitor(monitor);


	int   n = 2;
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

#if 0
	int   n = 10;
	printf("\r\n>>>sleep %d seconds to stop monitor\r\n", n);
	sleep_while(n);
#endif

	// 打印所有连接池集群的存活状态
	printf("\r\n");
	std::vector<connect_pool*>& pools = __conn_manager->get_pools();
	std::vector<connect_pool*>::const_iterator cit = pools.begin();
	for (; cit != pools.end(); ++cit) {
		printf(">>>server: %s, %s\r\n",
			(*cit)->get_addr(), (*cit)->aliving()
			? "alive" : "dead");
	}
	printf("\r\n>>> STOPPING check thread now\r\n");

	// 停止后台检测线程
	acl::connect_monitor* monitor = __conn_manager->stop_monitor(true);
	// 删除检测器对象
	delete monitor;

	// 销毁连接池
	delete __conn_manager;
}

// HTTP 请求过程，向服务器发送请求后从服务器读取响应数据
static bool http_get(http_request* conn, const char* addr, int n)
{
	if (__debug) {
		printf(">>>check addr: %s, n: %d\r\n", addr, n);
	}

	// 创建 HTTP 请求头数据
	http_header& header = conn->request_header();
	header.set_url("/")
		.set_host(addr)
		.set_keep_alive(true)
		.set_method(HTTP_METHOD_GET)
		.accept_gzip(__unzip);

	if (__debug) {
		printf("%lu--%d: begin send request\r\n",
			(unsigned long) acl_pthread_self(), n);
	}
	// 发送 HTTP 请求数据同时接收 HTTP 响应头
	if (conn->request(NULL, 0) == false) {
		printf("%lu--%d: send GET request error\r\n",
			(unsigned long) acl_pthread_self(), n);
		return false;
	}

	char  buf[8192];
	int   ret, length = 0;

	// 接收 HTTP 响应体数据
	while (true) {
		ret = conn->read_body(buf, sizeof(buf) - 1);
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			printf("%lu--%d: error, length: %d\r\n",
				(unsigned long) acl_pthread_self(),
				n, length);
			return false;
		}
		//buf[ret] = 0;
		//printf("%s", buf);fflush(stdout);

		length += ret;
		if (__debug) {
			printf("%lu--%d: read length: %d, %d\r\n",
				(unsigned long) acl_pthread_self(),
				n, length, ret);
		}
	}
	if (__debug) {
		printf("%lu--%d: read body over, length: %d\r\n",
			(unsigned long) acl_pthread_self(), n, length);
	}
	return true;
}

static void check_all_connections(void)
{
	std::vector<connect_pool*>& pools = __conn_manager->get_pools();
	std::vector<connect_pool*>::const_iterator cit = pools.begin();
	for (; cit != pools.end(); ++cit) {
		printf(">>>addr: %s %s\r\n", (*cit)->get_addr(),
			(*cit)->aliving() ? "alive" : "dead");
	}
}

// 线程处理过程
static void thread_main(void*)
{
	for (int i = 0; i < __loop_count; i++) {
		http_request_pool* pool = (http_request_pool*)
			__conn_manager->peek();
		if (pool == NULL) {
			printf("\r\n>>>%lu(%d): peek pool failed<<<\r\n",
				(unsigned long) acl_pthread_self(), __LINE__);
			check_all_connections();
			exit (1);
		}

		// 从连接池中获取一个 HTTP 连接
		http_request* conn = (http_request*) pool->peek();
		if (conn == NULL) {
			printf("\r\n>>>%lu: peek connect failed from %s<<<\r\n",
				(unsigned long) acl_pthread_self(),
				pool->get_addr());
			check_all_connections();
			exit (1);
		}

		// 需要对获得的连接重置状态，以清除上次请求过程的临时数据
		else {
			conn->reset();
		}

		// 开始新的 HTTP 请求过程
		if (http_get(conn, pool->get_addr(), i) == false) {
			printf("one request failed, close connection\r\n");
			// 错误连接需要关闭
			pool->put(conn, false);
		} else {
			pool->put(conn, true);
		}
	}

	if (__debug) {
		printf(">>>>thread: %lu OVER<<<<\r\n",
			(unsigned long) acl_pthread_self());
	}
}

static void run(int cocurrent)
{
	// 向线程池中添加任务
	for (int i = 0; i < cocurrent; i++) {
		acl_pthread_pool_add(__thr_pool, thread_main, NULL);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s http_server_addrs [www.sina.com.cn:80;www.263.net:80;www.qq.com:80]\r\n"
		"	-z [unzip response body, default: false]\r\n"
		"	-c cocurrent [default: 10]\r\n"
		"	-D [if using debug mode]\r\n"
		"	-n loop_count[default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 10;
	string addrs("www.sina.com.cn:80;www.263.net:80;www.qq.com:81");

	// 初始化 acl 库
	acl::acl_cpp_init();

	// 日志输出至标准输出
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:Dn:c:z")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addrs = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'D':
			__debug = true;
			break;
		case 'n':
			__loop_count = atoi(optarg);
			break;
		case 'z':
			__unzip = true;
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
