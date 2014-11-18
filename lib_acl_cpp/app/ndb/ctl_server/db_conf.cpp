#include "stdafx.h"
#include "db_conf.h"

char* var_cfg_mysql_dbaddr;
char* var_cfg_mysql_dbname;
char* var_cfg_mysql_dbuser;
char* var_cfg_mysql_dbpass;

int   var_cfg_mysql_dbpool_limit;
int   var_cfg_mysql_dbpool_timeout;
int   var_cfg_mysql_dbpool_dbping;

static ACL_CFG_STR_TABLE __conf_str_tab[] =
{
	{ "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr },
	{ "mysql_dbname", "", &var_cfg_mysql_dbname },
	{ "mysql_dbuser", "root", &var_cfg_mysql_dbuser },
	{ "mysql_dbpass", "", &var_cfg_mysql_dbpass },
	{ 0, 0, 0 },
};

static ACL_CFG_INT_TABLE __conf_int_tab[] =
{
	{ "mysql_dbpool_limit", 25, &var_cfg_mysql_dbpool_limit, 0, 0 },
	{ "mysql_dbpool_timeout", 120, &var_cfg_mysql_dbpool_timeout, 0, 0 },
	{ "mysql_dbpool_dbping", 30, &var_cfg_mysql_dbpool_dbping, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static ACL_XINETD_CFG_PARSER* __cfg = NULL;

bool db_conf_load(const char* path)
{
	__cfg = acl_xinetd_cfg_load(path);

	acl_xinetd_params_int_table(__cfg, __conf_int_tab);
	acl_xinetd_params_str_table(__cfg, __conf_str_tab);

	return true;
}

void db_conf_unload(void)
{
	if (__cfg)
	{
		acl_xinetd_cfg_free(__cfg);
		__cfg = NULL;
	}
}
