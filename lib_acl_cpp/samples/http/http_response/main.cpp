#include "stdafx.h"
#include <getopt.h>

static void handle_request(acl::socket_stream* conn)
{
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

		printf("read header ok\r\n");

		tmp.clear();
		if (res.get_body(tmp) == false)
		{
			printf("read_body error\r\n");
			break;
		}

		printf("request body's size: %d\r\n", (int) tmp.length());

		acl::http_header& header = res.response_header();
		header.set_status(200);

		acl::http_client* client = res.get_client();
		header.set_keep_alive(client->keep_alive());
		const char* ptr = client->header_value("Accept-Encoding");
		if (ptr && strstr(ptr, "gzip") != NULL)
			header.set_transfer_gzip(true);
		header.set_chunked(client->keep_alive() ? true : false);
		// header.set_content_length(buf.length());

		acl::string hdr;
		header.build_response(hdr);
		printf("response header:\r\n%s\r\n", hdr.c_str());

		if (res.response(buf.c_str(), buf.length()) == false)
		{
			printf("response error\r\n");
			break;
		}
		if (res.response(NULL, 0) == false)
		{
			printf("response trailer error\r\n");
			break;
		}

		printf("response body ok\r\n");
		printf("===============================================\r\n");

		n++;

		if (!client->keep_alive())
			break;
	}

	conn->close();
	printf(">>>> close client <<<<\r\n");
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr[127.0.0.1:8194]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch;
	acl::string addr("0.0.0.0:8194");

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

	while (true)
	{
		acl::socket_stream* conn = server.accept();
		if (conn == NULL)
		{
			printf("accept error\r\n");
			return 1;
		}
		printf("accept one: %s\r\n", conn->get_peer());

		handle_request(conn);
	}

	return 0;
}
