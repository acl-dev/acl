
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "lib_acl.h"

#include "global.h"
#include "spool_main.h"

#define	ARG_UNUSE(_x_) (_x_ = _x_)

#define	VAR_LISTENER_BANNER		"listener_banner"
#define	VAR_LISTENER_TMOUT		"listener_timeout"

char *var_listener_banner;
int   var_listener_tmout;

int   var_cfg_max_threads;
int   var_cfg_client_idle_limit;
char *var_cfg_access_allow;

static SPOOL *__spool_handle = NULL;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ VAR_LISTENER_TMOUT, 600, &var_listener_tmout, 0, 0	},

	{ VAR_CFG_MAX_THREADS, 50, &var_cfg_max_threads, 0, 0		},
	{ VAR_CFG_CLIENT_IDLE_LIMIT, 60, &var_cfg_client_idle_limit, 0, 0},

	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ VAR_LISTENER_BANNER, "welcome", &var_listener_banner },

	{ VAR_CFG_ACCESS_ALLOW, "access_allow", &var_cfg_access_allow },

	{ 0, 0, 0 },
};

static void __service(ACL_VSTREAM *stream, char *service, char **argv)
{
	char  myname[] = "__service";
	char  ip[64];

	/*
	 * Sanity check. This service takes no command-line arguments.
	 */
	if (argv[0])
		acl_msg_fatal("%s, %s(%d): unexpected command-line argument: %s, service=%s",
				__FILE__, myname, __LINE__,
				argv[0], service ? service : "null");

	acl_watchdog_pat();

	if (acl_getpeername(ACL_VSTREAM_SOCK(stream), ip, sizeof(ip)) < 0) {
		acl_msg_warn("%s, %s(%d): can't get socket's ip",
				__FILE__, myname, __LINE__);
		acl_vstream_close(stream);
	} else if (!acl_access_permit(ip)) {
		acl_msg_warn("%s, %s(%d): ip(%s) be denied",
				__FILE__, myname, __LINE__, ip);
		(void) acl_vstream_fprintf(stream, "You are not welcome here!\r\n");
		acl_vstream_close(stream);
	} else
		spool_add_worker(__spool_handle, stream);
}

static void __pre_accept(char *name, char **argv)
{
	ARG_UNUSE(name);
	ARG_UNUSE(argv);
}

static void __pre_jail_init(char *name, char **argv)
{
	ARG_UNUSE(name);
	ARG_UNUSE(argv);
}

static void __post_jail_init(char *name, char **argv)
{
	char  myname[] = "__post_jail_init";

	ARG_UNUSE(name);
	ARG_UNUSE(argv);

	acl_access_add(var_cfg_access_allow, ",", ":");

	__spool_handle = spool_create(var_cfg_max_threads, var_cfg_client_idle_limit);
	if (__spool_handle == NULL)
		acl_msg_fatal("%s(%d): spool_create error(%s)",
				myname, __LINE__, strerror(errno));
	if (spool_start(__spool_handle) < 0)
		acl_msg_fatal("%s(%d): spool start error", myname, __LINE__);
}

int main(int argc, char *argv[])
{
	acl_msg_info("%s: starting...", argv[0]);

	acl_listener_server_main(argc, argv, __service,
				ACL_MASTER_SERVER_INT_TABLE, __conf_int_tab,
				ACL_MASTER_SERVER_STR_TABLE, __conf_str_tab,
				ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
				ACL_MASTER_SERVER_PRE_ACCEPT, __pre_accept,
				ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
				0);
	exit (0);
}

