#include "stdafx.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

char *var_cfg_str;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "str", "test_msg", &var_cfg_str },

	{ 0, 0, 0 }
};

int  var_cfg_bool;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "bool", 1, &var_cfg_bool },

	{ 0, 0, 0 }
};

int  var_cfg_rw_timeout;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "rw_timeout", 30, &var_cfg_rw_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int  var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

//////////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

bool master_service::thread_on_read(acl::socket_stream* conn)
{
	acl::string buf;

	if (!conn->gets(buf)) {
		logger_warn("gets error from %s, fd %d",
			conn->get_peer(), conn->sock_handle());
		return false;
	} else if(conn->format("%s\r\n", buf.c_str()) == -1) {
		return false;
	} else if (buf == "quit") {
		return false;
	} else {
		return true;
	}
}

bool master_service::thread_on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd: %d", conn->get_peer(true),
		conn->sock_handle());
	conn->set_rw_timeout(var_cfg_rw_timeout);
	if (var_cfg_rw_timeout > 0) {
		conn->set_tcp_non_blocking(true);
	}
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
}

void master_service::thread_on_init(void)
{
}

void master_service::thread_on_exit(void)
{
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
	logger(">>>proc_on_init<<<");
}

bool master_service::proc_exit_timer(size_t nclients, size_t nthreads)
{
	if (nclients == 0) {
		logger("clients count: %d, threads count: %d",
			(int) nclients, (int) nthreads);
		return true;
	}

	return false;
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
