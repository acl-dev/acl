#include "stdafx.h"
#include "util.h"

static void handle_connection(acl::socket_stream* conn)
{
	acl::string buf;
	int   i = 0;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	while (true)
	{
		if (conn->gets(buf, false) == false)
		{
			printf("readline from client over!\r\n");
			break;
		}
		if (conn->write(buf) == -1)
		{
			printf("write to client error\r\n");
			break;
		}

		if (i % 1000 == 0)
			printf("read count: %d, readline: %s", i, buf.c_str());
		i++;
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double n = util::stamp_sub(&end, &begin);
	printf("total get: %d, spent: %0.2f ms, speed: %0.2f\r\n",
		i, n, (i * 1000) /(n > 0 ? n : 1));
}

static void* thread_read(void* ctx)
{
	acl::socket_stream* conn = (acl::socket_stream*) ctx;
	acl::string buf;

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int   i = 0;

	while (true)
	{
		if (conn->gets(buf, false) == false)
		{
			printf("readline from client over!\r\n");
			break;
		}
		if (i % 10000 == 0)
			printf("read count: %d, readline: %s", i, buf.c_str());
		i++;
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
	acl::string buf("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\r\n");

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int   i = 0;
	while (true)
	{
		if (conn->write(buf) == -1)
		{
			printf("write to client finish!\r\n");
			break;
		}
		i++;
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
		"\t-p [separate read/write in two thread]\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::acl_cpp_init();

	acl::string addr("127.0.0.1:8888");
	int   ch;
	bool  separate = false;

	while ((ch = getopt(argc, argv, "hs:p")) > 0 )
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'p':
			separate = true;
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

	// 接收一个客户端连接
	acl::socket_stream* conn = server.accept();
	if (conn == NULL)
	{
		printf("accept error %s\r\n", acl::last_serror());
		return -1;
	}
	else
		printf("accept on client: %s\r\n", conn->get_peer());

	if (!separate)
	{
		handle_connection(conn);
		delete conn;
		return 0;
	}

	acl_pthread_attr_t attr;
	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 0);

	acl_pthread_t tid_read, tid_write;

	acl_pthread_create(&tid_read, &attr, thread_read, conn);
	acl_pthread_create(&tid_write, &attr, thread_write, conn);

	acl_pthread_join(tid_read, NULL);
	acl_pthread_join(tid_write, NULL);

	delete conn;
	return 0;
}
