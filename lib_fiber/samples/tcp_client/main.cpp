#include "stdafx.h"
#include <string.h>

class thread_fiber : public acl::thread
{
public:
	thread_fiber(acl::tcp_ipc& ipc, const char* addr,
		int count, int n, bool ipc_mode)
	: ipc_(ipc)
	, addr_(addr)
	, count_(count)
	, n_(n)
	, ipc_mode_(ipc_mode)
	{
	}

	~thread_fiber(void) {}

protected:
	void* run(void)
	{
		if (ipc_mode_)
			run_ipc();
		else
			run_pool();
		return NULL;
	}

	void run_pool(void)
	{
		acl::tcp_pool pool(addr_, 0);
		char* s = (char*) malloc(n_ + 1);
		memset(s, 'x', n_);
		s[n_] = 0;

		for (int i = 0; i < count_; i++)
		{
#if 1
			if (pool.send(s, n_) == false)
			{
				printf("send error to %s\r\n", addr_.c_str());
				break;
			}
#else
			acl::tcp_client* conn = (acl::tcp_client*) pool.peek();
			if (conn == NULL)
			{
				printf("peek error from %s\r\n", addr_.c_str());
				break;
			}
			if (conn->send(s, n_) == false)
			{
				pool.put(conn, false);
				printf("send error to %s\r\n", addr_.c_str());
				break;
			}
			pool.put(conn);
#endif
		}
	}

	void run_ipc(void)
	{
		char* s = (char*) malloc(n_ + 1);
		memset(s, 'x', n_);
		s[n_] = 0;

		for (int i = 0; i < count_; i++)
		{
			if (ipc_.send(addr_, s, (unsigned) n_) == false)
			{
				printf("send error\r\n");
				break;
			}

			if (i > 0 && i % 100000 == 0)
			{
				char info[128];
				snprintf(info, sizeof(info), "thread=%lu, i=%d",
					acl::thread::thread_self(), i);
				acl::meter_time(__FILE__, __LINE__, info);
			}
		}

		free(s);
	}

private:
	acl::tcp_ipc& ipc_;
	acl::string addr_;
	int count_;
	int n_;
	bool ipc_mode_;
};

static void usage(const char* procname)
{
	printf("usage: %s -s server\r\n"
		" -n count\r\n"
		" -l data_size\r\n"
		" -i [if ipc_mode]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int ch, count = 10, nthreads = 4, n = 10;
	bool ipc_mode = false;
	acl::string addr("127.0.0.1:8887");

	while ((ch = getopt(argc, argv, "hs:n:t:l:i")) > 0)
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
			count = atoi(optarg);
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'l':
			n = atoi(optarg);
			break;
		case 'i':
			ipc_mode = true;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	acl::tcp_ipc ipc;
	std::vector<acl::thread*> threads;

	for (int k = 0; k < nthreads; k++)
	{
		thread_fiber* thread =
			new thread_fiber(ipc, addr, count, n, ipc_mode);
		thread->set_detachable(false);
		threads.push_back(thread);
		thread->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}

	return 0;
}
