#include "stdafx.h"
#include "util.h"

static void handle_connection(acl::socket_stream* conn, int len, int step)
{
	char* buf = (char*) malloc(len + 1), *ptr;
	int   i = 0, ret;

	len = (len / step) * step;
	if (len == 0)
	{
		printf("len: %d too small, step: %d\r\n", len, step);
		return;
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	while (true)
	{
		ptr = buf;
		for (int j = 0; j < len; j += step)
		{
			ret = conn->read(ptr, step, true);
			if (ret == -1)
			{
				printf("readline from client over!\r\n");
				goto END;
			}

			ptr += ret;
		}

		if (i <= 1)
		{
			*ptr = 0;
			printf("buf: %s\r\n", buf);
		}

		if (conn->write(buf, len) == -1)
		{
			printf("write to client error\r\n");
			break;
		}

		buf[0] = 0;
		i++;
	}

END:
	free(buf);

	struct timeval end;
	gettimeofday(&end, NULL);

	double n = util::stamp_sub(&end, &begin);
	printf("total get: %d, spent: %0.2f ms, speed: %0.2f\r\n",
		i, n, (i * 1000) /(n > 0 ? n : 1));
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"\t-s server_addr[127.0.0.1:8888]\r\n"
		"\t-l read_length[1024]\r\n"
		"\t-k step[2]\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::acl_cpp_init();

	acl::string addr("127.0.0.1:8888");
	int   ch, len = 1024, step = 2;

	while ((ch = getopt(argc, argv, "hs:l:k:")) > 0 )
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'l':
			len = atoi(optarg);
			break;
		case 'k':
			step = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	acl::server_socket server;
	if (server.open(addr) == false)
	{
		printf("listen %s error\r\n", addr.c_str());
		return -1;
	}
	else
		printf("listen %s ok\r\n", addr.c_str());

	acl::socket_stream* conn = server.accept();
	if (conn == NULL)
	{
		printf("accept error %s\r\n", acl::last_serror());
		return -1;
	}
	else
		printf("accept on client: %s\r\n", conn->get_peer());

	handle_connection(conn, len, step);
	delete conn;
	return 0;
}
