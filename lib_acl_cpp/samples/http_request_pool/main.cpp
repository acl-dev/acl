#include "stdafx.h"

using namespace acl;

static int __loop_count = 10;
static connect_pool* __conn_pool = NULL;
static acl_pthread_pool_t* __thr_pool = NULL;
static bool __unzip = false;
static bool __debug = false;
static acl::string __host;

// 初始化过程
static void init(const char* addr, int count)
{
	// 创建 HTTP 请求连接池对象
	__conn_pool = new http_request_pool(addr, count);

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

// HTTP 请求过程，向服务器发送请求后从服务器读取响应数据
static bool http_get(http_request* conn, int n)
{
	// 创建 HTTP 请求头数据
	http_header& header = conn->request_header();
	header.set_url("/")
		.set_keep_alive(true)
		.set_method(HTTP_METHOD_GET)
		.accept_gzip(__unzip);
	if (!__host.empty()) {
		header.set_host(__host);
	}

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
		ret = conn->read_body(buf, sizeof(buf));
		if (ret == 0) {
			break;
		} else if (ret < 0) {
			printf("%lu--%d: error, length: %d\r\n",
				(unsigned long) acl_pthread_self(), n, length);
			return false;
		}
		length += ret;
		if (__debug) {
			printf("%lu--%d: read length: %d, %d\r\n",
				(unsigned long) acl_pthread_self(), n, length, ret);
		}
	}
	if (__debug) {
		printf("%lu--%d: read body over, length: %d\r\n",
			(unsigned long) acl_pthread_self(), n, length);
	}
	return true;
}

// 线程处理过程
static void thread_main(void*)
{
	for (int i = 0; i < __loop_count; i++) {
		// 从连接池中获取一个 HTTP 连接
		http_request* conn = (http_request*) __conn_pool->peek();
		if (conn == NULL) {
			printf("peek connect failed\r\n");
			break;
		}

		// 需要对获得的连接重置状态，以清除上次请求过程的临时数据
		else {
			conn->reset();
		}
		// 开始新的 HTTP 请求过程
		if (http_get(conn, i) == false) {
			printf("one request failed, close connection\r\n");
			// 错误连接需要关闭
			__conn_pool->put(conn, false);
		} else {
			__conn_pool->put(conn, true);
		}
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
		"	-s http_server_addr [www.sina.com.cn:80]\r\n"
		"	-H host\r\n"
		"	-z [unzip response body, default: false]\r\n"
		"	-c cocurrent [default: 10]\r\n"
		"	-D [if in debug mode]\r\n"
		"	-n loop_count[default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 10;
	string addr("www.sina.com.cn:80");

	// 初始化 acl 库
	acl::acl_cpp_init();

	while ((ch = getopt(argc, argv, "hs:H:n:c:zD")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'H':
			__host = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			__loop_count = atoi(optarg);
			break;
		case 'z':
			__unzip = true;
			break;
		case 'D':
			__debug = true;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	if (__host.empty()) {
		__host = addr;
	}

	init(addr, cocurrent);
	run(cocurrent);
	end();

#ifdef WIN32
	printf("enter any key to exit...\r\n");
	getchar();
#endif

	return 0;
}
