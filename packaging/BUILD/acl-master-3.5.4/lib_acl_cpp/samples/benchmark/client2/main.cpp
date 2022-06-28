#include "stdafx.h"
#include "util.h"

static int __max = 10;
static int __len = 1024;

static void handle_connection(acl::socket_stream& conn)
{
	acl::string wbuf(__len + 1);
	char* buf = (char*) malloc(__len + 1);

	int   i;

	for (i = 0; i < __len; i++)
		wbuf += 'X';

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (i = 0; i < __max; i++)
	{
		if (conn.write(wbuf) == -1)
		{
			printf("write to server error\r\n");
			break;
		}

		if (conn.read(buf, __len, true) == -1)
		{
			printf("readline from server error\r\n");
			break;
		}

		if (i <= 1)
		{
			buf[__len] = 0;
			printf("buf: %s\r\n", buf);
		}

		if (i % 1000 == 0)
		{
			char tmp[64];
			snprintf(tmp, sizeof(tmp), "total: %d, curr: %d, len: %d",
				__max, i, __len);
			ACL_METER_TIME(tmp);
		}
	}

	free(buf);

	struct timeval end;
	gettimeofday(&end, NULL);

	double n = util::stamp_sub(&end, &begin);
	printf("total get: %d, spent: %0.2f ms, speed: %0.2f\r\n",
		__max, n, (i * 1000) /(n > 0 ? n : 1));
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"\t-s server_addr[127.0.0.1:8888]\r\n"
		"\t-n max_loop[10]\r\n"
		"\t-l package_length[1024]\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::acl_cpp_init();

	acl::string addr("127.0.0.1:8888");
	int   ch;

	while ((ch = getopt(argc, argv, "hs:n:l:")) > 0 )
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			__max = atoi(optarg);
			if (__max < 1)
				__max = 1;
			break;
		case 'l':
			__len = atoi(optarg);
			if (__len <= 0)
				__len = 10;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	acl::socket_stream conn;
	if (conn.open(addr, 30, 30) == false)
	{
		printf("connect %s error\r\n", addr.c_str());
		return -1;
	}

	handle_connection(conn);
	return 0;
}
