#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#define	 STACK_SIZE	320000
static int __rw_timeout = 0;
static bool __echo = false;

class fiber_client : public acl::fiber
{
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override
	void run(void)
	{
		acl::string buf;
		acl::tcp_reader reader(*conn_);
		acl::tcp_sender sender(*conn_);

		while (true)
		{
			if (reader.read(buf) == false)
			{
				printf("read over %s\r\n", acl::last_serror());
				break;
			}
			if (__echo && sender.send(buf, buf.size()) == false)
			{
				printf("send error %s\r\n", acl::last_serror());
				break;
			}
			buf.clear();
		}

		delete conn_;
		delete this;
	}

private:
	acl::socket_stream* conn_;
	~fiber_client(void) {}
};


class fiber_server : public acl::fiber
{
public:
	fiber_server(const char* addr) : addr_(addr) {}

protected:
	void run(void)
	{
		acl::server_socket server;

		if (server.open(addr_) == false)
		{
			printf("open %s error\r\n", addr_.c_str());
			exit (1);
		}
		else
			printf("open %s ok\r\n", addr_.c_str());

		while (true)
		{
			acl::socket_stream* conn = server.accept();
			if (conn == NULL)
			{
				printf("accept failed: %s\r\n", acl::last_serror());
				break;
			}

			conn->set_rw_timeout(__rw_timeout);
			printf("accept one: %d\r\n", conn->sock_handle());
			acl::fiber* fb = new fiber_client(conn);
			fb->start();
		}

		delete this;
	}

private:
	acl::string addr_;

	~fiber_server(void) {}
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr -r rw_timeout -e\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl::string addr("127.0.0.1:8887");
	int  ch;

	while ((ch = getopt(argc, argv, "hs:r:e")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'e':
			__echo = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::fiber* fiber = new fiber_server(addr);
	fiber->start(STACK_SIZE);
	acl::fiber::schedule();

	return 0;
}
