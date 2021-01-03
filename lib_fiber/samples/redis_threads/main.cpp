#include "stdafx.h"
#include "redis_thread.h"

#define	STACK_SIZE 256000

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s redis_addr\r\n"
		" -g [if use global cluster, default: false]\r\n"
		" -p passwd\r\n"
		" -n operation_count\r\n"
		" -c fibers count\r\n"
		" -m threads_count\r\n"
		" -t conn_timeout\r\n"
		" -r rw_timeout\r\n"
		" -a command[set|get|del|all]\r\n"
		, procname);
}

int main(int argc, char *argv[])
{
	int   ch, nthreads = 1, fibers_max = 100, oper_count = 100;
	acl::string addr("127.0.0.1:6379"), passwd;
	int conn_timeout = 2, rw_timeout = 2;
	bool use_global_cluster = false;
	acl::string cmd = "del";

	while ((ch = getopt(argc, argv, "hs:n:c:r:t:m:p:ga:")) > 0) {
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
		case 'p':
			passwd = optarg;
			break;
		case 'g':
			use_global_cluster = true;
			break;
		case 'a':
			cmd = optarg;
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

	acl::redis_client_cluster cluster;
	cluster.bind_thread(true);
	cluster.set(addr, 0, conn_timeout, rw_timeout);
	cluster.set_password("default", passwd);

	for (int i = 0; i < nthreads; i++) {
		redis_thread* thread;
		if (use_global_cluster) {
			thread = new redis_thread(cluster, fibers_max,
				stack_size, oper_count, cmd);
		} else {
			thread = new redis_thread(addr, passwd, conn_timeout,
				rw_timeout, fibers_max, stack_size,
				oper_count, cmd);
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
