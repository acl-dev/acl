#include "stdafx.h"

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -s addr[127.0.0.1:8088] -n max_client[100]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, max = 100;
	acl::string addr("127.0.0.1:8088");

	while ((ch = getopt(argc, argv, "hs:n:")) > 0)
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
			max = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	acl::server_socket server;
	if (server.open(addr) == false)
	{
		printf("can't listen %s, %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	else
		printf("listen %s ok\r\n", addr.c_str());

	std::list<acl::socket_stream*> clients;
	for (int i = 0; i < max; i++)
	{
		acl::socket_stream* conn = server.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}
		clients.push_back(conn);
		printf("accept one ok, max: %d, curr: %d, fd: %d\r\n",
			max, i, conn->sock_handle());
	}

	printf("enter any key to exist\r\n");
	getchar();

	std::list<acl::socket_stream*>::iterator it = clients.begin();
	for (; it != clients.end(); ++it)
		delete (*it);

	printf("Over now\r\n");
	return 0;
}
