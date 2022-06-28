#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

char *var_cfg_single_banner;
int   var_cfg_single_timeout;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ "single_timeout", 60, &var_cfg_single_timeout, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ "single_banner", "hello, welcome!", &var_cfg_single_banner },
	{ 0, 0, 0 },
};

static void __service(void *ctx acl_unused, ACL_VSTREAM *stream)
{
	const char *myname = "__service";
	char  buf[4096];
	int   n, ret;

	acl_msg_info("%s(%d)->%s: rw_timeout = %d",
		__FILE__, __LINE__, myname, stream->rw_timeout);

	acl_msg_info("total alloc: %d", acl_mempool_total_allocated());
	do {
		acl_watchdog_pat();

		if (isatty(ACL_VSTREAM_SOCK(stream)))
			acl_vstream_printf("Please input: ");

		n = acl_vstream_gets(stream, buf, sizeof(buf) - 1);
		if (n == ACL_VSTREAM_EOF) {
			acl_msg_info("%s(%d)->%s: read over",
				__FILE__, __LINE__, myname);
			break;
		}

		if (isatty(ACL_VSTREAM_SOCK(stream))) {
			acl_vstream_printf("Your input: ");
			ret = acl_vstream_writen(ACL_VSTREAM_OUT, buf, n);
		} else
			ret = acl_vstream_writen(stream, buf, n);
		if (ret != n) {
			acl_msg_info("%s(%d)->%s: write error = %s",
				__FILE__, __LINE__, myname, strerror(errno));
			break;
		}
		if (strncasecmp(buf, "quit", 4) == 0)
			break;
	} while (1);
}

static void __pre_jail_init(char *name acl_unused, char **argv acl_unused)
{
}

static void __post_jail_init(char *name acl_unused, char **argv acl_unused)
{
}

static void service_exit(char *service acl_unused, char **argv acl_unused)
{
}

int main(int argc, char *argv[])
{
	acl_single_server_main(argc, argv, __service,
		ACL_MASTER_SERVER_INT_TABLE, __conf_int_tab,
		ACL_MASTER_SERVER_STR_TABLE, __conf_str_tab,
		ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
		ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
		ACL_MASTER_SERVER_EXIT, service_exit,
		0);
	exit (0);
}

