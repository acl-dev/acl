#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_proc.hpp"
#endif

namespace acl
{
static master_proc* __mp = NULL;

master_proc::master_proc()
{
	acl_assert(__mp == NULL);
	__mp = this;
}

master_proc::~master_proc()
{

}

static bool has_called = false;

void master_proc::run_daemon(int argc, char** argv)
{
#ifdef ACL_WINDOWS
	logger_fatal("not support ACL_WINDOWS!");
#else
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	acl_single_server_main(argc, argv, service_main,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		0);
#endif
}

//////////////////////////////////////////////////////////////////////////

static int  __count_limit = 1;
static int  __count = 0;
static bool __stop = false;

static void close_all_listener(std::vector<ACL_VSTREAM*>& sstreams)
{
	std::vector<ACL_VSTREAM*>::iterator it = sstreams.begin();
	for (; it != sstreams.end(); ++it)
		acl_vstream_close(*it);
}

void master_proc::listen_callback(int, ACL_EVENT*, ACL_VSTREAM *sstream, void*)
{
	ACL_VSTREAM* client = acl_vstream_accept(sstream, NULL, 0);
	if (client == NULL)
	{
		logger_error("accept error %s", last_serror());
		__stop = true;
	}
	else
	{
		service_main(client, NULL, NULL);
		acl_vstream_close(client); // 因为在 service_main 里不会关闭连接

		__count++;
		if (__count_limit > 0 && __count >= __count_limit)
			__stop = true;
	}
}

bool master_proc::run_alone(const char* addrs, const char* path /* = NULL */,
	int   count /* = 1 */)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
	__count_limit = count;
	acl_assert(addrs && *addrs);

#ifdef ACL_WINDOWS
	acl_init();
#endif
	ACL_EVENT* eventp = acl_event_new_select(1, 0);
	set_event(eventp);  // 调用基类方法设置事件引擎句柄

	std::vector<ACL_VSTREAM*> sstreams;
	ACL_ARGV* tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER iter;

	acl_foreach(iter, tokens)
	{
		const char* addr = (const char*) iter.data;
		ACL_VSTREAM* sstream = acl_vstream_listen(addr, 128);
		if (sstream == NULL)
		{
			logger_error("listen %s error %s",
				addr, last_serror());
			close_all_listener(sstreams);
			acl_argv_free(tokens);
			return false;
		}
		acl_event_enable_listen(eventp, sstream, 0,
			listen_callback, sstream);
		sstreams.push_back(sstream);
	}
	acl_argv_free(tokens);

	// 初始化配置参数
	conf_.load(path);

	service_pre_jail(NULL, NULL);
	service_init(NULL, NULL);

	while (!__stop)
		acl_event_loop(eventp);

	close_all_listener(sstreams);
	acl_event_free(eventp);
	service_exit(NULL, NULL);
	return true;
}

//////////////////////////////////////////////////////////////////////////

void master_proc::service_main(ACL_VSTREAM *stream, char*, char**)
{
	socket_stream* client = NEW socket_stream();
	if (client->open(stream) == false)
		logger_fatal("open stream error!");

	acl_assert(__mp != NULL);
#ifndef	ACL_WINDOWS
	if (__mp->daemon_mode_)
		acl_watchdog_pat();  // 必须通知 acl_master 框架一下
#endif
	__mp->on_accept(client);
	client->unbind();
	delete client;
}

void master_proc::service_pre_jail(char*, char**)
{
	acl_assert(__mp != NULL);

#ifndef ACL_WINDOWS
	if (__mp->daemon_mode())
	{
		ACL_EVENT* eventp = acl_single_server_event();
		__mp->set_event(eventp);
	}
#endif

	__mp->proc_pre_jail();
}

void master_proc::service_init(char*, char**)
{
	acl_assert(__mp != NULL);

	__mp->proc_inited_ = true;
	__mp->proc_on_init();
}

void master_proc::service_exit(char*, char**)
{
	acl_assert(__mp != NULL);
	__mp->proc_on_exit();
}

}  // namespace acl
