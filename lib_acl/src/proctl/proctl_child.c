#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#endif  /* ACL_PREPARE_COMPILE */

#ifdef ACL_WINDOWS

#include "stdlib/acl_stdlib.h"
#include "net/acl_net.h"
#include <windows.h>
#include "proctl_internal.h"

static void (*__onexit_fn)(void*) = NULL;
static void *__onexit_arg = NULL;

static void usage(ACL_VSTREAM *client)
{
	acl_vstream_fprintf(client,
		"usage: %s -h[help]|-d|STOP|-f|filepath|[-a|args]\r\n", var_progname);
	acl_msg_info("usage: %s -h[help]|-d|STOP|-f|filepath|[-a|args]\r\n",
		var_progname);
}

static void proctl_child_cmd_stop(ACL_VSTREAM *client, const char *filepath)
{
	const char *myname = "proctl_child_cmd_stop";

	if (filepath[0] == 0) {
		acl_vstream_fprintf(client, "-ERR|filepath null\r\n");
		acl_msg_error("%s(%d): no filepath", myname, __LINE__);
		return;
	}

	if (strcasecmp(filepath, var_progname) != 0) {
		acl_vstream_fprintf(client, "-ERR|filepath(%s)!=progname(%s)\r\n",
			filepath, var_progname);
		acl_msg_error("%s(%d): filepath=(%s) invalid, progname=(%s)",
			myname, __LINE__, filepath, var_progname);
		return;
	}

	acl_vstream_fprintf(client, "+OK|%s is stopping\r\n", filepath);
	acl_vstream_close(client);  /* 显式关闭是为了将数据传输完 */

	if (__onexit_fn) {
		acl_msg_info("%s(%d): call onexit_fn before exit, filepath(%s) ",
			myname, __LINE__, filepath);
		__onexit_fn(__onexit_arg);
	}

	acl_msg_info("%s(%d): filepath(%s) exit now", myname, __LINE__, filepath);
	exit(0);
}

static int proctl_child_main(ACL_VSTREAM *client, int argc, char *argv[])
{
	const char *myname = "proctl_child_main";
	char  cmd[256], filepath[256], args[256];
	int   i;

	cmd[0] = 0;
	filepath[0] = 0;
	args[0] = 0;

	/* "d:f:a:h" */
	for (i = 0; i < argc; i++) {
		if (argv[i][0] != '-')
			continue;

		switch (argv[i][1]) {
		case 'h':
			usage(client);
			return (0);
		case 'd':
			if (argv[++i] == NULL) {
				usage(client);
				return (0);
			}
			ACL_SAFE_STRNCPY(cmd, argv[i], sizeof(cmd));
			break;
		case 'f':
			if (argv[++i] == NULL) {
				usage(client);
				return (0);
			}
			ACL_SAFE_STRNCPY(filepath, argv[i], sizeof(filepath));
			break;
		case 'a':
			if (argv[++i] == NULL) {
				usage(client);
				return (0);
			}
			ACL_SAFE_STRNCPY(args, argv[i], sizeof(args));
			break;
		default:
			break;
		}
	}

	if (strcasecmp(cmd, "STOP") == 0) {
		proctl_child_cmd_stop(client, filepath);
	} else {
		usage(client);
		acl_msg_warn("%s(%d): unknown cmd(%s)", myname, __LINE__, cmd);
	}

	return (0);
}

static void proctl_child_loop(ACL_VSTREAM *sstream)
{
	const char *myname = "proctl_child_loop";
	ACL_VSTREAM *client;
	int   n;
	char  buf[1024];
	ACL_ARGV *cmd_argv;

	while (1) {
		client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL)
			continue;

		n = acl_vstream_gets_nonl(client, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			continue;

		acl_debug(ACL_DEBUG_PROCTL, 2) ("%s(%d): get buf(%s)",
			myname, __LINE__, buf);
		cmd_argv = acl_argv_split(buf, "|");
		if (cmd_argv)
			proctl_child_main(client, cmd_argv->argc, cmd_argv->argv);
		else
			acl_msg_error("%s(%d): buf(%s) invalid", myname, __LINE__, buf);
		acl_vstream_close(client);
	}
}

void proctl_child_atexit(void (*onexit_fn)(void*), void *arg)
{
	__onexit_fn = onexit_fn;
	__onexit_arg = arg;
}

/* 服务子进程单独处理消息的线程 */
void *proctl_child_thread(void *arg)
{
	const char *myname = "ptoctl_child_thread";
	ACL_VSTREAM *sstream;
	char  ebuf[256];

	sstream = local_listen();
	if (sstream == NULL) {
		acl_msg_error("%s(%d): local_listen error(%s), maybe there's another"
			" instance is running", myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)));
		/* XXX: 此处必须以0的方式退出程序运行，以防止被父进程频繁启动 */
		exit(0);
	}

	proctl_child_loop(sstream);

	return (NULL);
}

#endif /* ACL_WINDOWS */

