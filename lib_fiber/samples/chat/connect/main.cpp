#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s server_ip\r\n"
		" -n max_loop\r\n", procname);
}

struct A {
	int m;
	ACL_RING entry;
};

static void test(void)
{
	ACL_RING ring;

	acl_ring_init(&ring);
	for (int i = 0; i < 10; i++)
	{
		A* a = new A;
		a->m = i;
		acl_ring_prepend(&ring, &a->entry);
		printf("put %d\r\n", i);
	}

	printf("-------------------------------------------------------\r\n");

	while (true)
	{
		ACL_RING* head = acl_ring_pop_head(&ring);
		if (head == NULL)
			break;
		A* a = ACL_RING_TO_APPL(head, A, entry);
		printf("pop %d\r\n", a->m);
		delete a;
	}

	exit (0);
}

int main(int argc, char *argv[])
{
	int   ch, max_loop = 100, n = 100;
	acl::string addr("127.0.0.1:9002");
       
	if (0)
		test();

	signal(SIGPIPE, SIG_IGN);

	while ((ch = getopt(argc, argv, "hn:s:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			max_loop = atoi(optarg);
			break;
		case 'l':
			n = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	for (int j = 0; j < n; j++)
	{
		std::vector<acl::socket_stream*> conns;
		int i = 0;
		for (; i < max_loop; i++)
		{
			acl::socket_stream* conn = new acl::socket_stream;
			if (conn->open(addr, 0, 0) == false)
			{
				printf("connect %s error %s\r\n",
					addr.c_str(), acl::last_serror());
				delete conn;
				break;
			}
			conns.push_back(conn);
		}

		printf("i: %d, max_loop: %d\r\n", i, max_loop);

		sleep(1);

		for (std::vector<acl::socket_stream*>::iterator
			it = conns.begin(); it != conns.end(); ++it)
		{
			//int sock = (*it)->sock_handle();
			//shutdown(sock, SHUT_RDWR);
			delete *it;
		}

		conns.clear();
	}

	return 0;
}
