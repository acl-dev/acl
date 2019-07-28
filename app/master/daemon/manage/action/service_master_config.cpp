#include "stdafx.h"
#include "master/master_params.h"
#include "manage/http_client.h"
#include "service_master_config.h"

#define	CMD	"master_config"

bool service_master_config::run(acl::json& json)
{
	master_config_req_t req;
	master_config_res_t res;

	if (deserialize<master_config_req_t>(json, req) == false) {
		res.status = 400;
		res.msg    = "invalid json";
		client_.reply<master_config_res_t>(res.status, CMD, res);
		return false;
	}

	res.status = 200;
	res.msg    = "ok";

	res.data[ACL_VAR_MASTER_STOP_KILL] = acl_var_master_stop_kill;
	res.data[ACL_VAR_MASTER_THROTTLE_TIME] =
		acl::string::parse_int(acl_var_master_throttle_time);
	res.data[ACL_VAR_MASTER_DAEMON_DIR] = acl_var_master_daemon_dir;
	res.data[ACL_VAR_MASTER_SERVICE_DIR] = acl_var_master_service_dir;
	res.data[ACL_VAR_MASTER_FILE_EXTS] = acl_var_master_file_exts;
	res.data[ACL_VAR_MASTER_SERVICE_FILE] = acl_var_master_service_file;
	res.data[ACL_VAR_MASTER_QUEUE_DIR] = acl_var_master_queue_dir;
	res.data[ACL_VAR_MASTER_LOG_FILE] = acl_var_master_log_file;
	res.data[ACL_VAR_MASTER_PID_FILE] = acl_var_master_pid_file;
	res.data[ACL_VAR_MASTER_SCAN_SUBDIR] = acl_var_master_scan_subdir ?
		"yes" : "no";
	res.data[ACL_VAR_MASTER_MANAGE_ADDR] = acl_var_master_manage_addr;
	res.data[ACL_VAR_MASTER_RELOAD_TIMEO] =
		acl::string::parse_int(acl_var_master_reload_timeo);
	res.data[ACL_VAR_MASTER_START_TIMEO] =
		acl::string::parse_int(acl_var_master_start_timeo);

	client_.reply<master_config_res_t>(res.status, CMD, res, false);
	client_.on_finish();

	return true;
}
