#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_threads2.hpp"

namespace acl
{

static master_threads2* __mt = NULL;

master_threads2::master_threads2(void)
{
	// 全局静态变量
	acl_assert(__mt == NULL);
	__mt = this;
}

master_threads2::~master_threads2(void)
{
}

static bool has_called = false;

void master_threads2::run_daemon(int argc, char** argv)
{
#ifndef WIN32
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	// 调用 acl 服务器框架的多线程模板
	acl_threads_server_main(argc, argv, service_main, NULL,
		ACL_MASTER_SERVER_ON_ACCEPT, service_on_accept,
		ACL_MASTER_SERVER_ON_HANDSHAKE, service_on_handshake,
		ACL_MASTER_SERVER_ON_TIMEOUT, service_on_timeout,
		ACL_MASTER_SERVER_ON_CLOSE, service_on_close,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT_TIMER, service_exit_timer,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_THREAD_INIT, thread_init,
		ACL_MASTER_SERVER_THREAD_EXIT, thread_exit,
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_END);
#else
	logger_fatal("no support win32 yet!");
#endif
}

//////////////////////////////////////////////////////////////////////////

static bool __stop = false;
static int  __count_limit = 1;
static int  __count = 0;
static acl_pthread_pool_t* __thread_pool = NULL;

static void close_sstreams(ACL_EVENT* event, std::vector<ACL_VSTREAM*>& streams)
{
	std::vector<ACL_VSTREAM*>::iterator it = streams.begin();
	for (; it != streams.end(); ++it)
	{
		acl_event_disable_readwrite(event, *it);
		acl_vstream_close(*it);
	}

	streams.clear();
}

bool master_threads2::run_alone(const char* addrs, const char* path /* = NULL */,
	unsigned int count /* = 1 */, int threads_count /* = 1 */)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
	acl_assert(addrs && *addrs);

	__count_limit = count;

	std::vector<ACL_VSTREAM*> sstreams;

#ifdef WIN32
	acl_init();
	ACL_EVENT* eventp = acl_event_new_select_thr(1, 0);
#else
	ACL_EVENT* eventp = acl_event_new_kernel_thr(1, 0);
#endif

	set_event(eventp);  // 设置基类的事件句柄

	ACL_ARGV*  tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER   iter;

	acl_foreach(iter, tokens)
	{
		const char* addr = (const char*) iter.data;
		ACL_VSTREAM* sstream = acl_vstream_listen(addr, 128);
		if (sstream == NULL)
		{
			logger_error("listen %s error(%s)",
				addr, acl_last_serror());
			acl_argv_free(tokens);
			close_sstreams(eventp, sstreams);
			acl_event_free(eventp);
			return false;
		}
		acl_event_enable_listen(eventp, sstream, 0,
			listen_callback, sstream);
		sstreams.push_back(sstream);
	}

	acl_argv_free(tokens);

	// 初始化配置参数
	conf_.load(path);

	service_pre_jail(NULL);
	service_init(NULL);

	if (threads_count > 1)
	{
		__thread_pool = acl_thread_pool_create(threads_count, 120);
		acl_pthread_pool_atinit(__thread_pool, thread_begin, NULL);
		acl_pthread_pool_atfree(__thread_pool, thread_finish, NULL);
	}
	else
		thread_init(NULL);

	while (!__stop)
		acl_event_loop(eventp);

	if (__thread_pool)
		acl_pthread_pool_destroy(__thread_pool);
	else
		thread_exit(NULL);

	service_exit(NULL);

	// 必须在调用 acl_event_free 前调用 close_sstreams，因为在关闭
	// 网络流对象时依然有对 ACL_EVENT 引擎的使用
	close_sstreams(eventp, sstreams);
	acl_event_free(eventp);
	eventp = NULL;

	return true;
}

void master_threads2::listen_callback(int, ACL_EVENT*, ACL_VSTREAM* sstream, void*)
{
	ACL_VSTREAM* client = acl_vstream_accept(sstream, NULL, 0);

	if (client == NULL)
	{
		logger_error("accept error(%s)", acl_last_serror());
		__stop = true;
	}
	else if (__thread_pool != NULL)
	{
		acl_pthread_pool_add(__thread_pool, thread_run, client);
		__count++;
	}
	else
	{
		// 单线程方式串行处理
		run_once(client);
		__count++;
	}
}

int master_threads2::thread_begin(void* arg)
{
	thread_init(arg);
	return 0;
}

void master_threads2::thread_finish(void* arg)
{
	thread_exit(arg);
}

void master_threads2::thread_run(void* arg)
{
	ACL_VSTREAM* client = (ACL_VSTREAM*) arg;
	run_once(client);
}

void master_threads2::run_once(ACL_VSTREAM* client)
{
	if (service_on_accept(client) != 0)
		return;

	socket_stream* stream = (socket_stream*) client->context;
	acl_assert(stream);
	ACL_SOCKET fd = stream->sock_handle();
	int   timeout = stream->get_rw_timeout();
	int   ret;

	while (true)
	{
		if (ACL_VSTREAM_BFRD_CNT(client) > 0)
		{
			// 当函数返回 1 时表示 client 已经被关闭了
			if (service_main(client, NULL) == 1)
				break;
			continue;
		}

		// acl_read_wait 当 timeout 为 -1 时才是完全阻塞
		// 等待连接有数据可读，当为 0 时则会立即返回，当
		// > 0 时则等待最多指定超时时间
		if(acl_read_wait(fd, timeout > 0 ? timeout : -1) == 0)
			client->sys_read_ready = 1;
		else if (service_on_timeout(client, NULL) == 0)
			continue;
		else
		{
			service_on_close(client, NULL);

			// stream 对象会在 service_on_close 中被删除，
			// 但在删除时并不会真正关闭套接流，所以需要在
			// 此处关闭套接字流
			acl_vstream_close(client);
			break;
		}

		// 返回 -1 表示需要关闭该客户端连接
		if ((ret = service_main(client, NULL)) == -1)
		{
			service_on_close(client, NULL);

			// stream 对象会在 service_on_close 中被删除，
			// 但在删除时并不会真正关闭套接流，所以需要在
			// 此处关闭套接字流
			acl_vstream_close(client);
			break;
		}

		// service_main 只能返回 0 或 -1
		acl_assert(ret == 0);
	}

	if (__count_limit > 0 && __count >= __count_limit)
		__stop = true;
}

//////////////////////////////////////////////////////////////////////////

void master_threads2::service_pre_jail(void*)
{
	acl_assert(__mt != NULL);

#ifndef WIN32
	if (__mt->daemon_mode())
	{
		ACL_EVENT* eventp = acl_threads_server_event();
		__mt->set_event(eventp);
	}
#endif

	__mt->proc_pre_jail();
}

void master_threads2::service_init(void*)
{
	acl_assert(__mt != NULL);

	__mt->proc_inited_ = true;
	__mt->proc_on_init();
}

int master_threads2::service_exit_timer(size_t nclients, size_t nthreads)
{
	acl_assert(__mt != NULL);
	return __mt->proc_exit_timer(nclients, nthreads) == true ? 1 : 0;
}

void master_threads2::service_exit(void*)
{
	acl_assert(__mt != NULL);
	__mt->proc_on_exit();
}

int master_threads2::thread_init(void*)
{
	acl_assert(__mt != NULL);
	__mt->thread_on_init();
	return 0;
}

void master_threads2::thread_exit(void*)
{
	acl_assert(__mt != NULL);
	__mt->thread_on_exit();
}

int master_threads2::service_on_accept(ACL_VSTREAM* client)
{
	// client->context 不应被占用
	if (client->context != NULL)
		logger_fatal("client->context not null!");

	socket_stream* stream = NEW socket_stream();

	// 设置 client->context 为流对象，该流对象将统一在
	// service_on_close 中被释放
	client->context = stream;

	if (stream->open(client) == false)
	{
		logger_error("open stream error(%s)", acl_last_serror());
		// 返回 -1 由上层框架调用 service_on_close 过程，在里面
		// 释放 stream 对象
		return -1;
	}

	acl_assert(__mt != NULL);

	// 如果子类的 thread_on_accept 方法返回 false，则直接返回给上层
	// 框架 -1，由上层框架再调用 service_on_close 过程，从而在该过程
	// 中将 stream 对象释放
	if (__mt->thread_on_accept(stream) == false)
		return -1;

	// 返回 0 表示可以继续处理该客户端连接，从而由上层框架将其置入
	// 读监控集合中
	return 0;
}

int master_threads2::service_on_handshake(ACL_VSTREAM *client)
{
	acl_assert(__mt != NULL);

	// client->context 在 service_on_accept 中被设置
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	if (__mt->thread_on_handshake(stream) == true)
		return 0;
	return -1;
}

int master_threads2::service_main(ACL_VSTREAM *client, void*)
{
	acl_assert(__mt != NULL);

	// client->context 在 service_on_accept 中被设置
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	// 调用子类的虚函数实现，如果返回 true 表示让框架继续监控该连接流，
	// 否则需要关闭该流
	// 给上层框架返回 0 表示将与该连接保持长连接
	if (__mt->thread_on_read(stream) == true)
		return 0;

	// 返回 -1 表示由上层框架真正关闭流，上层框架在真正关闭流前
	// 将会回调 service_on_close 过程进行流关闭前的善后处理工作，
	// stream 对象将在 service_on_close 中被释放
	return -1;
}

int master_threads2::service_on_timeout(ACL_VSTREAM* client, void*)
{
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	acl_assert(__mt != NULL);

	return __mt->thread_on_timeout(stream) == true ? 0 : -1;
}

void master_threads2::service_on_close(ACL_VSTREAM* client, void*)
{
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	acl_assert(__mt != NULL);

	// 调用子类函数对将要关闭的流进行善后处理
	__mt->thread_on_close(stream);

	// 解释与连接流的绑定关系，这样可以防止在删除流对象时
	// 真正关闭了连接流，因为该流连接需要在本函数返回后由
	// 框架自动关闭
	(void) stream->unbind();
	delete stream;
}

}  // namespace acl
