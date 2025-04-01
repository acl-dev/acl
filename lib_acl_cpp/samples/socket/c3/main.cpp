#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>

class thread_client : public acl::thread
{
public:
	thread_client(const char* addr) : addr_(addr) {}

	~thread_client(void) {}

	void* run(void)
	{
		running();
		return NULL;
	}

	bool running(void)
	{
		acl::socket_stream* conn = new acl::socket_stream;
		if (conn->open(addr_, 10, 10) == false)
		{
			printf("open %s error %s\r\n", addr_.c_str(),
				acl::last_serror());
			return false;
		}

		char buf[8192];
		while (!conn->eof()) {
			int ret = conn->read(buf, sizeof(buf), false);
			if (ret == -1) {
				break;
			}
		}

		delete conn;
		return true;
	}

private:
	acl::string addr_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -c max_threads[default: 1]\r\n"
		" -s server_addr[default: 127.0.0.1:8082]\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	int   ch, max_threads = 1;
	acl::string addr = "127.0.0.1:8082";

	while ((ch = getopt(argc, argv, "hc:s:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			max_threads = atoi(optarg);
			break;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	std::vector<acl::thread*> threads;
	for (int i = 0; i < max_threads; i++)
	{
		acl::thread* thread = new thread_client(addr);
		thread->set_detachable(false);
		thread->start();
		threads.push_back(thread);

	}

	for (int i = 0; i < max_threads; i++)
	{
		threads[i]->wait(NULL);
		delete threads[i];
	}

	return 0;
}
