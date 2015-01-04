#include "stdafx.h"
#include <getopt.h>

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr[127.0.0.1:8194]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch;
	acl::string addr("127.0.0.1:8194");

	while ((ch = getopt(argc, argv, "hs:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::server_socket server;
	if (server.open(addr) == false)
	{
		printf("listen error, addr: %s\r\n", addr.c_str());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	acl::socket_stream* conn = server.accept();
	if (conn == NULL)
	{
		printf("accept error\r\n");
		return 1;
	}
	printf("accept one: %s\r\n", conn->get_peer());

	acl::string buf(1024);
	for (size_t i = 0; i < 1024; i++)
		buf << 'X';

	acl::http_response res(conn);
	acl::string tmp(1024);

	int   n = 0;

	while (true)
	{
		if (res.read_header() == false)
		{
			printf("read_header error\r\n");
			break;
		}

		if (n < 10)
			printf("read header ok\r\n");

		tmp.clear();
		if (res.get_body(tmp) == false)
		{
			printf("read_body error\r\n");
			break;
		}

		if (n < 10)
			printf(">>size: %d\r\n", (int) tmp.length());

		acl::http_header& header = res.response_header();
		header.set_status(200);
		header.set_keep_alive(true);
		header.set_content_length(buf.length());

		if (res.response(buf.c_str(), buf.length()) == false)
		{
			printf("response error\r\n");
			break;
		}

		n++;
	}

	conn->close();

	return 0;
}
