// http_server.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include "rpc_manager.h"
#include "rpc_stats.h"
#include "http_rpc.h"

static int var_data_size = 1024;

//////////////////////////////////////////////////////////////////////////

/**
 * 异步客户端流的回调类的子类
 */
class handle_io : public aio_callback
{
public:
	handle_io(aio_socket_stream* client)
		: client_(client)
	{
		http_ = new http_rpc(client_, (unsigned) var_data_size);
	}

	~handle_io()
	{
		delete http_;
		printf("delete io_callback now ...\r\n");
	}

	bool write_callback()
	{
		return (true);
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 */
	void close_callback()
	{
		printf("Closed now.\r\n");

		// 必须在此处删除该动态分配的回调类对象以防止内存泄露
		delete this;
	}

	/**
	 * 实现父类中的虚函数，客户端流的超时回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool timeout_callback()
	{
		printf("Timeout ...\r\n");
		return (false);
	}

	virtual bool read_wakeup()
	{
		// 测试状态
		rpc_read_wait_del();
		rpc_add();

		// 从异步监听集合中去掉对该异步流的监控
		client_->disable_read();

		// 发起一个 http 会话过程
		rpc_manager::get_instance().fork(http_);

		return true;
	}

private:
	aio_socket_stream* client_;
	http_rpc* http_;
};

//////////////////////////////////////////////////////////////////////////

/**
 * 异步监听流的回调类的子类
 */
class handle_accept : public aio_accept_callback
{
public:
	handle_accept(bool preread)
 	: preread_(preread)
	{
	}

	~handle_accept()
	{
		printf(">>io_accept_callback over!\n");
	}

	/**
	 * 基类虚函数，当有新连接到达后调用此回调过程
	 * @param client {aio_socket_stream*} 异步客户端流
	 * @return {bool} 返回 true 以通知监听流继续监听
	 */
	bool accept_callback(acl::aio_socket_stream* client)
	{
		// 如果允许在主线程中预读，则设置流的预读标志位
		if (preread_)
		{
			ACL_VSTREAM* vstream = client->get_vstream();
			vstream->flag |= ACL_VSTREAM_FLAG_PREREAD;
		}

		// 创建异步客户端流的回调对象并与该异步流进行绑定
		handle_io* callback = new handle_io(client);

		// 注册异步流的读回调过程
		client->add_read_callback(callback);

		// 注册异步流的写回调过程
		client->add_write_callback(callback);

		// 注册异步流的关闭回调过程
		client->add_close_callback(callback);

		// 注册异步流的超时回调过程
		client->add_timeout_callback(callback);

		rpc_read_wait_add();

		// 监控异步流是否可读
		client->read_wait(10);

		return (true);
	}

private:
	bool preread_;
};

static void usage(const char* procname)
{
	printf("usage: %s \r\n"
		" -h[help]\r\n"
		" -p[preread in main thread]\r\n"
		" -l listen_addr[127.0.0.1:9001]\r\n"
		" -m[use mempool]\r\n"
		" -k[use kernel engine]\r\n"
		" -n data size response\r\n"
		" -N thread pool limit\r\n"
		" -r rpc_addr\r\n"
		" -v[enable stdout]\r\n", procname);
}

int main(int argc, char* argv[])
{
#ifdef WIN32
	acl_cpp_init();
#endif

	bool preread = false;
	char addr[32], rpc_addr[32], ch;
	bool use_mempool = false;
	bool use_kernel = false;
	bool enable_stdout = false;
	int  nthreads = 20;

	acl::safe_snprintf(addr, sizeof(addr), "127.0.0.1:9001");
	acl::safe_snprintf(rpc_addr, sizeof(rpc_addr), "127.0.0.1:0");

	while ((ch = getopt(argc, argv, "vkhpms:n:N:r:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			acl::safe_snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'p':
			preread = true;
			break;
		case 'm':
			use_mempool = true;
			break;
		case 'k':
			use_kernel = true;
			break;
		case 'n':
			var_data_size = atoi(optarg);
			if (var_data_size <= 0)
				var_data_size = 1024;
			break;
		case 'N':
			nthreads = atoi(optarg);
			if (nthreads <= 0)
				nthreads = 10;
			break;
		case 'v':
			enable_stdout = true;
			break;
		case 'r':
			acl::safe_snprintf(rpc_addr, sizeof(rpc_addr), "%s", optarg);
			break;
		default:
			break;
		}
	}

	// 是否采用线程局部内存池
	if (use_mempool)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

	rpc_stats_init();

	// 允许日志信息输出至屏幕
	if (enable_stdout)
		log::stdout_open(true);

	// 异步通信框架句柄，采用 select 系统 api
	aio_handle* handle = new aio_handle(use_kernel ? ENGINE_KERNEL : ENGINE_SELECT);

	// 创建监听异步流
	aio_listen_stream* sstream = new aio_listen_stream(handle);

	// 监听指定的地址
	if (sstream->open(addr) == false)
	{
		printf("open %s error!\r\n", addr);
		sstream->close();
		// XXX: 为了保证能关闭监听流，应在此处再 check 一下
		handle->check();
#ifdef WIN32
		getchar();
#endif
		return 1;
	}

	// 初始化异步 RPC 通信服务句柄
	rpc_manager::get_instance().init(handle, nthreads, rpc_addr);

	// 创建回调类对象，当有新连接到达时自动调用此类对象的回调过程
	handle_accept callback(preread);
	// 将回调处理类对象与异步监听流绑定
	sstream->add_accept_callback(&callback);

	printf("Listen: %s ok!\r\n", addr);

	time_t last = time(NULL), now;
	while (true)
	{
		// 如果返回 false 则表示不再继续，需要退出
		if (handle->check() == false)
		{
			printf("aio_server stop now ...\r\n");
			break;
		}

		time(&now);
		if (now - last >= 1)
		{
			printf("\r\n------------------------------\r\n");
			rpc_out(); // 输出当前 rpc 队列的数量
			rpc_req_out();
			rpc_read_wait_out();
			last = now;
		}
	}

	// 关闭监听流并释放流对象
	sstream->close();

	// 关闭 RPC 服务
	rpc_manager::get_instance().finish();

	// XXX: 为了保证能关闭监听流，应在此处再 check 一下
	handle->check();
	delete handle;

	rpc_stats_finish();

	if (use_mempool)
	{
		acl_mem_slice_gc();
		acl_mem_slice_destroy();
	}

	return 0;
}
