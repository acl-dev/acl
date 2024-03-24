#include "stdafx.h"
#include "echo_client.h"

static int __stack_size = 320000;
static int __rw_timeout = 5;
static int __echo_data  = 0;

static void fiber_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	echo_client(cstream, __rw_timeout, __echo_data);
}

static void fiber_accept(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		acl_fiber_create(fiber_client, cstream, __stack_size);
	}

	acl_vstream_close(sstream);
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		"  -s listen_addr\r\n"
		"  -r rw_timeout [default: 5]\r\n"
		"  -z stack_size\r\n"
		"  -w [if echo data, default: no]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int   ch, qlen = 128;

	acl_msg_stdout_enable(1);
	acl_fiber_msg_stdout_enable(1);

	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:wz:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'w':
			__echo_data = 1;
			break;
		case 'z':
			__stack_size = atoi(optarg);
			break;
		default:
			break;
		}
	}

	sstream = acl_vstream_listen(addr, qlen);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	acl_fiber_create(fiber_accept, sstream, 327680);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule();

	return 0;
}
