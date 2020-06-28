#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/server_socket.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/master/master_aio.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

master_aio::master_aio(void) : handle_(NULL) {}

master_aio::~master_aio(void)
{
	if (!daemon_mode_) {
		delete handle_;
	}
}

aio_handle* master_aio::get_handle(void) const
{
	acl_assert(handle_);
	return handle_;
}

static bool __has_called = false;

void master_aio::run_daemon(int argc, char** argv)
{
#ifndef ACL_WINDOWS
	// 每个进程只能有一个实例在运行
	acl_assert(__has_called == false);
	__has_called = true;
	daemon_mode_ = true;

	// 调用 acl 服务器框架的单线程非阻塞模板
	acl_aio_server2_main(argc, argv, service_main,
		ACL_MASTER_SERVER_CTX, this,
		ACL_MASTER_SERVER_ON_LISTEN, service_on_listen,
		ACL_MASTER_SERVER_PRE_INIT, service_pre_jail,
		ACL_MASTER_SERVER_POST_INIT, service_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		ACL_MASTER_SERVER_SIGHUP, service_on_sighup,
		ACL_MASTER_SERVER_BOOL_TABLE, conf_.get_bool_cfg(),
		ACL_MASTER_SERVER_INT64_TABLE, conf_.get_int64_cfg(),
		ACL_MASTER_SERVER_INT_TABLE, conf_.get_int_cfg(),
		ACL_MASTER_SERVER_STR_TABLE, conf_.get_str_cfg(),
		ACL_MASTER_SERVER_END);
#else
	logger_fatal("no support win32 yet!");
#endif
}

const char* master_aio::get_conf_path(void) const
{
#ifndef ACL_WINDOWS
	if (daemon_mode_) {
		const char* ptr = acl_aio_server_conf();
		return ptr && *ptr ? ptr : NULL;
	}
#endif
	return conf_.get_path();
}

//////////////////////////////////////////////////////////////////////////

static void close_all_listener(std::vector<aio_listen_stream*>& sstreams)
{
	std::vector<aio_listen_stream*>::iterator it = sstreams.begin();
	for (; it != sstreams.end(); ++it) {
		(*it)->close();
	}
}

bool master_aio::run_alone(const char* addrs, const char* path /* = NULL */,
	aio_handle_type ht /* = ENGINE_SELECT */)
{
	acl_assert(__has_called == false);
	__has_called = true;
	daemon_mode_ = false;

#ifdef ACL_WINDOWS
	acl_cpp_init();
#endif
	std::vector<aio_listen_stream*> sstreams;
	ACL_ARGV* tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER iter;

	// 初始化配置参数
	conf_.load(path);

	handle_ = NEW aio_handle(ht);

	ACL_AIO* aio = handle_->get_handle();
	acl_assert(aio);
	ACL_EVENT* eventp = acl_aio_event(aio);
	set_event(eventp);  // 设置基类的事件句柄

	acl_foreach(iter, tokens) {
		const char* addr = (const char*) iter.data;
		aio_listen_stream* sstream = NEW aio_listen_stream(handle_);
		// 监听指定的地址
		if (!sstream->open(addr)) {
			logger_error("listen %s error: %s", addr, last_serror());
			close_all_listener(sstreams);
			// XXX: 为了保证能关闭监听流，应在此处再 check 一下
			handle_->check();
			acl_argv_free(tokens);
			return (false);
		}

		service_on_listen(this, sstream->get_vstream());
		sstream->add_accept_callback(this);
	}
	acl_argv_free(tokens);

	service_pre_jail(this);
	service_init(this);
	while (true) {
		// 如果返回 false 则表示不再继续，需要退出
		if (!handle_->check()) {
			logger("aio_server stop now ...");
			break;
		}
	}
	close_all_listener(sstreams);
	handle_->check();
	service_exit(this);
	return true;
}

void master_aio::stop(void)
{
	acl_assert(handle_);
	handle_->stop();
}

bool master_aio::accept_callback(aio_socket_stream* client)
{
	return on_accept(client);
}

void master_aio::push_back(server_socket* ss)
{
	thread_mutex_guard guard(lock_);
	servers_.push_back(ss);
}

//////////////////////////////////////////////////////////////////////////

class aio_close_callback : public aio_callback
{
public:
	aio_close_callback(aio_socket_stream* ss)
	{
		stream_ = ss->get_astream();
	}

	~aio_close_callback(void) {}

protected:
	void close_callback(void)
	{
#ifndef ACL_WINDOWS
		// 通过下面调用通知服务器框架目前已经处理的连接个数，便于
		// 服务器框架半驻留操作
		acl_aio_server_on_close(stream_);
#endif // !ACL_WINDOWS
		delete this;
	}

private:
	ACL_ASTREAM *stream_;
};

//////////////////////////////////////////////////////////////////////////

void master_aio::service_pre_jail(void* ctx)
{
	master_aio* ma = (master_aio *) ctx;
	acl_assert(ma != NULL);

#ifndef ACL_WINDOWS
	if (ma->daemon_mode_) {
		acl_assert(ma->handle_ == NULL);

		ACL_EVENT* eventp = acl_aio_server_event();
		ma->set_event(eventp);

		ACL_AIO *aio = acl_aio_server_handle();
		acl_assert(aio);
		ma->handle_ = NEW aio_handle(aio);
	}
#endif

	ma->proc_pre_jail();
}

void master_aio::service_init(void* ctx)
{
	master_aio* ma = (master_aio *) ctx;
	acl_assert(ma != NULL);

	ma->proc_inited_ = true;
	ma->proc_on_init();
}

void master_aio::service_exit(void* ctx)
{
	master_aio* ma = (master_aio *) ctx;
	acl_assert(ma != NULL);
	ma->proc_on_exit();
}

void master_aio::service_main(ACL_SOCKET fd, void* ctx)
{
	master_aio* ma = (master_aio *) ctx;
	acl_assert(ma->handle_);
	acl_assert(ma);

	aio_socket_stream* stream = NEW aio_socket_stream(ma->handle_, fd);

	aio_close_callback* callback = NEW aio_close_callback(stream);
	stream->add_close_callback(callback);

	ma->on_accept(stream);
}

void master_aio::service_on_listen(void* ctx, ACL_VSTREAM *sstream)
{
	master_aio* ma = (master_aio *) ctx;
	acl_assert(ma);
	server_socket* ss = NEW server_socket(sstream);
	ma->push_back(ss);
	ma->proc_on_listen(*ss);
}

int master_aio::service_on_sighup(void* ctx, ACL_VSTRING* buf)
{
	master_aio* ma = (master_aio *) ctx;
	acl_assert(ma);
	string s;
	bool ret = ma->proc_on_sighup(s);
	if (buf) {
		acl_vstring_strcpy(buf, s.c_str());
	}
	return ret ? 0 : -1;
}

}  // namespace acl

#endif // ACL_CLIENT_ONLY
