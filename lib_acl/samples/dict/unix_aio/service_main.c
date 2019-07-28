
#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib_protocol.h"
#include "dict_pool.h"

#include "app_main.h"
#include "http_service.h"
#include "service_main.h"

/* configure pool */

/* TODO: you can add configure items here */

int   var_cfg_debug_mem;
char *var_cfg_dbpath;
char *var_cfg_dbnames;
int   var_cfg_rw_timeout;
int   var_cfg_use_bdb;

ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[] = {
        /* TODO: you can add configure variables of int type here */
        { "debug_mem", 0, &var_cfg_debug_mem },
	{ "use_bdb", 1, &var_cfg_use_bdb },

        { 0, 0, 0 },
};

ACL_CONFIG_INT_TABLE service_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "rw_timeout", 1200, &var_cfg_rw_timeout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

ACL_CONFIG_STR_TABLE service_conf_str_tab[] = {

	/* TODO: you can add configure variables of (char *) type here */
	/* example: { "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr }, */

	{ "db_names", "default:8", &var_cfg_dbnames },
	{ "db_path", "dbpath", &var_cfg_dbpath },
	{ 0, 0, 0 },
};

void service_init(void *init_ctx)
{
	acl_init();
	http_service_init(init_ctx);
}

void service_exit(void *exit_ctx)
{
	http_service_exit(exit_ctx);
}

int service_main(ACL_ASTREAM *stream, void *run_ctx)
{
	if (var_cfg_debug_mem)
		acl_msg_info("total alloc: %d", acl_mempool_total_allocated());

	http_service_main(stream, run_ctx);
	return (0);
}
