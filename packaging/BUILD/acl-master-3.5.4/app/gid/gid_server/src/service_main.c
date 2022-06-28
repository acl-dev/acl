#include "lib_acl.h"
#include "lib_protocol.h"

#include "global.h"
#include "gid_oper.h"
#include "cmdline_service.h"
#include "http_service.h"
#include "service_main.h"

/* 配置文件项 */

/* TODO: you can add configure items here */

int   var_cfg_debug_mem;
int   var_cfg_loop_enable;
int   var_cfg_sync_gid;

ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[] = {
	/* TODO: you can add configure variables of (bool) type here */
	{ "debug_mem", 0, &var_cfg_debug_mem },
	{ "loop_enable", 0, &var_cfg_loop_enable },
	{ "sync_gid", 1, &var_cfg_sync_gid },
	{ 0, 0, 0 },
};

int   var_cfg_debug_section;
int   var_cfg_gid_step;
int   var_cfg_gid_test;
int   var_cfg_fh_limit;
int   var_cfg_io_timeout;

ACL_CONFIG_INT_TABLE service_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ "debug_section", 120, &var_cfg_fh_limit, 0, 0 },
	{ "gid_step", 1, &var_cfg_gid_step, 0, 0 },
	{ "gid_test", 50000, &var_cfg_gid_test, 0, 0 },
	{ "fh_limit", 100, &var_cfg_fh_limit, 0, 0 },
	{ "io_timeout", 30, &var_cfg_io_timeout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

char *var_cfg_gid_path;
char *var_cfg_proto_list;
char *var_cfg_proto_default;

ACL_CONFIG_STR_TABLE service_conf_str_tab[] = {
	/* TODO: you can add configure variables of (char *) type here */
	{ "gid_path", "./var", &var_cfg_gid_path },
	{ "proto_list", "http|127.0.0.1:7070|127.0.0.1:7071, cmdline|127.0.0.1:7072|127.0.0.1:7073", &var_cfg_proto_list },
	{ "proto_default", "http", &var_cfg_proto_default },
	{ 0, 0, 0 },
};

typedef struct PROTO {
	char  name[32];
	char  addr[256];
	size_t len;
	int   (*service)(ACL_VSTREAM*);
} PROTO;

static ACL_ARRAY *__proto_map = NULL;
static int (*__default_service)(ACL_VSTREAM*) = NULL;

static void proto_add(const char *data)
{
	int (*service)(ACL_VSTREAM*) = NULL;
	ACL_ARGV *tokens = acl_argv_split(data, "|");
	PROTO *proto;
	int   i;

	if (tokens->argc < 2) {
		acl_msg_error("invalid proto: %s", data);
		acl_argv_free(tokens);
		return;
	}

	if (strcasecmp(tokens->argv[0], "cmdline") == 0)
		service = cmdline_service;
	else if (strcasecmp(tokens->argv[0], "http") == 0)
		service = http_service;
	else {
		acl_msg_error("invalid service name: %s", tokens->argv[0]);
		acl_argv_free(tokens);
		return;
	}

	for (i = 1; i < tokens->argc; i++) {
		proto = (struct PROTO*) acl_mymalloc(sizeof(struct PROTO));
		proto->service = service;
		snprintf(proto->name, sizeof(proto->name), "%s", tokens->argv[0]);
		snprintf(proto->addr, sizeof(proto->addr), "%s", tokens->argv[i]);
		proto->len = strlen(proto->addr);
		acl_array_append(__proto_map, proto);
	}

	acl_argv_free(tokens);
}

static void parse_proto_list(void)
{
	ACL_ARGV *tokens = acl_argv_split(var_cfg_proto_list, ",; \t");
	ACL_ITER  iter;

	__proto_map = acl_array_create(1);
	acl_foreach(iter, tokens) {
		proto_add((const char*) iter.data);
	}

	acl_argv_free(tokens);
}

/* 初始化函数 */
void service_init(void *ctx acl_unused)
{
	if (var_cfg_debug_mem) {
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
	}

	if (strcasecmp(var_cfg_proto_default, "cmdline") == 0)
		__default_service = cmdline_service;
	else
		__default_service = http_service;

	parse_proto_list();
	gid_init(var_cfg_fh_limit, var_cfg_sync_gid, var_cfg_debug_section);
}

void service_exit(void *ctx acl_unused)
{
	if (__proto_map != NULL)
		acl_array_free(__proto_map, acl_myfree_fn);

	gid_finish();
}

int service_main(void *ctx acl_unused, ACL_VSTREAM *client)
{
	int   (*service)(ACL_VSTREAM*) = NULL;
	int   ret;
	char  addr[256];
	ACL_ITER iter;
	PROTO *proto;

	if (acl_getsockname(ACL_VSTREAM_SOCK(client), addr, sizeof(addr)) < 0) {
		acl_msg_error("can't get local addr from fd: %d",
			ACL_VSTREAM_SOCK(client));
		return -1;
	}

	acl_foreach(iter, __proto_map) {
		proto = (PROTO*) iter.data;
		if (acl_strrncasecmp(proto->addr, addr, proto->len) == 0) {
			service = proto->service;
			break;
		}
	}

	if (service == NULL) {
		service = __default_service;
		acl_msg_info("user default service for %s", var_cfg_proto_default);
	}

	if (var_cfg_loop_enable) {
		while (1) {
			if ((ret = service(client)) != 1)
				return (-1);
		}
	} else if (service(client) != 1)
		return (-1);
	else
		return (0);
}
