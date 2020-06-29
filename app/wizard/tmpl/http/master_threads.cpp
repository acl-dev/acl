#include "stdafx.h"
#include "http_servlet.h"
#include "http_service.h"

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

http_service::http_service(void)
{
	redis_ = NULL;
}

http_service::~http_service(void)
{
}

bool http_service::thread_on_read(acl::socket_stream* conn)
{
	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	if (servlet == NULL) {
		logger_fatal("servlet null!");
	}

	return servlet->doRun();
}

bool http_service::thread_on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd: %d", conn->get_peer(true),
		conn->sock_handle());

	conn->set_rw_timeout(var_cfg_rw_timeout);
	if (var_cfg_rw_timeout > 0) {
		conn->set_tcp_non_blocking(true);
	}

	acl::session* session;
	if (var_cfg_use_redis_session) {
		session = new acl::redis_session(*redis_, var_cfg_max_threads);
	} else {
		session = new acl::memcache_session("127.0.0.1:11211");
	}

	http_servlet* servlet = new http_servlet(handlers_, conn, session);
	conn->set_ctx(servlet);

	return true;
}

bool http_service::thread_on_timeout(acl::socket_stream* conn)
{
	logger("read timeout from %s, fd: %d", conn->get_peer(),
		conn->sock_handle());
	return false;
}

void http_service::thread_on_close(acl::socket_stream* conn)
{
	logger("disconnect from %s, fd: %d", conn->get_peer(),
		conn->sock_handle());

	http_servlet* servlet = (http_servlet*) conn->get_ctx();
	acl::session* session = &servlet->getSession();
	delete session;
	delete servlet;
}

void http_service::thread_on_init(void)
{
}

void http_service::thread_on_exit(void)
{
}

void http_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void http_service::proc_on_init(void)
{
	// create redis cluster for session cluster
	redis_ = new acl::redis_client_cluster;
	redis_->init(NULL, var_cfg_redis_addrs, var_cfg_max_threads,
		var_cfg_conn_timeout, var_cfg_rw_timeout);
}

void http_service::proc_on_exit(void)
{
	delete redis_;
}

bool http_service::proc_exit_timer(size_t nclients, size_t nthreads)
{
	if (nclients == 0) {
		logger("clients count: %d, threads count: %d",
			(int) nclients, (int) nthreads);
		return true;
	}

	return false;
}

bool http_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}

//////////////////////////////////////////////////////////////////////////

http_service& http_service::Get(const char* path, http_handler_t fn)
{
	Service(http_handler_get, path, fn);
	return *this;
}

http_service& http_service::Post(const char* path, http_handler_t fn)
{
	Service(http_handler_post, path, fn);
	return *this;
}

http_service& http_service::Head(const char* path, http_handler_t fn)
{
	Service(http_handler_head, path, fn);
	return *this;
}

http_service& http_service::Put(const char* path, http_handler_t fn)
{
	Service(http_handler_put, path, fn);
	return *this;
}

http_service& http_service::Patch(const char* path, http_handler_t fn)
{
	Service(http_handler_patch, path, fn);
	return *this;
}

http_service& http_service::Connect(const char* path, http_handler_t fn)
{
	Service(http_handler_connect, path, fn);
	return *this;
}

http_service& http_service::Purge(const char* path, http_handler_t fn)
{
	Service(http_handler_purge, path, fn);
	return *this;
}

http_service& http_service::Delete(const char* path, http_handler_t fn)
{
	Service(http_handler_delete, path, fn);
	return *this;
}

http_service& http_service::Options(const char* path, http_handler_t fn)
{
	Service(http_handler_options, path, fn);
	return *this;
}

http_service& http_service::Propfind(const char* path, http_handler_t fn)
{
	Service(http_handler_profind, path, fn);
	return *this;
}

http_service& http_service::Websocket(const char* path, http_handler_t fn)
{
	Service(http_handler_websocket, path, fn);
	return *this;
}

http_service& http_service::Unknown(const char* path, http_handler_t fn)
{
	Service(http_handler_unknown, path, fn);
	return *this;
}

http_service& http_service::Error(const char* path, http_handler_t fn)
{
	Service(http_handler_error, path, fn);
	return *this;
}

void http_service::Service(int type, const char* path, http_handler_t fn)
{
	if (type >= http_handler_get && type < http_handler_max
		&& path && *path) {

		// The path should lookup like as "/xxx/" with
		// lower charactors.

		acl::string buf(path);
		if (buf[buf.size() - 1] != '/') {
			buf += '/';
		}
		buf.lower();
		handlers_[type][buf] = fn;
	}
}
