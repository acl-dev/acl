#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "lib_acl.h"
#include "hook_close.h"

static void check_connection(const char *server_addr)
{
	ACL_VSTREAM *conn = acl_vstream_connect(server_addr, ACL_BLOCKING,
		0, 0, 8192);

	if (conn == NULL) {
		printf("connect %s error %s\r\n", server_addr, acl_last_serror());
		return;
	}

	acl_vstream_close(conn);
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -s server_addr[default: 127.0.0.1:6379]\r\n"
		" -p monitor_ports[default: 6379]\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;
	char addr[64], ports[256];
	ACL_ARGV *tokens;
	ACL_ITER iter;

	acl_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "127.0.0.1:6379");
	snprintf(ports, sizeof(ports), "6379");

	while ((ch = getopt(argc, argv, "hs:p:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'p':
			snprintf(ports, sizeof(ports), "%s", optarg);
			break;
		default:
			break;
		}
	}

	tokens = acl_argv_split(ports, ",; \t");

	acl_foreach(iter, tokens) {
		const char *ptr = (const char *) iter.data;
		int port = atoi(ptr);

		monitor_port_add(port); // add one port to be monitored
	}

	acl_argv_free(tokens);

	check_connection(addr);

	return (0);
}
