#include "stdafx.h"
#include <string.h>

static bool __read_echo = false;
static int  __cocurrent = 10;

static void run_pool(const char* addr, int length, int count)
{
	acl::string buf;
	acl::tcp_pool pool(addr, 0);
	char* s = (char*) malloc(length + 1);
	memset(s, 'x', length);
	s[length] = 0;

	for (int i = 0; i < count; i++)
	{
#if 1
		if (!pool.send(s, length, __read_echo ? &buf : NULL))
		{
			printf("send error to %s\r\n", addr);
			break;
		}
		buf.clear();
#else
		acl::tcp_client* conn = (acl::tcp_client*) pool.peek();
		if (conn == NULL)
		{
			printf("peek error from %s\r\n", addr);
			break;
		}
		if (!conn->send(s, length, __read_echo ? &buf : NULL))
		{
			pool.put(conn, false);
			printf("send error to %s\r\n", addr);
			break;
		}
		pool.put(conn);
		buf.clear();
#endif
	}
}

static void run_ipc(const char* addr, acl::tcp_ipc& ipc, int length, int count)
{
	acl::string buf;
	char* s = (char*) malloc(length + 1);
	memset(s, 'x', length);
	s[length] = 0;

	for (int i = 0; i < count; i++)
	{
		if (ipc.send(addr, s, (unsigned) length,
			__read_echo ? &buf : NULL) == false)
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

class thread_client : public acl::thread
{
public:
	thread_client(acl::tcp_ipc& ipc, const char* addr,
		int count, int length, bool ipc_mode)
	: ipc_(ipc)
	, addr_(addr)
	, count_(count)
	, length_(length)
	, ipc_mode_(ipc_mode)
	{
	}

	~thread_client(void) {}

protected:
	void* run(void)
	{
		if (ipc_mode_)
			run_ipc(addr_, ipc_, length_, count_);
		else
			run_pool(addr_, length_, count_);
		return NULL;
	}

private:
	acl::tcp_ipc& ipc_;
	acl::string addr_;
	int count_;
	int length_;
	bool ipc_mode_;
};

class fiber_client : public acl::fiber
{
public:
	fiber_client(acl::tcp_ipc& ipc, const char* addr, int count,
		int length, bool ipc_mode)
	: ipc_(ipc)
	, addr_(addr)
	, count_(count)
	, length_(length)
	, ipc_mode_(ipc_mode)
	{
	}

protected:
	void run(void)
	{
		if (ipc_mode_)
			run_ipc(addr_, ipc_, length_, count_);
		else
			run_pool(addr_, length_, count_);
		delete this;
		if (--__cocurrent == 0)
			acl::fiber::schedule_stop();
	}

private:
	acl::tcp_ipc& ipc_;
	acl::string addr_;
	int count_;
	int length_;
	bool ipc_mode_;

	~fiber_client(void) {}
};

static void usage(const char* procname)
{
	printf("usage: %s -s server\r\n"
		" -n count\r\n"
		" -c cocurrent[default: 4]\r\n"
		" -l data_size\r\n"
		" -f [if use fiber]\r\n"
		" -i [if ipc_mode]\r\n"
		" -r [if read echo]\r\n",
		procname);
}

int main(int argc, char* argv[])
{
	int ch, count = 10, cocurrent = 4, n = 10;
	bool ipc_mode = false, fiber_mode = false;
	acl::string addr("127.0.0.1:8887");

	while ((ch = getopt(argc, argv, "hs:n:c:l:irf")) > 0)
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
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'l':
			n = atoi(optarg);
			break;
		case 'i':
			ipc_mode = true;
			break;
		case 'r':
			__read_echo = true;
			break;
		case 'f':
			fiber_mode = true;
			break;
		default:
			usage(argv[0]);
			return 0;
		}
	}

	acl::tcp_ipc ipc;

	if (fiber_mode)
	{
		__cocurrent = cocurrent;
		for (int k = 0 ; k < cocurrent; k++)
		{
			fiber_client* fb = new fiber_client(ipc, addr, count,
				n, ipc_mode);
			fb->start();
		}

		acl::fiber::schedule();
	}
	else
	{
		std::vector<acl::thread*> threads;

		for (int k = 0; k < cocurrent; k++)
		{
			thread_client* thread =
				new thread_client(ipc, addr, count, n, ipc_mode);
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
	}

	return 0;
}
