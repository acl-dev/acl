#include "stdafx.h"
#include "util.h"
#include "https_request.h"

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -f path of libpolarssl.so\r\n"
		" -s server_addr [default: 127.0.0.1:8888]\r\n"
		" -H host\r\n"
		" -U url\r\n"
		" -L data_length [default: 1024]\r\n"
		" -c cocurrent [default: 1]\r\n"
		" -S [use ssl, default: no]\r\n"
		" -n count [default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 1, count = 10;
	bool  use_ssl = false;
	acl::string server_addr("127.0.0.1:1443"), host;
	acl::string libpath("libpolarssl.so");
	acl::string url("/");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hf:s:c:n:SH:U:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			libpath = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'n':
			count = atoi(optarg);
			break;
		case 's':
			server_addr = optarg;
			break;
		case 'S':
			use_ssl = true;
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

	acl::sslbase_conf* ssl_conf = NULL;
	if (libpath.find("mbedtls")) {
		ssl_conf = new acl::mbedtls_conf(false);
		acl::mbedtls_conf::set_libpath(libpath.c_str());
		if (!acl::mbedtls_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
	} else if (libpath.find("polarssl")) {
		ssl_conf = new acl::polarssl_conf;
		acl::polarssl_conf::set_libpath(libpath);
		if (!acl::polarssl_conf::load()) {
			printf("load %s error\r\n", libpath.c_str());
			return 1;
		}
	} else {
		use_ssl = false;
	}

	if (host.empty()) {
		host = server_addr;
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	std::list<https_request*> threads;

	for (int i = 0; i < cocurrent; i++) {
		https_request* thread = new https_request(
			use_ssl ? ssl_conf : NULL, server_addr, host, url);

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
