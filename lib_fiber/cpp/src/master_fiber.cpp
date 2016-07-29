#include "stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "fiber/master_fiber.hpp"

namespace acl {

static master_fiber* __mf = NULL;

master_fiber::master_fiber(void)
{
	acl_assert(__mf == NULL);
	__mf = this;
}

master_fiber::~master_fiber(void)
{
}

static bool has_called = false;

void master_fiber::run_daemon(int argc, char** argv)
{
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	acl_fiber_server_main(argc, argv, service_main, NULL,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_END);
}

bool master_fiber::run_alone(const char* addrs, const char* path /* = NULL */,
	unsigned int)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = false;
	acl_assert(addrs && *addrs);

	int  argc = 0;
	const char *argv[9];

	argv[argc++] = acl_process_path();
	argv[argc++] = "-L";
	argv[argc++] = addrs;
	if (path && *path)
	{
		argv[argc++] = "-f";
		argv[argc++] = path;
	}

	acl_fiber_server_main(argc, (char**) argv, service_main, NULL,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_END);

	return true;
}

//////////////////////////////////////////////////////////////////////////

void master_fiber::service_pre_jail(void*)
{
	acl_assert(__mf != NULL);
	__mf->proc_pre_jail();
}

void master_fiber::service_init(void*)
{
	acl_assert(__mf != NULL);
	__mf->proc_inited_ = true;
	__mf->proc_on_init();
}

void master_fiber::service_exit(void*)
{
	acl_assert(__mf != NULL);
	__mf->proc_on_exit();
}

void master_fiber::service_main(ACL_VSTREAM *client, void*)
{
	acl_assert(__mf != NULL);

	socket_stream stream;
	if (stream.open(client) == false)
	{
		logger_error("open stream error(%s)", acl_last_serror());
		return;
	}

	__mf->on_accept(stream);
	stream.unbind();
}

} // namespace acl
