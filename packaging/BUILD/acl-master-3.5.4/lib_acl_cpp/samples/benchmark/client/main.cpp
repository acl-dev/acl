#include "stdafx.h"
#include "util.h"

static int __max = 10;
static int __dlen = 100;

static void handle_connection(acl::socket_stream& conn)
{
	acl::string rbuf, wbuf;

	for (int i = 0; i < __dlen; i++)
		wbuf += 'X';
	wbuf += "\r\n";

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (int i = 0; i < __max; i++)
	{
		if (conn.write(wbuf) == -1)
		{
			printf("write to server error\r\n");
			break;
		}

		if (conn.gets(rbuf) == false)
		{
			printf("readline from server error\r\n");
			break;
		}
		if (i < 10 && !rbuf.empty())
			printf("readline: %s\r\n", rbuf.c_str());

		if (i % 1000 == 0)
		{
			char tmp[64];
			snprintf(tmp, sizeof(tmp), "total: %d, curr: %d", __max, i);
			ACL_METER_TIME(tmp);
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double n = util::stamp_sub(&end, &begin);
	printf("total get: %d, spent: %0.2f ms, speed: %0.2f\r\n",
		__max, n, (__max * 1000) /(n > 0 ? n : 1));
}

static void* thread_read(void* ctx)
{
	acl::socket_stream* conn = (acl::socket_stream*) ctx;
	acl::string buf;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int i;
	for (i = 0; i < __max; i++)
	{
		if (conn->gets(buf, false) == false)
		{
			printf("readline from client over!\r\n");
			break;
		}
		if (i % 10000 == 0)
			printf("read count: %d, readline: %s", i, buf.c_str());
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double n = util::stamp_sub(&end, &begin);
	printf("total read: %d, spent: %0.2f ms, speed: %0.2f\r\n",
			i, n, (i * 1000) /(n > 0 ? n : 1));

	return NULL;
}

static void* thread_write(void* ctx)
{
	acl::socket_stream* conn = (acl::socket_stream*) ctx;
	acl::string buf;

	int   i;
	for (i = 0; i < __dlen; i++)
		buf += 'X';
	buf += "\r\n";

	struct timeval begin;
	gettimeofday(&begin, NULL);

	for (i = 0; i < __max; i++)
	{
		if (conn->write(buf) == -1)
		{
			printf("write to client finish!\r\n");
			break;
		}
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double n = util::stamp_sub(&end, &begin);
	printf("total write: %d, spent: %0.2f ms, speed: %0.2f\r\n",
			i, n, (i * 1000) /(n > 0 ? n : 1));

	return NULL;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"\t-s server_addr[127.0.0.1:8888]\r\n"
		"\t-n max_loop[10]\r\n"
		"\t-p [separate read/write]\r\n"
		"\t-l package_length[100]\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::acl_cpp_init();

	acl::string addr("127.0.0.1:8888");
	int   ch;
	bool  separate = false;

	while ((ch = getopt(argc, argv, "hs:n:l:p")) > 0 )
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
			__dlen = atoi(optarg);
			if (__dlen <= 0)
				__dlen = 10;
			break;
		case 'p':
			separate = true;
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

	if (!separate)
	{
		handle_connection(conn);
		return 0;
	}

	acl_pthread_attr_t attr;
	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 0);

	acl_pthread_t tid_read, tid_write;

	acl_pthread_create(&tid_read, &attr, thread_read, &conn);
	acl_pthread_create(&tid_write, &attr, thread_write, &conn);

	acl_pthread_join(tid_read, NULL);
	acl_pthread_join(tid_write, NULL);

	printf("enter any key to exit\r\n");
	getchar();

	return 0;
}
