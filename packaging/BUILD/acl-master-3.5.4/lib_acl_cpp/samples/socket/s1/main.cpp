#include "acl_cpp/lib_acl.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	acl::server_socket server;
	acl::string addr = "127.0.0.1:9001";

	if (argc >= 2)
		addr = argv[1];

	acl::acl_cpp_init();

	if (server.open(addr) == false)
	{
		printf("open %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	else
		printf("open %s ok\r\n", addr.c_str());

	while (true)
	{
		acl::socket_stream* client = server.accept();
		if (client == NULL)
		{
			printf("accept failed: %s\r\n", acl::last_serror());
			break;
		}

		printf("before set sendbuf's size: %d, readbuf's size: %d\r\n",
			client->get_tcp_sendbuf(), client->get_tcp_recvbuf());

		client->set_tcp_sendbuf(1024000);
		client->set_tcp_recvbuf(2048000);

		printf("after set sendbuf's size: %d, readbuf's size: %d\r\n",
			client->get_tcp_sendbuf(), client->get_tcp_recvbuf());

		/*
		acl::string buf;
		if (client->gets(buf))
			printf("gets: %s\r\n", buf.c_str());
		else
			printf("gets error: %s\r\n", acl::last_serror());

		delete client;
		printf("close client ok\r\n");
		*/
	}

	return (0);
}
