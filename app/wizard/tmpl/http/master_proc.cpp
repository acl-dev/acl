#include "stdafx.h"
#include "http_service.h"
#include "http_servlet.h"
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
	service_ = new http_service;
}

master_service::~master_service(void)
{
	delete service_;
}

http_service& master_service::get_service(void) const
{
	return *service_;
}

void master_service::on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd %d", conn->get_peer(),
		conn->sock_handle());

	conn->set_rw_timeout(var_cfg_rw_timeout);
	if (var_cfg_rw_timeout > 0) {
		conn->set_tcp_non_blocking(true);
	}

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(*service_, conn, &session);

	// charset: big5, gb2312, gb18030, gbk, utf-8
	servlet.setLocalCharset("utf-8");

	while (servlet.doRun()) {}

	logger("disconnect from %s", conn->get_peer());
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
}

void master_service::proc_on_exit(void)
{
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}

void master_service::do_cgi(void)
{
	acl::memcache_session session("127.0.0.1:11211");
        http_servlet servlet(*service_, NULL, &session);
        servlet.setLocalCharset("utf-8");
        servlet.doRun();
}
