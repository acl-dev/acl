#include "lib_acl.h"
#include <list>

static void test(const char* addr)
{
	std::list<ACL_VSTREAM*> conns;

	for (int i = 0; i < 10000; i++)
	{
		ACL_VSTREAM* client = acl_vstream_connect(addr, ACL_BLOCKING, 10, 10, 4096);
		if (client == NULL)
			printf("connect addr: %s error\r\n", addr);
		else
		{
			printf("connect addr: %s ok, i: %d\r\n", addr, i);
			conns.push_back(client);
		}
	}

	printf("enter any key to exit\r\n");
	getchar();

	std::list<ACL_VSTREAM*>::iterator it = conns.begin();
	for (; it != conns.end(); ++it)
		acl_vstream_close(*it);
	printf("Exit now ok\r\n");
}

int main(int argc, char *argv[])
{
	test(argv[1]);
	return 0;

	ACL_VSTREAM *client;
	const char *addr;
	char  buf[1024];
	int   ret;

	if (argc != 2) {
		printf("usage: %s addr\n", argv[0]);
		return (0);
	}

	addr = argv[1];

	acl_msg_open("connect.log", argv[0]);
	printf("connecting %s ...\n", argv[1]);

	//acl_poll_prefered(1);
	for (int i = 0; i < 10000; i++)
	{
		client = acl_vstream_connect(addr, ACL_BLOCKING, 10, 10, 4096);
		if (client == NULL) {
			printf("connect %s error(%s)\n", addr, acl_last_serror());
			return (1);
		}
		printf("connect %s ok, %s\n", addr, acl_last_serror());
	}

	printf(">>>>>>connect all ok\r\n");
	sleep(100);
	acl_vstream_fprintf(client, "%s", "line1\nline2\nline3\nline4\nline5\nline6\nline7\n");

	while (1) {
		ret = acl_vstream_gets_nonl(client, buf, sizeof(buf));
		if (ret > 0) {
			printf("gets from %s: %s\n", addr, buf);
		} else if (ret == ACL_VSTREAM_EOF) {
			printf("get over\r\n");
			break;
		}
	}

	acl_vstream_close(client);
	return (0);
}
