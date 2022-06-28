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

void master_service::on_accept(acl::socket_stream* conn)
{
	logger("connect from %s, fd %d", conn->get_peer(),
		conn->sock_handle());

	conn->set_rw_timeout(var_cfg_rw_timeout);
	if (var_cfg_rw_timeout > 0)
		conn->set_tcp_non_blocking(true);

	acl::string buf;
	while (true) {
		if (!conn->gets(buf, false)) {
			break;
		}
		if (conn->write(buf) == -1) {
			break;
		}
		if (buf == "quit") {
			break;
		}
	}

	logger("disconnect from %s", conn->get_peer());
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
	logger(">>>proc_on_init<<<");
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
