#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

char *var_cfg_redis_addrs;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "redis_addrs", "127.0.0.1:6379", &var_cfg_redis_addrs },

	{ 0, 0, 0 }
};

int   var_cfg_use_redis_session;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "use_redis_session", 1, &var_cfg_use_redis_session },

	{ 0, 0, 0 }
};

int   var_cfg_conn_timeout;
int   var_cfg_rw_timeout;
int   var_cfg_max_threads;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "rw_timeout", 120, &var_cfg_rw_timeout, 0, 0 },
	{ "ioctl_max_threads", 128, &var_cfg_max_threads, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int   var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

//////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
	redis_ = NULL;
}

master_service::~master_service()
{
}

bool master_service::thread_on_read(acl::socket_stream* conn)
{
	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet == NULL)
		logger_fatal("servlet null!");

	return servlet->doRun();
}

bool master_service::thread_on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd: %d", conn->get_peer(true),
		conn->sock_handle());

	conn->set_rw_timeout(var_cfg_rw_timeout);
	if (var_cfg_rw_timeout > 0)
		conn->set_tcp_non_blocking(true);

	acl::session* session;
	if (var_cfg_use_redis_session)
		session = new acl::redis_session(*redis_, var_cfg_max_threads);
	else
		session = new acl::memcache_session("127.0.0.1:11211");

	http_servlet* servlet = new http_servlet(conn, session);
	conn->set_ctx(servlet);

	return true;
}

bool master_service::thread_on_timeout(acl::socket_stream* conn)
{
	logger("read timeout from %s, fd: %d", conn->get_peer(),
		conn->sock_handle());
	return false;
}

void master_service::thread_on_close(acl::socket_stream* conn)
{
	logger("disconnect from %s, fd: %d", conn->get_peer(),
		conn->sock_handle());

	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	acl::session* session = &servlet->getSession();
	delete session;
	delete servlet;
}

void master_service::thread_on_init()
{
}

void master_service::thread_on_exit()
{
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init()
{
	// create redis cluster for session cluster
	redis_ = new acl::redis_client_cluster;
	redis_->init(NULL, var_cfg_redis_addrs, var_cfg_max_threads,
		var_cfg_conn_timeout, var_cfg_rw_timeout);
}

void master_service::proc_on_exit()
{
	delete redis_;
}

bool master_service::proc_exit_timer(size_t nclients, size_t nthreads)
{
	if (nclients == 0)
	{
		logger("clients count: %d, threads count: %d",
			(int) nclients, (int) nthreads);
		return true;
	}

	return false;
}
