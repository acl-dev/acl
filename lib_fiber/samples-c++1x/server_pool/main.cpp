#include "stdafx.h"
#include "server_pool.h"

static void usage(const char* procname) {
	printf("usage: %s -h[help]\r\n"
		" -s ip:port, default: 127.0.0.1:9001\r\n"
		" -c fiber_pool_count [default: 100] \r\n"
		" -r read_timeout\r\n"
		" -w write_timeout\r\n"
		" -S [if in sync mode, default: false]\r\n"
		, procname);
}

int main(int argc, char* argv[]) {
	acl::string addr("127.0.0.1:9001");
	bool sync = false, use_unique = false;
	int  ch, nfibers = 100, rtimeo = -1, wtimeo = -1;

	while ((ch = getopt(argc, argv, "hs:Sc:r:w:U")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'S':
			sync = true;
			break;
		case 'c':
			nfibers = atoi(optarg);
			break;
		case 'U':
			use_unique = true;
			break;
		case 'r':
			rtimeo = atoi(optarg);
			break;
		case 'w':
			wtimeo = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::fiber::stdout_open(true);
	acl::log::stdout_open(true);

	if (use_unique) {
#if __cplusplus >= 201402L
		server_pool2_run(addr, sync, nfibers);
#else
		std::cout << "unique_ptr should be used for c++14" << std::endl;
#endif
	} else {
		server_pool_run(addr, sync, nfibers, rtimeo, wtimeo);
	}

	return 0;
}
