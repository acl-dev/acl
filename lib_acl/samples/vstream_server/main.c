#include "lib_acl.h"

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

static void usage(const char *proc)
{
	printf("usage: %s -h [help]\r\n"
		"	-s listen_addr[default: 127.0.0.1:8888]\r\n"
		"	-i inter count to print[default: 1000]\r\n",
		proc);
}

int   main(int argc, char *argv[])
{
	ACL_VSTREAM *server, *client;
	char  addr[64], *buf = NULL, line[128];
	int   n, i, len, inter = 1000;
	int   type = 0;
	double spent;
	struct timeval begin, end;

	snprintf(addr, sizeof(addr), "127.0.0.1:8888");

	while ((n = getopt(argc, argv, "hs:i:")) > 0) {
		switch (n) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'i':
			inter = atoi(optarg);
			break;
		default:
			break;
		}
	}

#if 0
	server = acl_vstream_listen(addr, 128);
	if (server == NULL) {
		printf("listen %s error %s\r\n", addr, acl_last_serror());
		return 1;
	}
#else
	if (strchr(addr, '/') != NULL || !acl_ipv4_addr_valid(addr)) {
		n = acl_unix_listen(addr, 128, ACL_BLOCKING);
#if defined(ACL_MACOSX)
		type = ACL_VSTREAM_TYPE_LISTEN_UNIX;
#endif
	} else {
		n = acl_inet_listen(addr, 127, ACL_BLOCKING);
#if defined(ACL_MACOSX)
		type = ACL_VSTREAM_TYPE_LISTEN_INET;
#endif
	}
	if (n == ACL_SOCKET_INVALID)
	{
		printf("listen %s error %s\r\n", addr, acl_last_serror());
		return 1;
	}
	server = acl_vstream_fdopen(n, 0, 8192, 0, type);
#endif

	printf("listening on %s ok!\r\n", addr);
	n = acl_check_socket(ACL_VSTREAM_SOCK(server));
	if (n == 1)
		printf("%d is listening socket\r\n", ACL_VSTREAM_SOCK(server));
	else if (n == 0)
		printf("%d is no-listening socket\r\n", ACL_VSTREAM_SOCK(server));
	else if (n == -1)
		printf("%d is not socket\r\n", ACL_VSTREAM_SOCK(server));

	n = acl_is_listening_socket(ACL_VSTREAM_SOCK(server));
	printf("server is listening socket: %s\r\n", n ? "yes" : "no");

	/* 接收外来客户端连接 */
	client = acl_vstream_accept(server, addr, sizeof(addr));
	if (client == NULL) {
		printf("accept error %s\r\n", acl_last_serror());
		acl_vstream_close(server);
		return 1;
	}
	printf("client is listening socket: %s\r\n",
		acl_is_listening_socket(ACL_VSTREAM_SOCK(client)) ? "yes" : "no");

	/* 从客户端读取一行数据，从而知道客户每次发送数据的长度 */
	n = acl_vstream_gets_nonl(client, line, sizeof(line));
	if (n == ACL_VSTREAM_EOF)
		goto END;

	len = atoi(line);
	if (len <= 0)
		goto END;

	buf = (char*) acl_mymalloc(len + 1);

	gettimeofday(&begin, NULL);

	i = 0;
	while (1) {
		n = acl_vstream_readn(client, buf, len);
		if (n == ACL_VSTREAM_EOF) {
			printf("read error %s\r\n", acl_last_serror());
			break;
		}
		buf[n] = 0;
		printf("readn: %s\r\n", buf);
		i++;
		if (i % inter == 0) {
			snprintf(line, sizeof(line), "curr: %d, nread: %d", i, n);
			ACL_METER_TIME(line);
		}
	}

	gettimeofday(&end, NULL);
	spent = stamp_sub(&end, &begin);
	printf("total: %d, spent: %.2f, speed: %.2f\r\n", i, spent,
		(i * 1000) / (spent > 1 ? spent : 1));

END:

	if (buf)
		acl_myfree(buf);
	acl_vstream_close(client);
	acl_vstream_close(server);

	return 0;
}
