#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_udp.hpp"
#endif

namespace acl
{

static master_udp* __mu = NULL;

master_udp::master_udp(void)
{
	// 全局静态变量
	acl_assert(__mu == NULL);
	__mu = this;
}

master_udp::~master_udp(void)
{
	close_sstreams();
}

void master_udp::close_sstreams(void)
{
	std::vector<socket_stream*>::iterator it = sstreams_.begin();
	for (; it != sstreams_.end(); ++it)
	{
		// 当在 daemon 运行模式，socket_stream 中的 ACL_VSTREAM
		// 对象将由 lib_acl 库中的 acl_udp_server 内部关闭，所以
		// 此处需要将 ACL_VSTREAM 对象与 socket_stream 解耦
		if (daemon_mode_)
			(*it)->unbind();
		delete *it;
	}

	// 必须清空流集合，因为该函数在析构时还将被调用一次
	sstreams_.clear();
}

static bool has_called = false;

void master_udp::run_daemon(int argc, char** argv)
{
#ifndef ACL_WINDOWS
	// 每个进程只能有一个实例在运行
	acl_assert(has_called == false);
	has_called = true;
	daemon_mode_ = true;

	// 调用 acl 服务器框架中 UDP 服务器模板接口
	acl_udp_server_main(argc, argv, service_main,
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

void master_udp::read_callback(int, ACL_EVENT*, ACL_VSTREAM *sstream, void*)
{
	service_main(sstream, NULL, NULL);
	__count++;
	if (__count_limit > 0 && __count >= __count_limit)
		__stop = true;
}

bool master_udp::run_alone(const char* addrs, const char* path /* = NULL */,
	unsigned int count /* = 1 */)
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
	set_event(eventp);  // 设置基类的事件句柄

	ACL_ARGV* tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER iter;

	acl_foreach(iter, tokens)
	{
		const char* addr = (const char*) iter.data;
		ACL_VSTREAM* sstream = acl_vstream_bind(addr, 0);
		if (sstream == NULL)
		{
			logger_error("bind %s error %s",
				addr, last_serror());
			close_sstreams();
			acl_event_free(eventp);
			acl_argv_free(tokens);
			return false;
		}
		acl_event_enable_read(eventp, sstream, 0,
			read_callback, sstream);
		socket_stream* ss = NEW socket_stream();
		if (ss->open(sstream) == false)
			logger_fatal("open stream error!");
		sstream->context = ss;
		sstreams_.push_back(ss);
	}

	acl_argv_free(tokens);

	// 初始化配置参数
	conf_.load(path);

	service_pre_jail(NULL, NULL);
	service_init(NULL, NULL);

	while (!__stop)
		acl_event_loop(eventp);

	service_exit(NULL, NULL);

	// 必须在调用 acl_event_free 前调用 close_sstreams，因为在关闭
	// 网络流对象时依然有对 ACL_EVENT 引擎的使用
	close_sstreams();
	acl_event_free(eventp);
	return true;
}

//////////////////////////////////////////////////////////////////////////

static void on_close(ACL_VSTREAM* stream, void* ctx)
{
	if (ctx && stream->context == ctx)
	{
		socket_stream* ss = (socket_stream*) ctx;
		delete ss;
	}
}

void master_udp::service_main(ACL_VSTREAM *stream, char*, char**)
{
	acl_assert(__mu != NULL);

	socket_stream* ss = (socket_stream*) stream->context;
	if (ss == NULL)
	{
		// 当本函数第一次被调用时，需要打开 socket_stream 流
		ss = NEW socket_stream();
		if (ss->open(stream) == false)
			logger_fatal("open stream error!");
		stream->context = ss;
		acl_vstream_add_close_handle(stream, on_close, ss);
	}

#ifndef	ACL_WINDOWS
	if (__mu->daemon_mode_)
		acl_watchdog_pat();  // 必须通知 acl_master 框架一下
#endif
	__mu->on_read(ss);
}

void master_udp::service_pre_jail(char*, char**)
{
	acl_assert(__mu != NULL);

#ifndef ACL_WINDOWS
	ACL_EVENT* eventp = acl_udp_server_event();
	__mu->set_event(eventp);
#endif

	__mu->proc_pre_jail();
}

void master_udp::service_init(char*, char**)
{
	acl_assert(__mu != NULL);

#ifndef	ACL_WINDOWS
	if (__mu->daemon_mode_)
	{
		ACL_VSTREAM** streams = acl_udp_server_streams();
		for (int i = 0; streams[i] != NULL; i++)
		{
			socket_stream* ss = NEW socket_stream();
			if (ss->open(streams[i]) == false)
				logger_fatal("open stream error!");
			__mu->sstreams_.push_back(ss);
		}
	}
#endif

	__mu->proc_inited_ = true;
	__mu->proc_on_init();
}

void master_udp::service_exit(char*, char**)
{
	acl_assert(__mu != NULL);
	__mu->proc_on_exit();
}

}  // namespace acl
