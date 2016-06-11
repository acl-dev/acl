#include "stdafx.h"
#include "redis_thread.h"

#define	STACK_SIZE 32000

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s redis_addr\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -m threads_count\r\n"
		" -t conn_timeout\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, nthreads = 1, fibers_max = 100, oper_count = 100;
	acl::string addr("127.0.0.1:6379");
	int conn_timeout = 2, rw_timeout = 2;

	while ((ch = getopt(argc, argv, "hs:n:c:r:t:m:")) > 0)
	{
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			oper_count = atoi(optarg);
			break;
		case 'c':
			fibers_max = atoi(optarg);
			break;
		case 'r':
			rw_timeout = atoi(optarg);
			break;
		case 't':
			conn_timeout = atoi(optarg);
			break;
		case 'm':
			nthreads = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();

	std::vector<acl::thread*> threads;
	int stack_size = STACK_SIZE;

	for (int i = 0; i < nthreads; i++)
	{
		redis_thread* thread = new redis_thread(addr, conn_timeout,
			rw_timeout, fibers_max, stack_size, oper_count);
		thread->set_detachable(false);
		thread->set_stacksize(stack_size * (fibers_max + 6400));
		threads.push_back(thread);
		thread->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait(NULL);
		delete (*it);
	}

	return 0;
}
