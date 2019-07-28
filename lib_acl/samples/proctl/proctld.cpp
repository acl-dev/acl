// proctld.cpp : 定义控制台应用程序的入口点。
//
#pragma comment(lib,"ws2_32")
#include "lib_acl.h"
#include <assert.h>

#ifdef WIN32
#define snprintf _snprintf
#endif

static void init(void)
{
}

static void usage(const char *progname)
{
	printf("usage: %s -h [help] -d [START|STOP|QUIT|LIST|PROBE] -f filepath -a args\r\n",
		progname);
	getchar();
}

int main(int argc, char *argv[])
{
	char  ch, filepath[256], cmd[256];
	char **child_argv = NULL;
	int   child_argc = 0, i;
	ACL_ARGV *argv_tmp;

	filepath[0] = 0;
	cmd[0] = 0;

	init();

	while ((ch = getopt(argc, argv, "d:f:a:h")) > 0) {
		switch(ch) {
		case 'd':
			ACL_SAFE_STRNCPY(cmd, optarg, sizeof(cmd));
			break;
		case 'f':
			ACL_SAFE_STRNCPY(filepath, optarg, sizeof(filepath));
			break;
		case 'a':
			argv_tmp = acl_argv_split(optarg, "|");
			assert(argv_tmp);
			child_argc = argv_tmp->argc;
			child_argv = (char**) acl_mycalloc(child_argc + 1, sizeof(char*));
			for (i = 0; i < child_argc; i++) {
				child_argv[i] = acl_mystrdup(argv_tmp->argv[i]);
			}
			child_argv[i] = NULL;
			
			acl_argv_free(argv_tmp);
			break;
		case 'h':
			usage(argv[0]);
			return (0);
		default:
			usage(argv[0]);
			return (0);
		}
	}

	if (strcasecmp(cmd, "STOP") == 0) {
		if (filepath[0])
			acl_proctl_stop_one(argv[0], filepath, child_argc, child_argv);
		else
			acl_proctl_stop_all(argv[0]);
	} else if (strcasecmp(cmd, "START") == 0) {
		if (filepath[0] == 0) {
			usage(argv[0]);
			return (0);
		}
		acl_proctl_start_one(argv[0], filepath, child_argc, child_argv);
	} else if (strcasecmp(cmd, "QUIT") == 0) {
		acl_proctl_quit(argv[0]);
	} else if (strcasecmp(cmd, "LIST") == 0) {
		acl_proctl_list(argv[0]);
	} else if (strcasecmp(cmd, "PROBE") == 0) {
		if (filepath[0] == 0) {
			usage(argv[0]);
			return (0);
		}
		acl_proctl_probe(argv[0], filepath);
	} else {
		char  buf[MAX_PATH], logfile[MAX_PATH], *ptr;

		acl_proctl_daemon_path(buf, sizeof(buf));
		ptr = strrchr(argv[0], '\\');
		if (ptr == NULL)
			ptr = strrchr(argv[0], '/');

		if (ptr == NULL)
			ptr = argv[0];
		else
			ptr++;

		snprintf(logfile, sizeof(logfile), "%s/%s.log", buf, ptr);
		acl_msg_open(logfile, "daemon");
		acl_debug_init("all:2");

		/* 以服务器模式启动监控进程 */
		acl_proctl_deamon_init(argv[0]);
		acl_proctl_daemon_loop();
	}

	if (child_argv) {
		for (i = 0; child_argv[i] != NULL; i++) {
			acl_myfree(child_argv[i]);
		}
		acl_myfree(child_argv);
	}
	return (0);
}
