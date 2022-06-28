#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/server_socket.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_proc.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

master_proc::master_proc(void) : stop_(false), count_limit_(0), count_(0) {}

master_proc::~master_proc(void) {}

static bool __has_called = false;

void master_proc::run_daemon(int argc, char** argv)
{
#ifdef ACL_WINDOWS
	logger_fatal("not support ACL_WINDOWS!");
#else
	// 每个进程只能有一个实例在运行
	acl_assert(__has_called == false);
	__has_called = true;
	daemon_mode_ = true;

	acl_single_server_main(argc, argv, service_main,
		ACL_MASTER_SERVER_CTX, this,
		ACL_MASTER_SERVER_ON_LISTEN, service_on_listen,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_SIGHUP, service_on_sighup,
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		0);
#endif
}

const char* master_proc::get_conf_path(void) const
{
#ifndef ACL_WINDOWS
	if (daemon_mode_) {
		const char* ptr = acl_single_server_conf();
		return ptr && *ptr ? ptr : NULL;
	}
#endif
	return conf_.get_path();
}

//////////////////////////////////////////////////////////////////////////

static void close_all_listener(std::vector<ACL_VSTREAM*>& sstreams)
{
	std::vector<ACL_VSTREAM*>::iterator it = sstreams.begin();
	for (; it != sstreams.end(); ++it) {
		acl_vstream_close(*it);
	}
}

void master_proc::listen_callback(int, ACL_EVENT*, ACL_VSTREAM *sstream,
	void* ctx)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp);

	ACL_VSTREAM* client = acl_vstream_accept(sstream, NULL, 0);
	if (client == NULL) {
		logger_error("accept error %s", last_serror());
		mp->stop_ = true;
	} else {
		service_main(ctx, client);
		acl_vstream_close(client); // 因为在 service_main 里不会关闭连接

		mp->count_++;
		if (mp->count_limit_ > 0 && mp->count_ >= mp->count_limit_) {
			mp->stop_ = true;
		}
	}
}

bool master_proc::run_alone(const char* addrs, const char* path /* = NULL */,
	int   count /* = 1 */)
{
	// 每个进程只能有一个实例在运行
	acl_assert(__has_called == false);
	__has_called = true;
	daemon_mode_ = false;
	count_limit_ = count;
	acl_assert(addrs && *addrs);

#ifdef ACL_WINDOWS
	acl_cpp_init();
#endif
	ACL_EVENT* eventp = acl_event_new_select(1, 0);
	set_event(eventp);  // 调用基类方法设置事件引擎句柄

	std::vector<ACL_VSTREAM*> sstreams;
	ACL_ARGV* tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER iter;

	acl_foreach(iter, tokens) {
		const char* addr = (const char*) iter.data;
		ACL_VSTREAM* sstream = acl_vstream_listen(addr, 128);
		if (sstream == NULL) {
			logger_error("listen %s error %s", addr, last_serror());
			close_all_listener(sstreams);
			acl_argv_free(tokens);
			return false;
		}

		service_on_listen(this, sstream);
		acl_event_enable_listen(eventp, sstream, 0,
			listen_callback, this);
		sstreams.push_back(sstream);
	}
	acl_argv_free(tokens);

	// 初始化配置参数
	conf_.load(path);

	service_pre_jail(this);
	service_init(this);

	while (!stop_) {
		acl_event_loop(eventp);
	}

	close_all_listener(sstreams);
	acl_event_free(eventp);
	service_exit(this);

	return true;
}

//////////////////////////////////////////////////////////////////////////

void master_proc::service_main(void* ctx, ACL_VSTREAM *stream)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp != NULL);

	socket_stream* client = NEW socket_stream();
	if (!client->open(stream)) {
		logger_fatal("open stream error!");
	}

#ifndef	ACL_WINDOWS
	if (mp->daemon_mode_) {
		acl_watchdog_pat();  // 必须通知 acl_master 框架一下
	}
#endif
	mp->on_accept(client);
	client->unbind();
	delete client;
}

void master_proc::service_pre_jail(void* ctx)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp != NULL);

#ifndef ACL_WINDOWS
	if (mp->daemon_mode()) {
		ACL_EVENT* eventp = acl_single_server_event();
		mp->set_event(eventp);
	}
#endif

	mp->proc_pre_jail();
}

void master_proc::service_init(void* ctx)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp != NULL);

	mp->proc_inited_ = true;
	mp->proc_on_init();
}

void master_proc::service_exit(void* ctx)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp != NULL);

	mp->proc_on_exit();
}

void master_proc::service_on_listen(void* ctx, ACL_VSTREAM* sstream)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp != NULL);

	server_socket* ss = NEW server_socket(sstream);
	mp->servers_.push_back(ss);
	server_socket m(sstream);
	mp->proc_on_listen(*ss);
}

int master_proc::service_on_sighup(void* ctx, ACL_VSTRING* buf)
{
	master_proc* mp = (master_proc *) ctx;
	acl_assert(mp != NULL);
	string s;
	bool ret = mp->proc_on_sighup(s);
	if (buf) {
		acl_vstring_strcpy(buf, s.c_str());
	}
	return ret ? 0 : -1;
}

}  // namespace acl

#endif // ACL_CLIENT_ONLY
