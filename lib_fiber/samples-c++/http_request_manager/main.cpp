#include "stdafx.h"
#include "http_thread.h"

#define	STACK_SIZE 256000

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s http_addr\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -m threads_count\r\n"
		" -t conn_timeout\r\n"
		" -g use_global_cluster\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	int   ch, nthreads = 1, fibers_max = 100, oper_count = 100;
	acl::string addr("www.baidu.com:80"), passwd;
	int conn_timeout = 2, rw_timeout = 2;
	bool use_global_cluster = false;

	while ((ch = getopt(argc, argv, "hs:n:c:r:t:m:g")) > 0) {
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
		case 'g':
			use_global_cluster = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);
	acl_fiber_msg_stdout_enable(1);

	std::vector<acl::thread*> threads;
	int stack_size = STACK_SIZE;

	acl::http_request_manager cluster;
	cluster.bind_thread(true);
	cluster.set(addr, 0, conn_timeout, rw_timeout);

	for (int i = 0; i < nthreads; i++) {
		http_thread* thread;
		if (use_global_cluster) {
			thread = new http_thread(cluster, fibers_max,
					stack_size, oper_count);
		} else {
			thread = new http_thread(addr, conn_timeout, rw_timeout,
					fibers_max, stack_size, oper_count);
		}
		thread->set_detachable(false);
		thread->set_stacksize(stack_size * (fibers_max + 6400));
		threads.push_back(thread);
		thread->start();
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait(NULL);
		delete (*it);
	}

	return 0;
}
