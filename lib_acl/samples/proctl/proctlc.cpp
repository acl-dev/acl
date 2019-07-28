// proctlc.cpp : 定义控制台应用程序的入口点。
//
#pragma comment(lib,"ws2_32")
#include "lib_acl.h"

static void onexit_fn(void *arg acl_unused)
{
}

int main(int argc, char *argv[])
{
	int   i;

	acl_socket_init();
	acl_msg_open("debug.txt", "proctlc");
	acl_msg_info(">>> in child progname(%s), argc=%d\r\n", argv[0], argc);
	if (argc > 1)
		acl_msg_info(">>> in child progname, argv[1]=(%s)\r\n", argv[1]);
	acl_proctl_child(argv[0], onexit_fn, NULL);

	for (i = 0; i < argc; i++) {
		acl_msg_info(">>>argv[%d]:%s\r\n", i, argv[i]);
	}

	i = 0;
	while (1) {
		acl_msg_info("i = %d\r\n", i++);
		if (i == 5)
			sleep(1);
		else
			sleep(1);
	}
	return (-1);  // 返回 -1 是为了让父进程继续启动
}

