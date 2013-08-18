#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_proc.hpp"

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
#ifdef WIN32
	logger_fatal("not support WIN32!");
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

bool master_proc::run_alone(const char* addr, const char* path /* = NULL */,
	int   count /* = 1 */)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
	acl_assert(addr && *addr);

#ifdef WIN32
	acl_init();
#endif

	ACL_VSTREAM* sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL)
	{
		logger_error("listen %s error(%s)",
			addr, acl_last_serror());
		return false;
	}

	// 初始化配置参数
	conf_.load(path);

	service_pre_jail(NULL, NULL);
	service_init(NULL, NULL);

	int   i = 0;
	while (true)
	{
		ACL_VSTREAM* client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL)
			break;

		service_main(client, NULL, NULL);
		acl_vstream_close(client); // 因为在 service_main 里不会关闭连接

		if (count > 0 && ++i >= count)
			break;
	}

	acl_vstream_close(sstream);
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
#ifndef	WIN32
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
