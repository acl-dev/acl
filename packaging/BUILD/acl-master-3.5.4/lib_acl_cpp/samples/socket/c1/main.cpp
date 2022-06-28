#include "acl_cpp/lib_acl.hpp"
#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>

class thread_client : public acl::thread
{
public:
	thread_client(acl::socket_stream* conn)
		: conn_(conn)
	{
	}

	~thread_client(void)
	{
		delete conn_;
	}

	void* run(void)
	{
		acl::string buf;
		if (conn_->gets(buf) == false)
			printf("gets error: %s\r\n", acl::last_serror());
		else
			printf("gets ok: %s\r\n", buf.c_str());

		return NULL;
	}

private:
	acl::socket_stream* conn_;
};

int main(int argc, char* argv[])
{
	acl::string addr = "127.0.0.1:9001";

	if (argc >= 2)
		addr = argv[1];

	acl::acl_cpp_init();

	int  max = 100;
	std::vector<acl::socket_stream*> conns;
	std::vector<acl::thread*> threads;
	for (int i = 0; i < max; i++)
	{
		acl::socket_stream* client = new acl::socket_stream;
		if (client->open(addr, 2, 10) == false)
		{
			printf("open %s error %s\n",
				addr.c_str(), acl::last_serror());
			return 1;
		}
		else
			printf("open %s ok\r\n", addr.c_str());

		acl::thread* thread = new thread_client(client);
		thread->set_detachable(false);
		thread->start();
		threads.push_back(thread);

	}

	for (int i = 0; i < max; i++)
	{
		threads[i]->wait(NULL);
		delete threads[i];
	}

	return (0);
}
