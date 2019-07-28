#include "stdafx.h"

static int __loop_count = 0;

static void on_sigint(int sig acl_unused)
{
	printf("loop_count: %d\r\n", __loop_count);
	exit (0);
}

static void run(const char *addr, int can_quit, int need_echo, int inter)
{
	char  buf[4096];
	int   ret, i = 0;
	ACL_VSTREAM *stream = acl_vstream_bind(addr, 0, 0);  /* °ó¶¨ UDP Ì×½Ó¿Ú */

	if (stream == NULL) {
		printf("acl_vstream_bind %s error %s\r\n",
			addr, acl_last_serror());
		return;
	}

	printf("bind udp addr %s ok\r\n", addr);

	while (1) {
		ret = acl_vstream_read(stream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("acl_vstream_read error %s\r\n",
				acl_last_serror());
			break;
		} else
			buf[ret] = 0;

		if (can_quit && strcasecmp(buf, "quit") == 0) {
			printf("Quit\r\n");
			break;
		}

		i++;
		__loop_count = i;

		if (need_echo) {
			ret = acl_vstream_write(stream, buf, ret);
			if (ret == ACL_VSTREAM_EOF) {
				printf("acl_vtream_writen error %s\r\n",
						acl_last_serror());
				break;
			}
		}

		if (i % inter == 0) {
			snprintf(buf, sizeof(buf), "curr: %d", i);
			ACL_METER_TIME(buf);
		}
	}

	printf("local addr: %s, peer addr: %s, total: %d\r\n",
		ACL_VSTREAM_LOCAL(stream), ACL_VSTREAM_PEER(stream), i);

	acl_vstream_close(stream);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-c if can quit from client quit cmd [default: false]\r\n"
		"	-i print_per_loop [default: 1000]\r\n"
		"	-e if_echo_to_client [default: false]\r\n"
		"	-s server_addr\r\n", procname);
}

int main(int argc, char *argv[])
{
	char  addr[64];
	int   ch, can_quit = 0, need_echo = 0, inter = 1000;

	acl_lib_init();
	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "127.0.0.1:8888");

	while ((ch = getopt(argc, argv, "hs:cei:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'c':
			can_quit = 1;
			break;
		case 'e':
			need_echo = 1;
			break;
		case 'i':
			inter = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (addr[0] == 0) {
		usage(argv[0]);
		return 1;
	}

	acl_msg_stdout_enable(1);
	signal(SIGINT, on_sigint);
	run(addr, can_quit, need_echo, inter);

	acl_lib_end();

	return 0;
}
