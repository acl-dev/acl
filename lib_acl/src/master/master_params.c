#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "stdlib/acl_mymalloc.h"
#include "master/acl_master_conf.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_loadcfg.h"
#include "stdlib/acl_xinetd_cfg.h"
#include "stdlib/acl_mystring.h"

#endif

#ifdef ACL_UNIX
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

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ ACL_VAR_MASTER_PROC_LIMIT, ACL_DEF_MASTER_PROC_LIMIT, &acl_var_master_proc_limit, 0, 0 },
	{ ACL_VAR_MASTER_THROTTLE_TIME, ACL_DEF_MASTER_THROTTLE_TIME, &acl_var_master_throttle_time, 0, 0 },
	{ ACL_VAR_MASTER_BUF_SIZE, ACL_DEF_MASTER_BUF_SIZE, &acl_var_master_buf_size, 0, 0 },
	{ ACL_VAR_MASTER_RW_TIMEOUT, ACL_DEF_MASTER_RW_TIMEOUT, &acl_var_master_rw_timeout, 0, 0 },
	{ ACL_VAR_MASTER_IN_FLOW_DELAY, ACL_DEF_MASTER_IN_FLOW_DELAY, &acl_var_master_in_flow_delay, 0, 0 },
	{ ACL_VAR_MASTER_DELAY_SEC, ACL_DEF_MASTER_DELAY_SEC, &acl_var_master_delay_sec, 0, 0 },
	{ ACL_VAR_MASTER_DELAY_USEC, ACL_DEF_MASTER_DELAY_USEC, &acl_var_master_delay_usec, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

char *acl_var_master_inet_interfaces;
char *acl_var_master_owner_user;
char *acl_var_master_owner;
char *acl_var_master_owner_group;
char *acl_var_master_daemon_dir;
char *acl_var_master_queue_dir;
char *acl_var_master_service_dir;
char *acl_var_master_log_file;
char *acl_var_master_pid_file;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ ACL_VAR_MASTER_INET_INTERFACES, ACL_DEF_MASTER_INET_INTERFACES, &acl_var_master_inet_interfaces },
	{ ACL_VAR_MASTER_OWNER_USER, ACL_DEF_MASTER_OWNER_USER, &acl_var_master_owner_user },
	{ ACL_VAR_MASTER_OWNER, ACL_DEF_MASTER_OWNER, &acl_var_master_owner },
	{ ACL_VAR_MASTER_OWNER_GROUP, ACL_DEF_MASTER_OWNER_GROUP, &acl_var_master_owner_group },
	{ ACL_VAR_MASTER_DAEMON_DIR, ACL_DEF_MASTER_DAEMON_DIR, &acl_var_master_daemon_dir },
	{ ACL_VAR_MASTER_QUEUE_DIR, ACL_DEF_MASTER_QUEUE_DIR, &acl_var_master_queue_dir },
	{ ACL_VAR_MASTER_SERVICE_DIR, ACL_DEF_MASTER_SERVICE_DIR, &acl_var_master_service_dir },
	{ ACL_VAR_MASTER_LOG_FILE, ACL_DEF_MASTER_LOG_FILE, &acl_var_master_log_file },
	{ ACL_VAR_MASTER_PID_FILE, ACL_DEF_MASTER_PID_FILE, &acl_var_master_pid_file },
	{ 0, 0, 0 },
};

int   acl_var_master_scan_subdir;
int   acl_var_master_limit_privilege;

static ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
	{ ACL_VAR_MASTER_SCAN_SUBDIR, ACL_DEF_MASTER_SCAN_SUBDIR, &acl_var_master_scan_subdir },
	{ ACL_VAR_MASTER_LIMIT_PRIVILEGE, ACL_DEF_MASTER_LIMIT_PRIVILEGE, &acl_var_master_limit_privilege },
	{ 0, 0, 0 },
};
#endif  /* ACL_UNIX */

/*-------------- global static inition and update functions ----------------*/

static void __init_conf_int_vars(ACL_CONFIG_INT_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void __init_conf_int64_vars(ACL_CONFIG_INT64_TABLE cit[])
{
	int   i;

	for (i = 0; cit[i].name != 0; i++)
		*(cit[i].target) = cit[i].defval;
}

static void __init_conf_str_vars(ACL_CONFIG_STR_TABLE cst[])
{
	int   i;

	for (i = 0; cst[i].name != 0; i++) {
		/*
		if (*(cst[i].target) != 0)
			acl_myfree(*(cst[i].target));
		*/
		*(cst[i].target) = acl_mystrdup(cst[i].defval);
	}
}

static void __init_conf_bool_vars(ACL_CONFIG_BOOL_TABLE cbt[])
{
	int   i;

	for (i = 0; cbt[i].name != 0; i++) {
		if (cbt[i].defval == 0)
			*(cbt[i].target) = 0;
		else
			*(cbt[i].target) = 1;
	}
}

static void __update_conf_int_vars(ACL_CONFIG_INT_TABLE cit[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = atoi(value);
	}
}

static void __update_conf_int64_vars(ACL_CONFIG_INT64_TABLE cit[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cit[i].name != 0; i++) {
		if (strcasecmp(cit[i].name, name) == 0)
			*(cit[i].target) = acl_atoi64(value);
	}
}

static void __update_conf_str_vars(ACL_CONFIG_STR_TABLE cst[],
	const char *name, const char *value)
{
	int   i;

	for (i = 0; cst[i].name != 0; i++) {
		if (strcasecmp(cst[i].name, name) == 0) {
			acl_myfree(*(cst[i].target));
			*(cst[i].target) = acl_mystrdup(value);
		}
	}
}

static void __update_conf_bool_vars(ACL_CONFIG_BOOL_TABLE cbt[],
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

/*--------------------------------------------------------------------------*/
#ifdef ACL_UNIX

/* for master process begin */

static ACL_CFG_PARSER *__cfg_parser = NULL;

static void __init_master_vars(void)
{
	__init_conf_int_vars(__conf_int_tab);
	__init_conf_str_vars(__conf_str_tab);
	__init_conf_bool_vars(__conf_bool_tab);
}

static void __update_master_conf_vars(ACL_CFG_PARSER *parser)
{
	int  i, n;
	ACL_CFG_LINE *cfg_line;

	n = acl_cfg_parser_size(parser);

	for (i = 0; i < n; i++) {
		cfg_line = acl_cfg_parser_index(parser, i);
		if (cfg_line == NULL)
			break;
		if (cfg_line->ncount < 2)
			continue;

		if (acl_msg_verbose)
			acl_msg_info("%s = [%s]", cfg_line->value[0],
				cfg_line->value[1]);

		__update_conf_int_vars(__conf_int_tab, cfg_line->value[0],
			cfg_line->value[1]);
	}

	for (i = 0; i < n; i++) {
		cfg_line = acl_cfg_parser_index(parser, i);
		if (cfg_line == NULL)
			break;
		if (cfg_line->ncount < 2)
			continue;

		if (acl_msg_verbose)
			acl_msg_info("%s = [%s]", cfg_line->value[0],
				cfg_line->value[1]);

		__update_conf_str_vars(__conf_str_tab,
			cfg_line->value[0], cfg_line->value[1]);
	}

	for (i = 0; i < n; i++) {
		cfg_line = acl_cfg_parser_index(parser, i);
		if (cfg_line == NULL)
			break;
		if (cfg_line->ncount < 2)
			continue;

		if (acl_msg_verbose)
			acl_msg_info("%s = [%s]", cfg_line->value[0],
				cfg_line->value[1]);

		__update_conf_bool_vars(__conf_bool_tab,
			cfg_line->value[0], cfg_line->value[1]);
	}
}

void acl_master_params_load(const char *pathname)
{
	const char *myname = "acl_master_params_load";

	if (pathname == NULL || *pathname == 0)
		acl_msg_fatal("%s(%d), %s: input error",
			__FILE__, __LINE__, myname);
	if (__cfg_parser != NULL)
		acl_cfg_parser_free(__cfg_parser);

	__init_master_vars();

	__cfg_parser = acl_cfg_parser_load(pathname, " =");

	if (__cfg_parser == NULL)
		acl_msg_fatal("%s(%d), %s: load file(%s) error(%s)",
			__FILE__, __LINE__, myname, pathname, strerror(errno));
	__update_master_conf_vars(__cfg_parser);
}

#endif /* ACL_UNIX */

/* for master process end */

/* below configure operations will be called by the app server */

static ACL_XINETD_CFG_PARSER *__app_cfg = NULL;
static char *__app_conf_file = NULL;

void acl_app_conf_load(const char *pathname)
{
	const char *myname = "acl_app_conf_load";

	if (pathname == NULL || *pathname == 0)
		acl_msg_fatal("%s(%d), %s: input error",
			__FILE__, __LINE__, myname);

	if (__app_cfg != NULL)
		acl_xinetd_cfg_free(__app_cfg);

	__app_cfg = acl_xinetd_cfg_load(pathname);

	if (__app_cfg == NULL)
		acl_msg_fatal("%s(%d), %s: load file(%s) error(%s)",
			__FILE__, __LINE__, myname, pathname, strerror(errno));

	__app_conf_file = acl_mystrdup(pathname);
}

void acl_app_conf_unload(void)
{
	if (__app_cfg) {
		acl_xinetd_cfg_free(__app_cfg);
		__app_cfg = NULL;
	}

	if (__app_conf_file) {
		acl_myfree(__app_conf_file);
		__app_conf_file = NULL;
	}
}

void  acl_get_app_conf_int_table(ACL_CONFIG_INT_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;
	__init_conf_int_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_int_vars(table, name, value);
	}
}

void  acl_get_app_conf_int64_table(ACL_CONFIG_INT64_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;
	__init_conf_int64_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_int64_vars(table, name, value);
	}
}

void  acl_get_app_conf_str_table(ACL_CONFIG_STR_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;
	__init_conf_str_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_str_vars(table, name, value);
	}
}

void  acl_free_app_conf_str_table(ACL_CONFIG_STR_TABLE *table)
{
	int   i;

	if (table == NULL)
		return;

	for (i = 0; table[i].name != 0; i++) {
		if (*(table[i].target) != 0)
			acl_myfree(*(table[i].target));
	}
}

void  acl_get_app_conf_bool_table(ACL_CONFIG_BOOL_TABLE *table)
{
	int   i, n, ret;
	char *name, *value;

	if (table == NULL)
		return;
	__init_conf_bool_vars(table);

	if (__app_cfg == NULL)
		return;

	n = acl_xinetd_cfg_size(__app_cfg);

	for (i = 0; i < n; i++) {
		ret = acl_xinetd_cfg_index(__app_cfg, i, &name, &value);
		if (ret == 0)
			__update_conf_bool_vars(table, name, value);
	}
}
