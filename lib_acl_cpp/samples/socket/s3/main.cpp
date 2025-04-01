#include "acl_cpp/lib_acl.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>

class thread_client : public acl::thread
{
public:
	thread_client(acl::socket_stream* conn, int max, int dlen, bool zerocp)
	: conn_(conn), max_(max), dlen_(dlen), zerocp_(zerocp) {}

protected:
	void* run(void)
	{
		char* buf = (char*) malloc(dlen_);
		memset(buf, 'X', dlen_);
		int flags = MSG_NOSIGNAL;
		if (zerocp_) {
			flags |= MSG_ZEROCOPY;
			conn_->set_zerocopy(true);
		}

		for (int i = 0; i < max_; i++)
		{
			if (conn_->send(buf, dlen_, flags) == -1)
			{
				printf("send error %s\r\n", acl::last_serror());
				break;
			}
			if (zerocp_ && !conn_->wait_iocp(5000)) {
				printf("wait result error %s\r\n", acl::last_serror());
				break;
			}
		}

		printf("Send over!\r\n");
		free(buf);
		delete conn_;
		delete this;

		return NULL;
	}

private:
	acl::socket_stream* conn_;
	int max_;
	int dlen_;
	bool zerocp_;

	~thread_client(void) {}
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s address[default: 0.0.0.0:8082\r\n"
		" -n loop_max: 10\r\n"
		" -d data_length: 8192\r\n"
		" -Z [if using zerocopy, default: false]\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::server_socket server;
	acl::string addr = "0.0.0.0:8082";
	bool zerocp = false;
	int ch, max = 10, dlen = 8192;

	while ((ch = getopt(argc, argv, "hs:n:d:Z")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		case 'd':
			dlen = atoi(optarg);
			break;
		case 'Z':
			zerocp = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

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
		thread_client* thread = new thread_client(client, max, dlen, zerocp);
		thread->start();
	}

	return (0);
}
