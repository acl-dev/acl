#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>

class thread_client : public acl::thread
{
public:
	thread_client(int max, const char* addr) : max_(max), addr_(addr) {}

	~thread_client(void) {}

	void* run(void)
	{
		for (int i = 0; i < max_; i++)
			if (running() == false)
				break;
		return NULL;
	}

	bool running(void)
	{
		acl::socket_stream* conn = new acl::socket_stream;
		if (conn->open(addr_, 10, 10) == false)
		{
			printf("open %s error\r\n", addr_.c_str());
			return false;
		}

		if (conn->puts("hello world1") == -1)
		{
			printf("write error\r\n");
			delete conn;
			return false;
		}

		acl::string buf;
		if (conn->gets(buf, false) == false)
		{
			printf("gets error\r\n");
			delete conn;
			return false;
		}

		delete conn;
		return true;
	}

private:
	int   max_;
	acl::string addr_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-c max_threads\r\n"
		"	-s server_addr\r\n"
		"	-n io_loop\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, max_io = 100, max_threads = 1;
	acl::string addr = "127.0.0.1:9001";

	while ((ch = getopt(argc, argv, "hc:n:s:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			max_threads = atoi(optarg);
			break;
		case 'n':
			max_io = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	std::vector<acl::thread*> threads;
	for (int i = 0; i < max_threads; i++)
	{
		acl::thread* thread = new thread_client(max_io, addr);
		thread->set_detachable(false);
		thread->start();
		threads.push_back(thread);

	}

	for (int i = 0; i < max_threads; i++)
	{
		threads[i]->wait(NULL);
		delete threads[i];
	}

	return (0);
}
