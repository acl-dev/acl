#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_udp.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

master_udp::master_udp(void) {}

master_udp::~master_udp(void)
{
	for (std::vector<socket_stream*>::iterator it = sstreams_.begin();
		it != sstreams_.end(); ++it) {

		(*it)->unbind();
		delete *it;
	}
}

static bool __has_called = false;

void master_udp::run(int argc, char** argv)
{
	// 调用 acl 服务器框架中 UDP 服务器模板接口
	acl_udp_server_main(argc, argv, service_main,
		ACL_MASTER_SERVER_CTX, this,
		ACL_APP_CTL_THREAD_INIT_CTX, this,
		ACL_MASTER_SERVER_ON_BIND, service_on_bind,
		ACL_MASTER_SERVER_ON_UNBIND, service_on_unbind,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_THREAD_INIT, thread_init,
		ACL_MASTER_SERVER_SIGHUP, service_on_sighup,
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		0);
}

void master_udp::run_daemon(int argc, char** argv)
{
#ifdef ACL_WINDOWS
	logger_fatal("no support win32 yet!");
#else
	// 每个进程只能有一个实例在运行
	acl_assert(__has_called == false);
	__has_called = true;
	daemon_mode_ = true;

	run(argc, argv);
#endif
}

const char* master_udp::get_conf_path(void) const
{
#ifndef ACL_WINDOWS
	if (daemon_mode_) {
		const char* ptr = acl_udp_server_conf();
		return ptr && *ptr ? ptr : NULL;
	}
#endif
	return conf_.get_path();
}

//////////////////////////////////////////////////////////////////////////

bool master_udp::run_alone(const char* addrs, const char* path /* = NULL */,
	unsigned int)
{
#ifdef ACL_WINDOWS
	acl_cpp_init();
#endif

	// 每个进程只能有一个实例在运行
	acl_assert(__has_called == false);
	__has_called = true;
	daemon_mode_ = false;
	acl_assert(addrs && *addrs);

	int  argc = 0;
	const char *argv[6];

	const char* proc = acl_process_path();
	argv[argc++] = proc ? proc : "demo";
	argv[argc++] = "-n";
	argv[argc++] = addrs;
	if (path && *path) {
		argv[argc++] = "-f";
		argv[argc++] = path;
	}
	argv[argc++] = "-r";

	run(argc, (char**) argv);
	return true;
}

void master_udp::push_back(socket_stream* ss)
{
	thread_mutex_guard guard(lock_);
	sstreams_.push_back(ss);
}

void master_udp::remove(socket_stream* ss)
{
	thread_mutex_guard guard(lock_);

	for (std::vector<socket_stream*>::iterator it = sstreams_.begin();
		it != sstreams_.end(); ++it) {

		if (*it == ss) {
			sstreams_.erase(it);
			return;
		}
	}

	logger_error("not found ss=%p", ss);
}

void master_udp::lock(void)
{
	lock_.lock();
}

void master_udp::unlock(void)
{
	lock_.unlock();
}

//////////////////////////////////////////////////////////////////////////

void master_udp::service_main(void* ctx, ACL_VSTREAM *stream)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu != NULL);

	socket_stream* ss = (socket_stream*) stream->context;
	acl_assert(ss);

/*
#ifndef	ACL_WINDOWS
	if (mu->daemon_mode_)
		acl_watchdog_pat();  // 必须通知 acl_master 框架一下
#endif
*/
	mu->on_read(ss);
}

void master_udp::service_pre_jail(void* ctx)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu != NULL);

#ifndef ACL_WINDOWS
	ACL_EVENT* eventp = acl_udp_server_event();
	mu->set_event(eventp);
#endif

	mu->proc_pre_jail();
}

void master_udp::service_init(void* ctx)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu != NULL);

	mu->proc_inited_ = true;
	mu->proc_on_init();
}

void master_udp::service_exit(void* ctx)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu != NULL);
	mu->proc_on_exit();
}

void master_udp::thread_init(void* ctx)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu != NULL);
	mu->thread_on_init();
}

void master_udp::service_on_bind(void* ctx, ACL_VSTREAM* stream)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu);

	socket_stream* ss = NEW socket_stream();
	if (!ss->open(stream)) {
		logger_fatal("open stream error!");
	}
	stream->context = ss;
	mu->push_back(ss);

	mu->proc_on_bind(*ss);
}

void master_udp::service_on_unbind(void* ctx, ACL_VSTREAM* stream)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu);

	socket_stream* ss = (socket_stream *) stream->context;
	if (ss == NULL) {
		logger_error("stream->context null, stream=%p", stream);
		return;
	} else if (ss->get_vstream() != stream) {
		logger_error("invalid stream=%p, ss->get_vstream()=%p",
			stream, ss->get_vstream());
		return;
	}

	mu->proc_on_unbind(*ss);
	mu->remove(ss);
	ss->unbind();
	delete ss;
}

int master_udp::service_on_sighup(void* ctx, ACL_VSTRING* buf)
{
	master_udp* mu = (master_udp *) ctx;
	acl_assert(mu);
	string s;
	bool ret = mu->proc_on_sighup(s);
	if (buf) {
		acl_vstring_strcpy(buf, s.c_str());
	}
	return ret ? 0 : -1;
}

}  // namespace acl

#endif // ACL_CLIENT_ONLY
