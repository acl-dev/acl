#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/master/master_aio.hpp"

namespace acl
{

static master_aio* __ma = NULL;
static aio_handle* __handle = NULL;

master_aio::master_aio()
{
	// 全局静态变量
	acl_assert(__ma == NULL);
	__ma = this;
}

master_aio::~master_aio()
{
	if (__handle)
	{
		__handle->check();
		if (daemon_mode_ == false)
			delete __handle;
		__handle = NULL;
	}
}

aio_handle* master_aio::get_handle() const
{
	acl_assert(__handle);
	return __handle;
}

static bool has_called = false;

void master_aio::run_daemon(int argc, char** argv)
{
#ifndef WIN32
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	// 调用 acl 服务器框架的单线程非阻塞模板
	acl_aio_app2_main(argc, argv, service_main, NULL,
		ACL_APP_CTL_PRE_JAIL, service_pre_jail,
		ACL_APP_CTL_INIT_FN, service_init,
		ACL_APP_CTL_EXIT_FN, service_exit,
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

static void close_all_listener(std::vector<aio_listen_stream*>& sstreams)
{
	std::vector<aio_listen_stream*>::iterator it = sstreams.begin();
	for (; it != sstreams.end(); ++it)
		(*it)->close();
}

bool master_aio::run_alone(const char* addrs, const char* path /* = NULL */,
	aio_handle_type ht /* = ENGINE_SELECT */)
{
	acl_assert(__handle == NULL);
	daemon_mode_ = false;
#ifdef WIN32
	acl_init();
#endif
	std::vector<aio_listen_stream*> sstreams;
	ACL_ARGV* tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER iter;

	// 初始化配置参数
	conf_.load(path);

	__handle = NEW aio_handle(ht);

	ACL_AIO* aio = __handle->get_handle();
	acl_assert(aio);
	ACL_EVENT* eventp = acl_aio_event(aio);
	set_event(eventp);  // 设置基类的事件句柄

	acl_foreach(iter, tokens)
	{
		const char* addr = (const char*) iter.data;
		aio_listen_stream* sstream = NEW aio_listen_stream(__handle);
		// 监听指定的地址
		if (sstream->open(addr) == false)
		{
			logger_error("listen %s error: %s", addr, last_serror());
			close_all_listener(sstreams);
			// XXX: 为了保证能关闭监听流，应在此处再 check 一下
			__handle->check();
			acl_argv_free(tokens);
			return (false);
		}
		sstream->add_accept_callback(this);
	}
	acl_argv_free(tokens);

	service_pre_jail(NULL);
	service_init(NULL);
	while (true)
	{
		// 如果返回 false 则表示不再继续，需要退出
		if (__handle->check() == false)
		{
			logger("aio_server stop now ...");
			break;
		}
	}
	close_all_listener(sstreams);
	__handle->check();
	service_exit(NULL);
	return true;
}

void master_aio::stop()
{
	acl_assert(__handle);
	__handle->stop();
}

bool master_aio::accept_callback(aio_socket_stream* client)
{
	return on_accept(client);
}

//////////////////////////////////////////////////////////////////////////

void master_aio::service_pre_jail(void*)
{
	acl_assert(__ma != NULL);

#ifndef WIN32
	if (__ma->daemon_mode_)
	{
		acl_assert(__handle == NULL);

		ACL_EVENT* eventp = acl_aio_server_event();
		__ma->set_event(eventp);

		ACL_AIO *aio = acl_aio_server_handle();
		acl_assert(aio);
		__handle = NEW aio_handle(aio);
	}
#endif

	__ma->proc_pre_jail();
}

void master_aio::service_init(void* ctx acl_unused)
{
	acl_assert(__ma != NULL);

	__ma->proc_inited_ = true;
	__ma->proc_on_init();
}

void master_aio::service_exit(void* ctx acl_unused)
{
	acl_assert(__ma != NULL);
	__ma->proc_on_exit();
}

int master_aio::service_main(ACL_SOCKET fd, void*)
{
	acl_assert(__handle);
	acl_assert(__ma);

	aio_socket_stream* stream = NEW aio_socket_stream(__handle, fd);
	__ma->on_accept(stream);
	return 0;
}

}  // namespace acl
