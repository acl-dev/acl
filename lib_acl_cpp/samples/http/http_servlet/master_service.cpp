#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_redis_servers;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "redis_servers", "127.0.0.1:9000", &var_cfg_redis_servers },

	{ 0, 0, 0 }
};

int  var_cfg_keep_loop;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "keep_loop", 1, &var_cfg_keep_loop },

	{ 0, 0, 0 }
};

int  var_cfg_conn_timeout;
int  var_cfg_rw_timeout;
int  var_cfg_max_threads;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "conn_timeout", 10, &var_cfg_conn_timeout, 0, 0 },
	{ "rw_timeout", 10, &var_cfg_rw_timeout, 0, 0 },
	{ "ioctl_max_threads", 128, &var_cfg_max_threads, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int  var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

static acl::redis_client_cluster* session_server = NULL;

////////////////////////////////////////////////////////////////////////////////

master_service::master_service()
{
}

master_service::~master_service()
{
}

bool master_service::thread_on_read(acl::socket_stream* conn)
{
	//logger("read from %s", conn->get_peer(true));
	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet == NULL)
		logger_fatal("servlet null!");

	acl::session& session = servlet->get_session();
	while (true)
	{
		bool ret = servlet->doRun(session, conn);
		if (ret == false)
			return false;
		if (!var_cfg_keep_loop)
			return true;
	}
}

bool master_service::thread_on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd: %d", conn->get_peer(true),
		conn->sock_handle());
	conn->set_rw_timeout(0);

	// 使用 redis 集群来存储 session
	http_servlet* servlet = new http_servlet(*session_server,
			var_cfg_max_threads);
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
	logger("disconnect from %s, fd: %d", conn->get_peer(true),
		conn->sock_handle());

	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet)
		delete servlet;
}

void master_service::thread_on_init()
{
}

void master_service::thread_on_exit()
{
}

void master_service::proc_on_init()
{
	// 创建 redis 集群客户端对象，并使用 redis 集群来存储 session
	session_server = new acl::redis_client_cluster;
	session_server->init(NULL, var_cfg_redis_servers, var_cfg_max_threads);
}

bool master_service::proc_exit_timer(size_t nclients, size_t nthreads)
{
	if (nclients == 0 || nthreads == 0)
	{
		logger("clients count: %d, threads count: %d",
			(int) nclients, (int) nthreads);
		return true;
	}

	return false;
}

void master_service::proc_on_exit()
{
	delete session_server;
}
