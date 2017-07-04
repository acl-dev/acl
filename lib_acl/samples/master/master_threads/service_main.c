#include "lib_acl.h"
#include "service_main.h"

/* 配置文件项 */
char *var_cfg_mysql_dbaddr;
char *var_cfg_mysql_dbuser;
char *var_cfg_mysql_dbpass;
char *var_cfg_mysql_dbname;

int   var_cfg_mysql_dbmax;
int   var_cfg_mysql_dbping;
int   var_cfg_mysql_dbtimeout;
int   var_cfg_mysql_auto_commit;

int   var_cfg_debug_mem;

/* configure info */

/* TODO: you can add configure items here */

ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ "debug_mem", 0, &var_cfg_debug_mem },
	{ 0, 0, 0 },
};

ACL_CONFIG_INT_TABLE service_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ "mysql_dbmax", 250, &var_cfg_mysql_dbmax, 0, 0 },
	{ "mysql_dbping", 60, &var_cfg_mysql_dbping, 0, 0 },
	{ "mysql_dbtimeout", 300, &var_cfg_mysql_dbtimeout, 0, 0 },
	{ "mysql_auto_commit", 1, &var_cfg_mysql_auto_commit, 0, 0 },

	{ 0, 0, 0, 0, 0 },
};

ACL_CONFIG_STR_TABLE service_conf_str_tab[] = {

	/* TODO: you can add configure variables of (char *) type here */
	{ "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr },
	{ "mysql_dbname", "dbname", &var_cfg_mysql_dbname },
	{ "mysql_dbuser", "dbuser", &var_cfg_mysql_dbuser },
	{ "mysql_dbpass", "111111", &var_cfg_mysql_dbpass },

	{ 0, 0, 0 },
};

/* 初始化函数 */
void service_init(void *init_ctx acl_unused)
{
	/*
	const char *ptr = getenv("MASTER_ENV");

	if (ptr)
		acl_msg_info("env=%s", ptr);
	else
		acl_msg_info("no env");
	*/
}

void service_exit(void *arg acl_unused)
{
}

/* 协议处理函数入口 */
int service_main(void *run_ctx acl_unused, ACL_VSTREAM *client)
{
	char  buf[256];
	int   ret;

	ACL_VSTREAM_SET_RWTIMO(client, 1200);

	if (var_cfg_debug_mem)
		acl_msg_info("total alloc: %d", acl_mempool_total_allocated());

	do {
		if (isatty(ACL_VSTREAM_SOCK(client))) {
			printf("Please input: ");
			fflush(stdout);
		} else
			acl_msg_info("waiting ......");
		ret = acl_vstream_gets(client, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF)
			return (-1);
		buf[ret] = 0;
		if (isatty(ACL_VSTREAM_SOCK(client)))
			ret = acl_vstream_printf("Your input: %s", buf);
		else {
			acl_msg_info("wakeup now, ret=%d", ret);
			ret = acl_vstream_writen(client, buf, strlen(buf));
		}
		if (ret == ACL_VSTREAM_EOF)
			return (-1);
	} while (1);

	acl_msg_info("over now\r\n");
	return (-1);
}
