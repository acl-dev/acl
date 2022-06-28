#include "stdafx.h"
#include <signal.h>
#include "json/service_struct.h"
#include "json/service_struct.gson.h"
#include "ipc_monitor.h"
#include "tools.h"
#include "guard_report.h"

#include "daemon/json/serialize.h"

#include "master_service.h"

char *var_cfg_guard_manager;

acl::master_str_tbl var_conf_str_tab[] = {
	{ "guard_manager", "master.domain:8390", &var_cfg_guard_manager },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

static int  var_cfg_conn_timeout;
static int  var_cfg_rw_timeout;
static int  var_cfg_connection_idle;

acl::master_int_tbl var_conf_int_tab[] = {
	{ "conn_timeout", 30, &var_cfg_conn_timeout, 0, 0 },
	{ "rw_timeout", 30, &var_cfg_rw_timeout, 0, 0 },
	{ "connetion_idle", 30, &var_cfg_connection_idle, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {
	{ 0, 0 , 0 , 0, 0 }
};


//////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
: service_exit_(false)
, monitor_(NULL)
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
	const char* ptr = coder.get("path");
	if (ptr && *ptr)
		res.path = ptr;

	ptr = coder.get("conf");
	if (ptr && *ptr)
		res.conf = ptr;

	ptr = coder.get("pid");
	if (ptr && *ptr)
	{
		int pid = atoi(ptr);
		res.pid = pid;
	}
	else
		res.pid = -1;

	ptr = coder.get("ver");
	if (ptr && *ptr)
	{
		if (strcasecmp(ptr, "-v") == 0 && !res.path.empty())
			tools::get_version(res.path, res.version);
		else
			res.version = ptr;
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
	guard_report report(var_cfg_guard_manager, ipc_, 10, 10);
	report.report(body);
	logger("report=|%s|", body.c_str());
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
	//logger(">>>proc_pre_jail<<<");
}

void master_service::proc_on_listen(acl::server_socket&)
{
	//logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
	//logger(">>>proc_on_init<<<");

	ipc_.set_limit(0)
		.set_idle(30)
		.set_conn_timeout(var_cfg_conn_timeout)
		.set_rw_timeout(var_cfg_rw_timeout);

	monitor_ = new ipc_monitor(ipc_, var_cfg_connection_idle, service_exit_);
	monitor_->set_detachable(false);
	monitor_->start();
}

static void wait_timeout(int)
{
	exit (1);
}

void master_service::proc_on_exit(void)
{
	//logger(">>>proc_on_exit<<<");
	service_exit_ = true;
	signal(SIGALRM, wait_timeout);
	alarm(10);
	monitor_->wait();
	delete monitor_;
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
