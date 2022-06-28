#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#endif  /* ACL_PREPARE_COMPILE */

#ifdef ACL_WINDOWS
#include "stdlib/acl_stdlib.h"
#include "net/acl_net.h"
#include <stdarg.h>
#include <windows.h>
#include "proctl_internal.h"

/* 连接某个服务进程的监听接口，发送停止消息 */
static void proctl_monitor_stop_service(ACL_VSTREAM *client,
	const char *filepath, const char *args)
{
	const char *myname = "proctl_monitor_stop_service";
	ACL_VSTREAM *stream;
	char  addr[256], ebuf[256], buf[1024], logfile[MAX_PATH];
	int   n;

	get_lock_file2(filepath, logfile, sizeof(logfile));

	if (get_addr_from_file(logfile, addr, sizeof(addr)) < 0) {
		acl_vstream_fprintf(client, "-ERR|get addr error from %s\r\n", filepath);
		acl_msg_error("%s(%d): get addr for filepath(%s) error",
			myname, __LINE__, filepath);
		return;
	}

	stream = acl_vstream_connect(addr, ACL_BLOCKING, 10, 10, 1024);
	if (stream == NULL) {
		acl_vstream_fprintf(client, "-ERR|connect addr=%s error, file=%s\r\n",
			addr, filepath);
		acl_msg_error("%s(%d): connect addr(%s) error(%s)",
			myname, __LINE__, addr, acl_last_strerror(ebuf, sizeof(ebuf)));
		return;
	}

	if (args && *args)
		n = acl_vstream_fprintf(stream, "%s|-d|STOP|-f|%s|-a|%s\r\n",
				filepath, filepath, args);
	else
		n = acl_vstream_fprintf(stream, "%s|-d|STOP|-f|%s\r\n",
				filepath, filepath);

	buf[0] = 0;

	if (n == ACL_VSTREAM_EOF) {
		acl_vstream_fprintf(client, "-ERR|write to addr=%s error, file=%s\r\n",
			addr, filepath);
		acl_msg_error("%s(%d): fprintf to acl_master error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	} else if (acl_vstream_gets_nonl(stream, buf, sizeof(buf)) == ACL_VSTREAM_EOF) {
		acl_vstream_fprintf(client, "-ERR|filepath(%s), not get respond\r\n", filepath);
		acl_msg_error("%s(%d): not get respond, filepath(%s)",
			myname, __LINE__, filepath);
	} else if (strncasecmp(buf, "+OK", 3) != 0) {
		acl_vstream_fprintf(client, "-ERR|filepath(%s), child respond(%s)\r\n",
			filepath, buf);
		acl_msg_error("%s(%d): child respond error(%s), filepath(%s)",
			myname, __LINE__, buf, filepath);
	} else {
		acl_vstream_fprintf(client, "+OK|stopped %s\r\n", filepath);
		acl_msg_info("%s(%d): stop child(%s) ok", myname, __LINE__, filepath);
	}

	acl_vstream_close(stream);
}

/* 停止所有服务进程 */
static void proctl_monitor_stop_all_service(ACL_VSTREAM *client)
{
	ACL_ARGV *service_argv;
	int   i;

	service_argv = proctl_serivce_get_all();
	if (service_argv == NULL) {
		acl_vstream_fprintf(client, "+OK|no service running yet!\r\n");
		return;
	}

	for (i = 0; i < service_argv->argc; i++) {
		proctl_monitor_stop_service(client, service_argv->argv[i], NULL);
	}

	proctl_service_free_all(service_argv);
}

/* 停止某个服务进程 */
static int proctl_monitor_cmd_stop(ACL_VSTREAM *client,
	const char *filepath, const char *args)
{
	const char *myname = "proctl_monitor_cmd_stop";

	if (filepath == NULL || *filepath == 0) {
		acl_vstream_fprintf(client, "-ERR|filepath null\r\n");
		acl_msg_error("%s(%d): no filepath", myname, __LINE__);
		return (-1);
	}

	if (strcasecmp(filepath, "all") == 0) {
		acl_msg_info("begin to stop file(%s)", filepath);
		proctl_monitor_stop_all_service(client);
	} else if (!proctl_service_exist(filepath)) {
		acl_msg_error("%s(%d): filepath(%s) not running now",
			myname, __LINE__, filepath);
		acl_vstream_fprintf(client, "-ERR|filepath(%s) not running\r\n",
			filepath);
		return (-1);
	}

	acl_msg_info("%s(%d): begin to stop file(%s)", myname, __LINE__, filepath);
	proctl_monitor_stop_service(client, filepath, args);
	acl_msg_info("%s(%d): stop (%s) end", myname, __LINE__, filepath);
	return (0);
}

/* 通知主线程，启动一个服务 */
static int proctl_monitor_cmd_start(ACL_VSTREAM *client,
	const char *filepath, const char *args)
{
	const char *myname = "proctl_monitor_cmd_start";
	ACL_VSTRING *cmdline;
	PROCTL_SERVICE *service;
	PROCTL_MSG *msg;

	if (filepath[0] == 0) {
		acl_vstream_fprintf(client, "-ERR|filepath null\r\n");
		acl_msg_error("%s(%d): no filepath", myname, __LINE__);
		return (-1);
	}

	if (proctl_service_exist(filepath)) {
		acl_msg_error("%s(%d): child(%s) maybe be running!",
			myname, __LINE__, filepath);
		acl_vstream_fprintf(client, "-ERR|child(%s) maybe be running!\r\n",
			filepath);
		return (-1);
	}

	cmdline = acl_vstring_alloc(256);
	acl_vstring_strcpy(cmdline, "\"");
	acl_vstring_strcat(cmdline, filepath);
	acl_vstring_strcat(cmdline, "\"");
	if (args && *args) {
		acl_vstring_strcat(cmdline, " ");
		acl_vstring_strcat(cmdline, args);
	}
	service = proctl_service_alloc(filepath, cmdline);
	msg = proctl_msg_new(PROCTL_MSG_START);
	msg->service = service;
	proctl_msg_push(msg);
	return (0);
}

/* 停止所有的服务，并退出整个控制进程 */
static void proctl_monitor_cmd_quit(ACL_VSTREAM *client)
{
	const char *myname = "proctl_monitor_cmd_quit";

	acl_msg_info("%s(%d): begin to quit ...", myname, __LINE__);
	acl_msg_info("%s(%d): begin to stop all ...", myname, __LINE__);
	proctl_monitor_stop_all_service(client);
	acl_msg_info("%s(%d): stop all end", myname, __LINE__);
	acl_msg_info("%s(%d): quit ok", myname, __LINE__);
	acl_vstream_fprintf(client, "+OK|stopped all children, and quit now\r\n");
	acl_vstream_close(client);
	exit(0);
}

static int proctl_monitor_cmd_list(ACL_VSTREAM *client)
{
	ACL_ARGV *service_argv;
	int   i, ret = 0;

	service_argv = proctl_serivce_get_all();
	if (service_argv == NULL) {
		acl_vstream_fprintf(client, "+OK|no service running yet!\r\n");
		return (0);
	}

	for (i = 0; i < service_argv->argc; i++) {
		ret = acl_vstream_fprintf(client, "+OK|service=%s running\r\n",
			service_argv->argv[i]);
		if (ret == ACL_VSTREAM_EOF)
			break;
	}

	if (ret != ACL_VSTREAM_EOF)
		acl_vstream_fprintf(client, "+OK|total service is %d\r\n",
			service_argv->argc);
	proctl_service_free_all(service_argv);

	return (0);
}

static int proctl_monitor_cmd_probe(ACL_VSTREAM *client, const char *filepath)
{
	if (filepath == NULL || *filepath == 0) {
		acl_vstream_fprintf(client, "-ERR|filepath is null\r\n");
		return (-1);
	}

	if (proctl_service_exist(filepath))
		acl_vstream_fprintf(client, "+OK|service: %s is running\r\n", filepath);
	else
		acl_vstream_fprintf(client, "+OK|service: %s is not running\r\n", filepath);

	return (0);
}

static void usage(ACL_VSTREAM *client)
{
	acl_vstream_fprintf(client, "usage: progname|-h[help]"
		"|-d|{action}[START|STOP|QUIT|LIST|PROBE]|-f|filepath|-a|args\r\n");
	acl_msg_info("usage: progname|-h[help]|-d|{action}[START|STOP|QUIT|LIST|PROBE]"
		"|-f|filepath|-a|args");
}

/* 控制进程的监听线程处理命令总入口 */
static int proctl_monitor_main(ACL_VSTREAM *client, int argc, char *argv[])
{
	const char *myname = "proctl_monitor_main";
	char   i, cmd[256], filepath[MAX_PATH], args[512];

	cmd[0] = 0;
	filepath[0] = 0;
	args[0] = 0;

	if (acl_do_debug(ACL_DEBUG_PROCTL, 2)) {
		int   i;
		for (i = 0; i < argc; i++)
			acl_msg_info("%s(%d): argv[%d]=%s", myname, __LINE__, i, argv[i]);
	}

	/* "d:f:a:h" */
	for (i = 0; i < argc; i++) {
		if (argv[i][0] != '-')
			continue;
		switch(argv[i][1]) {
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
		proctl_monitor_cmd_stop(client, filepath, args);
	} else if (strcasecmp(cmd, "START") == 0) {
		proctl_monitor_cmd_start(client, filepath, args);
	} else if (strcasecmp(cmd, "QUIT") == 0) {
		proctl_monitor_cmd_quit(client);
	} else if (strcasecmp(cmd, "LIST") == 0) {
		proctl_monitor_cmd_list(client);
	} else if (strcasecmp(cmd, "PROBE") == 0) {
		proctl_monitor_cmd_probe(client, filepath);
	} else {
		usage(client);
		acl_msg_warn("%s(%d): unknown cmd(%s)", myname, __LINE__, cmd);
	}

	return (0);
}

static void proctl_monitor_loop(ACL_VSTREAM *sstream)
{
	const char *myname = "proctl_monitor_loop";
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
			proctl_monitor_main(client, cmd_argv->argc, cmd_argv->argv);
		else
			usage(client);
		acl_vstream_close(client);
	}
}

void *proctl_monitor_thread(void *arg)
{
	const char *myname = "proctl_monitor_thread";
	ACL_VSTREAM *sstream;
	char  ebuf[256];

	sstream = local_listen();
	if (sstream == NULL)
		acl_msg_fatal("%s(%d): local_listen return NULL, error(%s)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
	proctl_monitor_loop(sstream);

	/* unreached */
	return (NULL);
}

#endif /* ACL_WINDOWS */

