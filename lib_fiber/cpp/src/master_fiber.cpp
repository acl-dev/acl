#include "stdafx.hpp"
#include "fiber_server.hpp"
#include "fiber/master_fiber.hpp"

namespace acl {

master_fiber::master_fiber(void) {}

master_fiber::~master_fiber(void) {}

static bool has_called = false;

const char* master_fiber::get_conf_path(void) const
{
	if (daemon_mode_) {
		const char* ptr = acl_fiber_server_conf();
		return ptr && *ptr ? ptr : NULL;
	} else {
		return conf_.get_path();
	}
}

void master_fiber::run(int argc, char** argv)
{
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;

	acl_fiber_server_main(argc, argv, service_on_accept, this,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_ON_LISTEN, service_on_listen,
		ACL_MASTER_SERVER_THREAD_INIT, thread_init,
		ACL_MASTER_SERVER_THREAD_INIT_CTX, this,
		ACL_MASTER_SERVER_SIGHUP, service_on_sighup,
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_END);
}

void master_fiber::run_daemon(int argc, char** argv)
{
	daemon_mode_ = true;
	run(argc, argv);
}

bool master_fiber::run_alone(const char* addrs, const char* path /* = NULL */)
{
	acl_assert(addrs && *addrs);

	daemon_mode_ = false;

	int  argc = 0;
	const char *argv[9];
	const char *file_path = acl_process_path();

	argv[argc++] = file_path ? file_path : "unknown";
	argv[argc++] = "-L";
	argv[argc++] = addrs;
	if (path && *path) {
		argv[argc++] = "-f";
		argv[argc++] = path;
	}

	run(argc, (char **) argv);
	return true;
}

//////////////////////////////////////////////////////////////////////////

void master_fiber::service_pre_jail(void* ctx)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf != NULL);
	mf->proc_pre_jail();
}

void master_fiber::service_init(void* ctx)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf != NULL);
	mf->proc_inited_ = true;
	mf->proc_on_init();
}

void master_fiber::service_exit(void* ctx)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf != NULL);
	mf->proc_on_exit();
}

void master_fiber::service_on_listen(void* ctx, ACL_VSTREAM* sstream)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf != NULL);
	server_socket* ss = new server_socket(sstream);
	mf->servers_.push_back(ss);
	logger("listen %s ok, fd=%d", ss->get_addr(), ss->sock_handle());
	mf->proc_on_listen(*ss);
}

void master_fiber::service_on_accept(void* ctx, ACL_VSTREAM *client)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf != NULL);

	socket_stream stream;
	if (!stream.open(client)) {
		logger_error("open stream error(%s)", acl_last_serror());
		return;
	}

	mf->on_accept(stream);
	stream.unbind();
}

void master_fiber::thread_init(void* ctx)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf != NULL);
	mf->thread_on_init();
}

int master_fiber::service_on_sighup(void* ctx, ACL_VSTRING* buf)
{
	master_fiber* mf = (master_fiber *) ctx;
	acl_assert(mf);
	string s;
	bool ret = mf->proc_on_sighup(s);
	if (buf) {
		acl_vstring_strcpy(buf, s.c_str());
	}
	return ret ? 0 : -1;
}

} // namespace acl
