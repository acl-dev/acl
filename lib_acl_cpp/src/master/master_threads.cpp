#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_threads.hpp"

#if 0

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

static bool has_called = false;

void master_threads::run_daemon(int argc, char** argv)
{
#ifndef WIN32
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	// 调用 acl 服务器框架的多线程模板
	acl_ioctl_app_main(argc, argv, service_main, NULL,
		ACL_APP_CTL_ON_ACCEPT, service_on_accept,
		ACL_APP_CTL_ON_TIMEOUT, service_on_timeout,
		ACL_APP_CTL_ON_CLOSE, service_on_close,
		ACL_APP_CTL_PRE_JAIL, service_pre_jail,
		ACL_APP_CTL_INIT_FN, service_init,
		ACL_APP_CTL_EXIT_FN, service_exit,
		ACL_APP_CTL_THREAD_INIT, thread_init,
		ACL_APP_CTL_THREAD_EXIT, thread_exit,
		ACL_APP_CTL_CFG_BOOL, conf_.get_bool_cfg(),
		ACL_APP_CTL_CFG_INT64, conf_.get_int64_cfg(),
		ACL_APP_CTL_CFG_INT, conf_.get_int_cfg(),
		ACL_APP_CTL_CFG_STR, conf_.get_str_cfg(),
		ACL_APP_CTL_END);
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

bool master_threads::run_alone(const char* addrs, const char* path /* = NULL */,
	unsigned int count /* = 1 */, int threads_count /* = 1 */)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
	acl_assert(addrs && *addrs);

	__count_limit = count;

#ifdef WIN32
	acl_init();
#endif

	std::vector<ACL_VSTREAM*> sstreams;
	ACL_EVENT* eventp = acl_event_new_select_thr(1, 0);
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

void master_threads::listen_callback(int, ACL_EVENT*, ACL_VSTREAM* sstream, void*)
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

int master_threads::thread_begin(void* arg)
{
	thread_init(arg);
	return 0;
}

void master_threads::thread_finish(void* arg)
{
	thread_exit(arg);
}

void master_threads::thread_run(void* arg)
{
	ACL_VSTREAM* client = (ACL_VSTREAM*) arg;
	run_once(client);
}

void master_threads::run_once(ACL_VSTREAM* client)
{
	if (service_on_accept(client) != 0)
		return;

	socket_stream* stream = (socket_stream*) client->context;
	acl_assert(stream);
	ACL_SOCKET fd = stream->sock_handle();
	int   timeout = stream->get_rw_timeout();

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
			// 删除流对象时会同时关闭套接字
			delete stream;
			break;
		}

		// 当函数返回 1 时表示 client 已经被关闭了
		if (service_main(client, NULL) == 1)
			break;
	}

	if (__count_limit > 0 && __count >= __count_limit)
		__stop = true;
}

//////////////////////////////////////////////////////////////////////////

void master_threads::service_pre_jail(void*)
{
	acl_assert(__mt != NULL);

#ifndef WIN32
	if (__mt->daemon_mode())
	{
		ACL_EVENT* eventp = acl_ioctl_server_event();
		__mt->set_event(eventp);
	}
#endif

	__mt->proc_pre_jail();
}

void master_threads::service_init(void*)
{
	acl_assert(__mt != NULL);

	__mt->proc_inited_ = true;
	__mt->proc_on_init();
}

void master_threads::service_exit(void*)
{
	acl_assert(__mt != NULL);
	__mt->proc_on_exit();
}

void master_threads::thread_init(void*)
{
	acl_assert(__mt != NULL);
	__mt->thread_on_init();
}

void master_threads::thread_exit(void*)
{
	acl_assert(__mt != NULL);
	__mt->thread_on_exit();
}

int master_threads::service_main(ACL_VSTREAM *client, void*)
{
	acl_assert(__mt != NULL);

	// client->context 在 service_on_accept 中被设置
	socket_stream* stream = (socket_stream*) client->context;
	if (stream == NULL)
		logger_fatal("client->context is null!");

	// 调用子类的虚函数实现，如果返回 true 表示让框架继续监控该连接流
	// 否则需要关闭该流
	if (__mt->thread_on_read(stream) == true)
		return 0;  // 与该连接保持长连接
	else
	{
		__mt->thread_on_close(stream);
		delete stream;
		return 1;  // 此连接已经关闭
	}
}

int master_threads::service_on_accept(ACL_VSTREAM* client)
{
	// client->context 不应被占用
	if (client->context != NULL)
		logger_fatal("client->context not null!");

	socket_stream* stream = NEW socket_stream();
	if (stream->open(client) == false)
	{
		logger_error("open stream error(%s)", acl_last_serror());
		delete stream;
		return -1;
	}
	// 设置 client->context 为流对象
	client->context = stream;

	acl_assert(__mt != NULL);

	if (__mt->thread_on_accept(stream) == false)
	{
		client->context = NULL;
		// 解释与连接流的绑定关系，这样可以防止在删除流对象时
		// 真正关闭了连接流，因为该流连接需要在本函数返回后由
		// 框架自动关闭
		(void) stream->unbind();
		// 删除流对象
		delete stream;
		// 让框架关闭连接流
		return -1;
	}
	return 0;
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
	__mt->thread_on_close(stream);
}

}  // namespace acl

#endif

