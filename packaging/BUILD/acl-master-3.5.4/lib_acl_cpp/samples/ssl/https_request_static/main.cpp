#include "stdafx.h"
#include "util.h"
#include "https_request.h"

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s server_addr [default: 127.0.0.1:8888]\r\n"
		" -H host\r\n"
		" -U url\r\n"
		" -c cocurrent [default: 1]\r\n"
		" -n count [default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 1, count = 10;
	acl::string server_addr("127.0.0.1:1443"), host;
	acl::string url("/");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:c:n:H:U:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 's':
			server_addr = optarg;
			break;
		case 'H':
			host = optarg;
			break;
		case 'U':
			url = optarg;
			break;
		default:
			break;
		}
	}

	acl::sslbase_conf* ssl_conf = new acl::mbedtls_conf(false);

	if (host.empty()) {
		host = server_addr;
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	std::list<https_request*> threads;

	for (int i = 0; i < cocurrent; i++) {
		https_request* thread = new https_request(
			ssl_conf, server_addr, host, url);

		thread->set_detachable(false);
		threads.push_back(thread);
		thread->start();
	}

	std::list<https_request*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it) {
		if ((*it)->wait(NULL)) {
			printf("wait thread(%lu) ok\r\n", (*it)->thread_id());
		} else {
			printf("wait one thread(%lu) error\r\n",
				(*it)->thread_id());
		}

		delete *it;
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double spent = util::stamp_sub(&end, &begin);
	printf("total: %d, spent: %.2f, speed: %.f\r\n", count,
		spent, (count * 1000) / (spent > 1 ? spent : 1));

	delete ssl_conf;
	return 0;
}
