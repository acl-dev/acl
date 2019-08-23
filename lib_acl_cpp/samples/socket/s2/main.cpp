#include "acl_cpp/lib_acl.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

class thread_client : public acl::thread
{
public:
	thread_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	void* run(void)
	{
		for (;;)
		{
			acl::string buf;

			if (conn_->gets(buf, false) == false)
				break;
			if (conn_->write(buf) == -1)
				break;
		}

		delete conn_;
		delete this;

		return NULL;
	}

private:
	acl::socket_stream* conn_;
	~thread_client(void) {}
};

int main(int argc, char* argv[])
{
	acl::server_socket server;
	acl::string addr = "127.0.0.1:9001";

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (argc >= 2)
		addr = argv[1];

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

		client->set_rw_timeout(10);
		thread_client* thread = new thread_client(client);
		thread->start();
	}

	return (0);
}
