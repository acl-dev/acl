#include "stdafx.h"
#if defined(_WIN32) || defined(_WIN64)
#include "lib_acl.h"
#else
#include <getopt.h>
#endif

class client_thread : public acl::thread
{
public:
	client_thread(acl::tcp_pool& pool, int count, int len)
	: pool_(pool), count_(count), len_(len) {}

	~client_thread(void) {}

protected:
	void* run(void)
	{
		char* s = new char[len_ + 1];
		memset(s, 'x', len_);
		s[len_] = 0;

		for (int i = 0; i < count_; i++) {
			if (pool_.send(s, len_) == false) {
				printf("send to %s error %s\r\n",
					pool_.get_addr(), acl::last_serror());
				break;
			}
		}

		delete []s;
		return NULL;
	}

private:
	acl::tcp_pool& pool_;
	int count_;
	int len_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s server_addr\r\n"
		" -c cocurrent\r\n"
		" -n loop_count\r\n"
		" -l data_length\r\n", procname);
}

int main(int argc, char* argv[])
{
	int ch, cocurrent = 4, count = 1000, len = 10;
	acl::string addr("127.0.0.1:8887");

	while ((ch = getopt(argc, argv, "hs:c:n:l:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 'l':
			len = atoi(optarg);
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	acl::tcp_pool pool(addr, count);

	std::vector<acl::thread*> threads;
	for (int i = 0; i < cocurrent; i++) {
		acl::thread* thread = new client_thread(pool, count, len);
		thread->set_detachable(false);
		threads.push_back(thread);
		thread->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {

		(*it)->wait();
		delete *it;
	}

	return 0;
}
