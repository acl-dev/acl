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

bool tcp_service::thread_on_read(acl::socket_stream* conn)
{
	acl::tcp_reader reader(*conn);

	acl::string buf;
	if (reader.read(buf) == false) {
		logger("read over from %s", conn->get_peer());
		return false;
	}
	logger_debug(DBG_NET, 1, "read from %s, %d bytes, buf=|%s|",
		conn->get_peer(), (int) buf.size(), buf.c_str());

	const char* peer_ip = conn->get_peer();
	if (peer_ip == NULL || *peer_ip == 0) {
		logger_error("can't get peer ip");
		return false;
	}

	guard_action* job = new guard_action(peer_ip, buf);
	job->run();
	return true;
}

bool tcp_service::thread_on_accept(acl::socket_stream*)
{
	return true;
}

bool tcp_service::thread_on_timeout(acl::socket_stream* conn)
{
	logger_debug(DBG_NET, 2, "read timeout from %s, fd: %d",
		conn->get_peer(), conn->sock_handle());
	return false;
}

void tcp_service::thread_on_close(acl::socket_stream* conn)
{
	logger_debug(DBG_NET, 2, "disconnect from %s, fd: %d",
		conn->get_peer(), conn->sock_handle());
}

void tcp_service::thread_on_init(void)
{
}

void tcp_service::thread_on_exit(void)
{
}

void tcp_service::proc_on_init(void)
{
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
	logger(">>>proc_on_exit<<<");
}

bool tcp_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
