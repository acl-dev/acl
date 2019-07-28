#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

static int request(ACL_VSTREAM *client, int cmd_num, int out)
{
	const char  cmd1[] = "CMD^new_gid|TAG^default:hello\r\n";
	const char  cmd2[] = "CMD^test_gid\r\n";
	const char *cmd;
	char  buf[256];
	int   ret;

	if (cmd_num == 1)
		cmd = cmd1;
	else
		cmd = cmd2;
	ret = acl_vstream_writen(client, cmd, strlen(cmd));
	if (ret == ACL_VSTREAM_EOF) {
		printf("write cmd to server error\n");
		return (-1);
	}
	ret = acl_vstream_gets(client, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF) {
		printf("gets error\n");
		return (-1);
	}

	if (out)
		printf(">>gets: %s\n", buf);
	return (0);
}

static void run(const char *addr, int n, int keep_alive, int cmd_num)
{
	ACL_VSTREAM *client = NULL;
	int   i, ret, out;
	time_t begin, end;

	time(&begin);

	for (i = 0; i < n; i++) {
		if (client == NULL) {
			client = acl_vstream_connect(addr, ACL_BLOCKING, 0, 0, 0);
			if (client == NULL) {
				printf("connect to %s error %s\n",
					addr, acl_last_serror());
				break;
			}
			acl_tcp_so_linger(ACL_VSTREAM_SOCK(client), 1, 0);
		}
		if ((i > 0 && i % 10000 == 0) || n <= 20)
			out = 1;
		else
			out = 0;
		ret = request(client, cmd_num, out);
		if (ret < 0)
			break;

		if (!keep_alive) {
			acl_vstream_close(client);
			client = NULL;
		}

		if (out) {
			printf(">>>i: %d\n", i);
			ACL_METER_TIME("----------");
		}
	}

	time(&end);
	printf("ok, total: %d, time spent: %ld\n", i, end - begin);
	if (client)
		acl_vstream_close(client);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s server_addr[127.0.0.1:7072] -n loop_count -k -i cmd_number\n", procname);
}

int main(int argc, char *argv[])
{
	char  ch, addr[32];
	int   n = 1, keep_alive = 0, cmd_num = 0;

	addr[0] = 0;
	while ((ch = getopt(argc, argv, "hks:n:i:")) > 0) {
		switch (ch) {
		case 's':
			ACL_SAFE_STRNCPY(addr, optarg, sizeof(addr));
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'k':
			keep_alive = 1;
			break;
		case 'i':
			cmd_num = atoi(optarg);
			break;
		case 'h':
		default:
			usage(argv[0]);
			return (0);
		}
	}

	if (addr[0] == 0 || n <= 0) {
		usage(argv[0]);
		return (0);
	}

	run(addr, n, keep_alive, cmd_num);
	return (0);
}
