#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/master/master_udp.hpp"

namespace acl
{

static master_udp* __mu = NULL;

master_udp::master_udp(void)
{
	// 全局静态变量
	acl_assert(__mu == NULL);
	__mu = this;
}

static bool has_called = false;

void master_udp::run_daemon(int argc, char** argv)
{
#ifndef WIN32
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

static void close_all_streams(std::vector<ACL_VSTREAM*>& sstreams)
{
	std::vector<ACL_VSTREAM*>::iterator it = sstreams.begin();
	for (; it != sstreams.end(); ++it)
		acl_vstream_close(*it);
}

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

#ifdef WIN32
	acl_init();
#endif
	ACL_EVENT* eventp = acl_event_new_select(1, 0);
	std::vector<ACL_VSTREAM*> sstreams;
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
			close_all_streams(sstreams);
			acl_argv_free(tokens);
			return false;
		}
		acl_event_enable_read(eventp, sstream, 0,
			read_callback, sstream);
		sstreams.push_back(sstream);
	}
	acl_argv_free(tokens);

	// 初始化配置参数
	conf_.load(path);

	service_pre_jail(NULL, NULL);
	service_init(NULL, NULL);

	while (!__stop)
		acl_event_loop(eventp);

	close_all_streams(sstreams);
	acl_event_free(eventp);
	service_exit(NULL, NULL);
	return true;
}

//////////////////////////////////////////////////////////////////////////

void master_udp::service_main(ACL_VSTREAM *stream, char*, char**)
{
	socket_stream* ss = NEW socket_stream();
	if (ss->open(stream) == false)
		logger_fatal("open stream error!");

	acl_assert(__mu != NULL);
#ifndef	WIN32
	if (__mu->daemon_mode_)
		acl_watchdog_pat();  // 必须通知 acl_master 框架一下
#endif
	__mu->on_read(ss);
	ss->unbind();
	delete ss;
}

void master_udp::service_pre_jail(char*, char**)
{
	acl_assert(__mu != NULL);
	__mu->proc_pre_jail();
}

void master_udp::service_init(char*, char**)
{
	acl_assert(__mu != NULL);
	__mu->proc_inited_ = true;
	__mu->proc_on_init();
}

void master_udp::service_exit(char*, char**)
{
	acl_assert(__mu != NULL);
	__mu->proc_on_exit();
}

}  // namespace acl
