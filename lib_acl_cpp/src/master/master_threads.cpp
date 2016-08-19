#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_threads.hpp"
#endif

namespace acl
{

static master_threads* __mt = NULL;

master_threads::master_threads(void)
{
	// 全局静态变量
	acl_assert(__mt == NULL);
	__mt = this;
}

master_threads::~master_threads(void)
{
}

//////////////////////////////////////////////////////////////////////////

static bool has_called = false;

void master_threads::run_daemon(int argc, char** argv)
{
#ifndef ACL_WINDOWS
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

bool master_threads::run_alone(const char* addrs, const char* path /* = NULL */,
	unsigned int, int)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
	acl_assert(addrs && *addrs);

	int  argc = 0;
	const char *argv[9];

	const char* proc = acl_process_path();
	argv[argc++] = proc ? proc : "demo";
	argv[argc++] = "-L";
	argv[argc++] = addrs;
	if (path && *path)
	{
		argv[argc++] = "-f";
		argv[argc++] = path;
	}

	// 调用 acl 服务器框架的多线程模板
	acl_threads_server_main(argc, (char**) argv, service_main, NULL,
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

	return true;
}

//////////////////////////////////////////////////////////////////////////

void master_threads::thread_disable_read(socket_stream* stream)
{
	ACL_EVENT* event = get_event();

	if (event == NULL)
		logger_error("event NULL");
	else
		acl_event_disable_readwrite(event, stream->get_vstream());
}

void master_threads::thread_enable_read(socket_stream* stream)
{
	ACL_EVENT* event = get_event();
	if (event == NULL)
	{
		logger_error("event NULL");
		return;
	}

	acl_pthread_pool_t* threads = acl_threads_server_threads();
	if (threads != NULL)
		acl_threads_server_enable_read(event, threads,
			stream->get_vstream());
	else
		logger_error("threads NULL!");
}

//////////////////////////////////////////////////////////////////////////

void master_threads::service_pre_jail(void*)
{
	acl_assert(__mt != NULL);

	ACL_EVENT* eventp = acl_threads_server_event();
	__mt->set_event(eventp);

	__mt->proc_pre_jail();
}

void master_threads::service_init(void*)
{
	acl_assert(__mt != NULL);

	__mt->proc_inited_ = true;
	__mt->proc_on_init();
}

int master_threads::service_exit_timer(size_t nclients, size_t nthreads)
{
	acl_assert(__mt != NULL);
	return __mt->proc_exit_timer(nclients, nthreads) == true ? 1 : 0;
}

void master_threads::service_exit(void*)
{
	acl_assert(__mt != NULL);
	__mt->proc_on_exit();
}

int master_threads::thread_init(void*)
{
	acl_assert(__mt != NULL);
	__mt->thread_on_init();
	return 0;
}

void master_threads::thread_exit(void*)
{
	acl_assert(__mt != NULL);
	__mt->thread_on_exit();
}

int master_threads::service_on_accept(ACL_VSTREAM* client)
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

	// 如果子类的 thread_on_handshake 方法返回 false，则直接返回给上层
	// 框架 -1，由上层框架再调用 service_on_close 过程，从而在该过程
	// 中将 stream 对象释放
	if (__mt->thread_on_handshake(stream) == false)
		return -1;

	// 返回 0 表示可以继续处理该客户端连接，从而由上层框架将其置入
	// 读监控集合中
	return 0;
}

int master_threads::service_on_handshake(ACL_VSTREAM *client)
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

int master_threads::service_main(ACL_VSTREAM *client, void*)
{
	acl_assert(__mt != NULL);

	// client->context 在 service_on_accept 中被设置
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	// 调用子类的虚函数实现，如果返回 true 表示让框架继续监控该连接流，
	// 否则需要关闭该流
	// 给上层框架返回值函数如下：
	// 0 表示将与该连接保持长连接，且需要继续监控该连接是否可读
	// -1 表示需要关闭该连接
	// 1 表示不再监控该连接

	if (__mt->thread_on_read(stream) == true)
	{
		// 如果子类在返回 true 后不希望框架继续监控流，则直接返回给框架 1
		if (!__mt->keep_read(stream))
			return 1;

		// 否则，需要检查该流是否已经关闭，如果关闭，则必须返回 -1
		if (stream->eof())
		{
			logger_error("DISCONNECTED, CLOSING, FD: %d",
				(int) stream->sock_handle());
			return -1;
		}

		// 返回 0 表示继续监控该流的可读状态
		return 0;
	}

	// 返回 -1 表示由上层框架真正关闭流，上层框架在真正关闭流前
	// 将会回调 service_on_close 过程进行流关闭前的善后处理工作，
	// stream 对象将在 service_on_close 中被释放
	return -1;
}

int master_threads::service_on_timeout(ACL_VSTREAM* client, void*)
{
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	acl_assert(__mt != NULL);

	return __mt->thread_on_timeout(stream) == true ? 0 : -1;
}

void master_threads::service_on_close(ACL_VSTREAM* client, void*)
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
