#include "stdafx.h"
#include "json/service_struct.h"
#include "json/service_struct.gson.h"
#include "tools.h"
#include "guard_report.h"

#include "daemon/json/serialize.h"

#include "master_service.h"

char *var_cfg_guard_manager;
char *var_cfg_master_ctld;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "guard_manager", "master.qiyi.domain:8390", &var_cfg_guard_manager },
	{ "master_ctld", "127.0.0.1:8290", &var_cfg_master_ctld },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

static int  var_cfg_io_timeout;

acl::master_int_tbl var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {
	{ 0, 0 , 0 , 0, 0 }
};


//////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

void master_service::handle(const acl::string& data)
{
	service_dead_res_t res;

	acl::url_coder coder;
	coder.decode(data);
	const char* ptr = coder.get("proc");
	if (ptr && *ptr)
		res.path = ptr;

	ptr = coder.get("ver");
	if (ptr && *ptr)
		res.version = ptr;

	ptr = coder.get("pid");
	if (ptr && *ptr)
	{
		int pid = atoi(ptr);
		res.pid = pid;
	}

	ptr = coder.get("rcpt");
	if (ptr && *ptr)
		res.rcpt = ptr;

	ptr = coder.get("info");
	if (ptr && *ptr)
		res.info = ptr;

	res.status = 200;
	res.cmd    = "service_dead";

	acl::string body;
	serialize<service_dead_res_t>(res, body);
	guard_report report(var_cfg_guard_manager, 10, 10);
	report.report(body);
	printf("body=|%s|\r\n", body.c_str());
}

void master_service::on_accept(acl::socket_stream& conn)
{
	conn.set_rw_timeout(10);

	acl::string buf;

	while (true)
	{
		if (conn.gets(buf) == false)
			break;

		if (!buf.empty())
			handle(buf);
	}
}

void master_service::proc_pre_jail(void)
{
	logger(">>>proc_pre_jail<<<");
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
