#include "stdafx.h"
#include "http_servlet.h"
#include "master_service.h"

char *var_cfg_manager_addr;
char *var_cfg_access_url;
char *var_cfg_home_path;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "manager_addr", "127.0.0.1:10080", &var_cfg_manager_addr },
	{ "access_url", "http://127.0.0.1:10080/dispatch_manager?type=xml",
		&var_cfg_access_url },
	{ "home_path", "./html", &var_cfg_home_path },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

acl::master_int_tbl var_conf_int_tab[] = {

	{ 0, 0, 0, 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {
	{ 0, 0, 0, 0, 0 }
};


//////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

void master_service::on_accept(acl::socket_stream& conn)
{
	logger("connect from %s, fd %d", conn.get_peer(), conn.sock_handle());

	conn.set_rw_timeout(30);

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(&conn, &session);

	// charset: big5, gb2312, gb18030, gbk, utf-8
	servlet.setLocalCharset("utf-8");

	while(servlet.doRun()) {}

	logger("disconnect from %s", conn.get_peer());
}

void master_service::proc_pre_jail(void)
{
}

void master_service::proc_on_init(void)
{
}

void master_service::proc_on_exit(void)
{
}
