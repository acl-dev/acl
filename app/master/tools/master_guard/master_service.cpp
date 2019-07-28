#include "stdafx.h"
#include <signal.h>
#include "ipc_monitor.h"
#include "action/service_list.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// ÅäÖÃÄÚÈÝÏî

char *var_cfg_guard_manager;
char *var_cfg_master_ctld;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "guard_manager", "master.domain:8390", &var_cfg_guard_manager },
	{ "master_ctld", "127.0.0.1:8290", &var_cfg_master_ctld },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

int   var_cfg_conn_timeout;
int   var_cfg_rw_timeout;
int   var_cfg_connection_idle;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "conn_timeout", 30, &var_cfg_conn_timeout, 0, 0 },
	{ "rw_timeout", 30, &var_cfg_rw_timeout, 0, 0 },
	{ "connection_idle", 30, &var_cfg_connection_idle, 0, 0 },

	{ 0, 0, 0, 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

//////////////////////////////////////////////////////////////////////////////

master_service::master_service()
: service_exit_(false)
, monitor_(NULL)
{
}

master_service::~master_service()
{
}

void master_service::on_trigger()
{
	service_list action(var_cfg_master_ctld, var_cfg_guard_manager, ipc_);
	action.run();
}

void master_service::proc_on_init()
{
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

void master_service::proc_on_exit()
{
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
