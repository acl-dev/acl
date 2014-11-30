#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>

#include "service_main.h"

/* configure info */

/* TODO: you can add configure items here */

static int   var_cfg_debug_mem;

ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[] = {
        /* TODO: you can add configure variables of int type here */
        { "debug_mem", 0, &var_cfg_debug_mem },

        { 0, 0, 0 },
};

static int   var_cfg_io_idle_limit;
static int   var_cfg_debug_mem;

ACL_CONFIG_INT_TABLE service_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "io_idle_limit", 60, &var_cfg_io_idle_limit, 0, 0 },

	{ 0, 0, 0, 0, 0 },
};

ACL_CONFIG_STR_TABLE service_conf_str_tab[] = {

	/* TODO: you can add configure variables of (char *) type here */
	/* example: { "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr }, */

	{ 0, 0, 0 },
};

void service_init(void *init_ctx acl_unused)
{
}

void service_exit(void *exist_ctx acl_unused)
{
}

static int read_callback(ACL_ASTREAM *client, void *ctx acl_unused,
	char *data, int dlen)
{
	acl_aio_writen(client, data, dlen);
	return (0);
}

void service_main(ACL_ASTREAM *client, void *run_ctx acl_unused)
{
	acl_aio_ctl(client,
		ACL_AIO_CTL_READ_HOOK_ADD, read_callback, run_ctx,
		ACL_AIO_CTL_TIMEOUT, 300,
		ACL_AIO_CTL_END);
	acl_aio_gets(client);
}
