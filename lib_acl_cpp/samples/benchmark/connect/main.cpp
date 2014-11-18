#include "stdafx.h"

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -s server_addr[127.0.0.1:8088] -n max_connect[100]\r\n", procname);
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


	std::list<acl::socket_stream*> clients;

	for (int i = 0; i < max; i++)
	{
		acl::socket_stream* client = new acl::socket_stream();
		if (client->open(addr, 10, 10) == false)
		{
			printf("connect %s error %s\r\n",
				addr.c_str(), acl::last_serror());
			break;
		}
		else
			printf("connect %s ok, fd: %d, max: %d, curr: %d\r\n",
				addr.c_str(), client->sock_handle(), max, i);
		clients.push_back(client);
	}

	printf("enter any key to exist\r\n");
	getchar();

	std::list<acl::socket_stream*>::iterator it = clients.begin();
	for (; it != clients.end(); ++it)
		delete (*it);

	printf("Over now\r\n");
	return 0;
}
