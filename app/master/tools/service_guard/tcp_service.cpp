#include "stdafx.h"
#include "action/guard_action.h"
#include "configure.h"
#include "tcp_service.h"

tcp_service::tcp_service(void)
{
}

tcp_service::~tcp_service(void)
{
}

bool tcp_service::thread_on_read(acl::socket_stream* stream)
{
	int   n;
	char  buf[1500];

	if ((n = stream->read(buf, sizeof(buf) - 1, false)) == -1)
		return false;

	buf[n] = 0;
	logger("read from %s, %d bytes, buf=|%s|", stream->get_peer(), n, buf);

	const char* peer_ip = stream->get_peer();
	if (peer_ip == NULL || *peer_ip == 0) {
		logger_error("can't get peer ip");
		return false;
	}

	guard_action* job = new guard_action(peer_ip, buf);
	threads_->execute(job);
	return true;
}

void tcp_service::thread_on_init(void)
{
}

void tcp_service::thread_on_exit(void)
{
}

void tcp_service::proc_on_init(void)
{
	threads_ = new acl::thread_pool;
	threads_->set_limit(var_cfg_threads_max);
	threads_->set_idle(var_cfg_threads_idle);
	threads_->start();

	var_redis.init(NULL, var_cfg_redis_addrs, var_cfg_threads_max,
		var_cfg_redis_conn_timeout, var_cfg_redis_rw_timeout);
	var_redis.set_password("default", var_cfg_redis_passwd);

	if (var_cfg_main_service_list && *var_cfg_main_service_list)
	{
		ACL_ARGV *tokens  = acl_argv_split(
			var_cfg_main_service_list, ",; \t\r\n");
		ACL_ITER iter;
		acl_foreach(iter, tokens)
		{
			const char* ptr = (const char*) iter.data;
			var_main_service_list[ptr] = true;
		}
		acl_argv_free(tokens);
	}

	logger(">>>proc_on_init<<<");
}

void tcp_service::proc_on_exit(void)
{
	delete threads_;
	logger(">>>proc_on_exit<<<");
}

bool tcp_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
