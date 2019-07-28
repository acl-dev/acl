#include "stdafx.h"
#include <grp.h>
#include "master_params.h"

char *acl_var_master_conf_dir; /* must be set value in app */

int   acl_var_master_proc_limit;
uid_t acl_var_master_owner_uid;
gid_t acl_var_master_owner_gid;
int   acl_var_master_throttle_time;
int   acl_var_master_buf_size;
int   acl_var_master_rw_timeout;
pid_t acl_var_master_pid;
int   acl_var_master_in_flow_delay;
int   acl_var_master_delay_sec;
int   acl_var_master_delay_usec;
int   acl_var_master_reload_timeo;
int   acl_var_master_start_timeo;

static ACL_CONFIG_INT_TABLE int_tab[] = {
	{ ACL_VAR_MASTER_PROC_LIMIT, ACL_DEF_MASTER_PROC_LIMIT,
		&acl_var_master_proc_limit, 0, 0 },
	{ ACL_VAR_MASTER_THROTTLE_TIME, ACL_DEF_MASTER_THROTTLE_TIME,
		&acl_var_master_throttle_time, 0, 0 },
	{ ACL_VAR_MASTER_BUF_SIZE, ACL_DEF_MASTER_BUF_SIZE,
		&acl_var_master_buf_size, 0, 0 },
	{ ACL_VAR_MASTER_RW_TIMEOUT, ACL_DEF_MASTER_RW_TIMEOUT,
		&acl_var_master_rw_timeout, 0, 0 },
	{ ACL_VAR_MASTER_IN_FLOW_DELAY, ACL_DEF_MASTER_IN_FLOW_DELAY,
		&acl_var_master_in_flow_delay, 0, 0 },
	{ ACL_VAR_MASTER_DELAY_SEC, ACL_DEF_MASTER_DELAY_SEC,
		&acl_var_master_delay_sec, 0, 0 },
	{ ACL_VAR_MASTER_DELAY_USEC, ACL_DEF_MASTER_DELAY_USEC,
		&acl_var_master_delay_usec, 0, 0 },
	{ ACL_VAR_MASTER_RELOAD_TIMEO, ACL_DEF_MASTER_RELOAD_TIMEO,
		&acl_var_master_reload_timeo, 0, 0 },
	{ ACL_VAR_MASTER_START_TIMEO, ACL_DEF_MASTER_START_TIMEO,
		&acl_var_master_start_timeo, 0, 0 },

	{ 0, 0, 0, 0, 0 },
};

char *acl_var_master_inet_interfaces;
char *acl_var_master_owner_group;
char *acl_var_master_daemon_dir;
char *acl_var_master_queue_dir;
char *acl_var_master_service_dir;
char *acl_var_master_log_file;
char *acl_var_master_pid_file;
char *acl_var_master_manage_addr;
char *acl_var_master_stop_kill;
char *acl_var_master_file_exts;
char *acl_var_master_service_file;

static ACL_CONFIG_STR_TABLE str_tab[] = {
	{ ACL_VAR_MASTER_INET_INTERFACES, ACL_DEF_MASTER_INET_INTERFACES,
		&acl_var_master_inet_interfaces },
	{ ACL_VAR_MASTER_OWNER_GROUP, ACL_DEF_MASTER_OWNER_GROUP,
		&acl_var_master_owner_group },
	{ ACL_VAR_MASTER_DAEMON_DIR, ACL_DEF_MASTER_DAEMON_DIR,
		&acl_var_master_daemon_dir },
	{ ACL_VAR_MASTER_QUEUE_DIR, ACL_DEF_MASTER_QUEUE_DIR,
		&acl_var_master_queue_dir },
	{ ACL_VAR_MASTER_SERVICE_DIR, ACL_DEF_MASTER_SERVICE_DIR,
		&acl_var_master_service_dir },
	{ ACL_VAR_MASTER_LOG_FILE, ACL_DEF_MASTER_LOG_FILE,
		&acl_var_master_log_file },
	{ ACL_VAR_MASTER_PID_FILE, ACL_DEF_MASTER_PID_FILE,
		&acl_var_master_pid_file },
	{ ACL_VAR_MASTER_MANAGE_ADDR, ACL_DEF_MASTER_MANAGE_ADDR,
		&acl_var_master_manage_addr },
	{ ACL_VAR_MASTER_STOP_KILL, ACL_DEF_MASTER_STOP_KILL,
		&acl_var_master_stop_kill },
	{ ACL_VAR_MASTER_FILE_EXTS, ACL_DEF_MASTER_FILE_EXTS,
		&acl_var_master_file_exts },
	{ ACL_VAR_MASTER_SERVICE_FILE, ACL_DEF_MASTER_SERVICE_FILE,
		&acl_var_master_service_file },

	{ 0, 0, 0 },
};

int   acl_var_master_scan_subdir;
int   acl_var_master_limit_privilege;

static ACL_CONFIG_BOOL_TABLE bool_tab[] = {
	{ ACL_VAR_MASTER_SCAN_SUBDIR, ACL_DEF_MASTER_SCAN_SUBDIR,
		&acl_var_master_scan_subdir },
	{ ACL_VAR_MASTER_LIMIT_PRIVILEGE, ACL_DEF_MASTER_LIMIT_PRIVILEGE,
		&acl_var_master_limit_privilege },

	{ 0, 0, 0 },
};

/*-------------- global static inition and update functions ----------------*/

static void init_conf_int_vars(ACL_CONFIG_INT_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void init_conf_str_vars(ACL_CONFIG_STR_TABLE cst[])
{
	int   i;
	static int first_call = 1;

	for (i = 0; cst[i].name != 0; i++) {
		if (first_call)
			first_call = 0;
		else if (*(cst[i].target) != 0)
			acl_myfree(*(cst[i].target));
		*(cst[i].target) = acl_mystrdup(cst[i].defval);
	}
}

static void init_conf_bool_vars(ACL_CONFIG_BOOL_TABLE cbt[])
{
	int   i;

	for (i = 0; cbt[i].name != 0; i++) {
		if (cbt[i].defval == 0)
			*(cbt[i].target) = 0;
		else
			*(cbt[i].target) = 1;
	}
}

static void update_int_vars(ACL_CONFIG_INT_TABLE cit[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = atoi(value);
	}
}

static void update_str_vars(ACL_CONFIG_STR_TABLE cst[],
	const char *name, const ACL_CFG_LINE *item)
{
	int   i;
	ACL_VSTRING *buf;

	if (item->ncount <= 1)
		return;

	buf = acl_vstring_alloc(128);

	for (i = 1; i < item->ncount; i++)
		acl_vstring_strcat(buf, item->value[i]);

	for (i = 0; cst[i].name != 0; i++) {
		if (strcasecmp(cst[i].name, name) == 0) {
			acl_myfree(*(cst[i].target));
			*(cst[i].target) = acl_vstring_export(buf);
		}
	}
}

static void update_bool_vars(ACL_CONFIG_BOOL_TABLE cbt[],
	const char *name, const char *value)
{
	int   i, n;

	for (i = 0; cbt[i].name != 0; i++) {
		if (strcasecmp(cbt[i].name, name) == 0) {
			n = atoi(value);
			if (n != 0)
				*(cbt[i].target) = 1;
			else
				*(cbt[i].target) = 0;
		}
	}
}

static ACL_CFG_PARSER *cfg_parser = NULL;

static void init_master_vars(void)
{
	init_conf_int_vars(int_tab);
	init_conf_str_vars(str_tab);
	init_conf_bool_vars(bool_tab);
}

static void update_master_conf_vars(ACL_CFG_PARSER *parser)
{
	int  i, n = acl_cfg_parser_size(parser);
	ACL_CFG_LINE *item;

	for (i = 0; i < n; i++) {
		item = acl_cfg_parser_index(parser, i);
		if (item == NULL)
			break;
		if (item->ncount < 2)
			continue;

		update_int_vars(int_tab, item->value[0], item->value[1]);
	}

	for (i = 0; i < n; i++) {
		item = acl_cfg_parser_index(parser, i);
		if (item == NULL)
			break;
		if (item->ncount < 2)
			continue;

		update_str_vars(str_tab, item->value[0], item);
	}

	for (i = 0; i < n; i++) {
		item = acl_cfg_parser_index(parser, i);
		if (item == NULL)
			break;
		if (item->ncount < 2)
			continue;

		update_bool_vars(bool_tab, item->value[0], item->value[1]);
	}
}

void acl_master_params_load(const char *pathname)
{
	const char *myname = "acl_master_params_load";

	if (pathname == NULL || *pathname == 0)
		acl_msg_fatal("%s(%d), %s: input error",
			__FILE__, __LINE__, myname);
	if (cfg_parser != NULL)
		acl_cfg_parser_free(cfg_parser);

	init_master_vars();

	cfg_parser = acl_cfg_parser_load(pathname, "=\t ");
	if (cfg_parser == NULL) {
		acl_msg_error("%s(%d), %s: load file(%s) error(%s)",
			__FILE__, __LINE__, myname, pathname, strerror(errno));
		return;
	}

	update_master_conf_vars(cfg_parser);
}
