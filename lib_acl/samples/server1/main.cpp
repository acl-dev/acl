#include "lib_acl.h"
#pragma comment(lib,"ws2_32")
#pragma comment(lib, "wsock32")

static void echo(ACL_VSTREAM *client)
{
	int   ret;
	char  buf[1024];

	while (1) {
		ret = acl_vstream_gets(client, buf, sizeof(buf));
		if (ret == ACL_VSTREAM_EOF) {
			break;
		}

		//ret = acl_vstream_fprintf(client, "hello world\r\nhello world\r\nhello world\r\n");
		ret = acl_vstream_fprintf(client, "hello world\r\n");
		if (ret == ACL_VSTREAM_EOF)
			break;
	}
}

int main(int argc, char *argv[])
{
	ACL_VSTREAM *sstream, *client;
	const char *addr = "127.0.0.1:30082";

	acl_init();

	sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL) {
		printf("listen on %s error(%s)\r\n", addr, acl_last_serror());
		getchar();
		return (0);
	}

	printf("listening on %s ...\r\n", addr);

	while (1) {
		client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL) {
			printf("accept error(%s)\r\n", acl_last_serror());
			break;
		}
		echo(client);
		acl_vstream_close(client);
	}

	printf("Over now\r\n");
	getchar();
	return (0);
}