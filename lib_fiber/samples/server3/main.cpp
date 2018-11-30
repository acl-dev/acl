#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class fiber_client : public acl::fiber
{
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override
	void run(void)
	{
		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());

		char buf[8192];
		while (true)
		{
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1)
				break;
			if (conn_->write(buf, ret) == -1)
				break;
		}

		delete conn_;
		delete this;
	}

private:
	acl::socket_stream* conn_;

	~fiber_client(void) {}
};

static void test_file(void)
{
	const char* path = "./Makefile";
	acl::string buf;
	acl::ifstream in;
	if (in.open_read(path) == false)
		printf("open file %s error %s\r\n", path, acl::last_serror());
	else if (in.gets(buf) == false)
		printf("gets from %s error %s", path, acl::last_serror());
	else
		printf("gets ok: %s\r\n", buf.c_str());
}

class fiber_server : public acl::fiber
{
public:
	fiber_server(acl::server_socket& ss) : ss_(ss) {}
	~fiber_server(void) {}

protected:
	// @override
	void run(void)
	{
		test_file();

		while (true)
		{
			acl::socket_stream* conn = ss_.accept();
			if (conn == NULL)
			{
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			printf("accept ok, fd: %d\r\n", conn->sock_handle());
			// create one fiber for one connection
			fiber_client* fc = new fiber_client(conn);
			// start the client fiber
			fc->start();
		}
	}

private:
	acl::server_socket& ss_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

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

	acl::server_socket ss;
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	fiber_server fs(ss);
	fs.start();		// start listen fiber

	acl::fiber::schedule();	// start fiber schedule
	return 0;
}
