#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <string.h>
#include <getopt.h>

#include "service_main.h"

static void client_thread(int event_type acl_unused, void* arg)
{
	ACL_WORKER_ATTR* attr = (ACL_WORKER_ATTR*) arg;
	ACL_VSTREAM *client = (ACL_VSTREAM*) attr->run_data;
	time_t last, cost;
	
	while (1) {
		last = time(NULL);
		if (service_main(NULL, client) < 0)
			break;
		cost = time(NULL) - last;
		if (cost >= 1)
			printf("%s: >>> time cost = %ld\r\n", __FILE__, cost);
	}
	acl_vstream_close(client);
}

static void usage(const char *progname)
{
	printf("usage: %s -[help]\r\n"
			"-s server_addr\r\n"
			"-f regnum\r\n", progname);
	getchar();
}

int main(int argc, char *argv[])
{
	ACL_VSTREAM *sstream, *client;
	ACL_WORK_QUEUE* wq = acl_workq_create(100, 60, NULL, NULL);
	char  ch, *addr = "127.0.0.1:8889";
	char *regnum = "regnum";

	while ((ch = getopt(argc, argv, "hs:f:")) > 0) {
		switch (ch) {
		default:
		case 'h':
			usage(argv[0]);
			exit (0);
		case 's':
			addr = acl_mystrdup(optarg);
			break;
		case 'f':
			regnum = optarg;
			break;
		}
	}

	var_cfg_cache_file = acl_mystrdup(regnum);
	acl_socket_init();
	service_init(NULL);

	sstream = acl_vstream_listen(addr, 32);
	if (sstream == NULL)
		acl_msg_fatal("can't listen on addr(%s)", addr);

	printf("started, listen on %s ...\r\n", addr);
	while (1) {
		client = acl_vstream_accept(sstream, NULL, 0);
		acl_workq_add(wq, client_thread, 0, client);	
	}

	exit (0);
}
