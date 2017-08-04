#include "stdafx.h"
#include "util.h"
#include "https_client.h"
#include "https_request.h"

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-f path of libpolarssl.so\r\n"
		"	-s server_addr [default: 127.0.0.1:8888]\r\n"
		"	-k [keep alive, default: false]\r\n"
		"	-L data_length [default: 1024]\r\n"
		"	-c cocurrent [default: 1]\r\n"
		"	-S [use ssl, default: no]\r\n"
		"	-n count [default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 1, count = 10, length = 1024;
	bool  keep_alive = false, use_ssl = false;
	acl::string server_addr("127.0.0.1:1443");
	acl::string domain, libpath("libpolarssl.so");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hf:s:c:n:kSH:")) > 0)
	{
		switch (ch)
		{
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
		case 'k':
			keep_alive = true;
			break;
		case 's':
			server_addr = optarg;
			break;
		case 'S':
			use_ssl = true;
			break;
		case 'H':
			domain = optarg;
			break;
		default:
			break;
		}
	}

	acl::polarssl_conf::set_libpath(libpath);
	acl::polarssl_conf::load();

	if (domain.empty())
		domain = server_addr;

	static acl::polarssl_conf ssl_conf;

	struct timeval begin;
	gettimeofday(&begin, NULL);

#if 0
	std::list<https_client*> threads;

	for (int i = 0; i < cocurrent; i++)
	{
		https_client* thread = new https_client(server_addr, domain,
				keep_alive, count, length);

		if (use_ssl)
			thread->set_ssl_conf(&ssl_conf);

		thread->set_detachable(false);

		threads.push_back(thread);

		thread->start();
	}

	std::list<https_client*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it)
	{
		if ((*it)->wait(NULL) == false)
			printf("wait one thread(%lu) error\r\n",
				(*it)->thread_id());
		/*
		else
			printf("wait one thread(%lu) ok\r\n",
				(*it)->thread_id());
		*/
		delete *it;

	}
#else
	(void) length;

	std::list<https_request*> threads;

	for (int i = 0; i < cocurrent; i++)
	{
		// ?????߳?
		https_request* thread = new https_request(server_addr,
				use_ssl ? &ssl_conf : NULL);

		// ???ô??????߳?Ϊ?Ƿ???ģʽ???Ա??????????Ե?? thread::wait
		// ?ȴ??߳̽???
		thread->set_detachable(false);

		// ???̷߳??ڶ?????
		threads.push_back(thread);

		// ?????߳?
		thread->start();
	}

//	sleep(2);

	std::list<https_request*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it)
	{
		// ?ȴ??߳̽???
		if ((*it)->wait(NULL) == false)
			printf("wait one thread(%lu) error\r\n",
				(*it)->thread_id());
		else
			printf("wait one thread(%lu) ok\r\n",
				(*it)->thread_id());
		// ɾ????̬???????̶߳???
		delete *it;

	}
#endif

	struct timeval end;
	gettimeofday(&end, NULL);

	double spent = util::stamp_sub(&end, &begin);
	printf("total: %d, spent: %.2f, speed: %.f\r\n", count,
		spent, (count * 1000) / (spent > 1 ? spent : 1));

	printf("enter any key to exit\r\n");
	getchar();

	return 0;
}
