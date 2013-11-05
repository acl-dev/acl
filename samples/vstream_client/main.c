#include "lib_acl.h"

static void usage(const char *proc)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addr[default: 127.0.0.1:8888]\r\n"
		"	-n loop_count[default: 10]\r\n"
		"	-i inter count to print\r\n"
		"	-l data size[default: 4096]\r\n",
		proc);
}

int   main(int argc, char *argv[])
{
	ACL_VSTREAM *client;
	char  addr[64], *buf = NULL, line[128];
	int   n, i, len = 4096, count = 10, inter = 1000;

	snprintf(addr, sizeof(addr), "127.0.0.1:8888");

	while ((n = getopt(argc, argv, "hs:l:n:i:")) > 0) {
		switch (n) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'l':
			len = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'i':
			inter = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (len <= 0) {
		printf("invalid len: %d <= 0\r\n", len);
		return 1;
	}

	/* 连接服务器 */
	client = acl_vstream_connect(addr, ACL_BLOCKING, 10, 10, 4096);
	if (client == NULL) {
		printf("connect %s error %s\r\n", addr, acl_last_serror());
		return 1;
	}
	printf("connect %s ok ...\r\n", addr);

	buf = (char*) acl_mymalloc(len);
	snprintf(buf, len, "%d\r\n", len);

	/* 发送一行数据通知服务端每次数据体的长度 */
	if (acl_vstream_writen(client, buf, strlen(buf)) == ACL_VSTREAM_EOF)
	if (n == ACL_VSTREAM_EOF)
		goto END;

	memset(buf, 'X', len);

	for (i = 0; i < count; i++) {
		n = acl_vstream_writen(client, buf, len);
		if (n == ACL_VSTREAM_EOF) {
			printf("write error %s\r\n", acl_last_serror());
			break;
		}
		if (i % inter == 0) {
			snprintf(line, sizeof(line), "curr: %d, total: %d", i, count);
			ACL_METER_TIME(line);
		}
	}

END:

	if (buf)
		acl_myfree(buf);
	acl_vstream_close(client);

	return 0;
}
