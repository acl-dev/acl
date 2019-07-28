#include "stdafx.h"
#include "util.h"
#include "thread_client.h"

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"	-s server_addr [default: 127.0.0.1:8888]\r\n"
		"	-k [keep alive, default: false]\r\n"
		"	-L data_length [default: 1024]\r\n"
		"	-c cocurrent [default: 1]\r\n"
		"	-n count [default: 10]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int   ch, cocurrent = 1, count = 10, length = 1024;
	bool  keep_alive = false;
	acl::string server_addr("127.0.0.1:8888");

	// ³õÊ¼»¯ acl ¿â
	acl::acl_cpp_init();

	while ((ch = getopt(argc, argv, "hs:c:n:k")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
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
		default:
			break;
		}
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	std::list<thread_client*> threads;

	for (int i = 0; i < cocurrent; i++)
	{
		// ´´½¨Ïß³
		thread_client* thread = new thread_client(server_addr, keep_alive,
				count, length);

		// ÉèÖÃ´´½¨µÄÏß³ÌÎª·Ç·ÖÀëÄ£Ê½£¬ÒÔ±ãÓÚÏÂÃæ¿ÉÒÔµ÷Ó thread::wait
		// µÈ´ýÏß³Ì½áÊø
		thread->set_detachable(false);

		// ½«Ïß³Ì·ÅÔÚ¶ÓÁÐÖ
		threads.push_back(thread);

		// Æô¶¯Ïß³
		thread->start();
	}

	std::list<thread_client*>::iterator it = threads.begin();
	for (; it != threads.end(); ++it)
	{
		// µÈ´ýÏß³Ì½áÊø
		if ((*it)->wait(NULL) == false)
			printf("wait one thread(%lu) error\r\n",
				(*it)->thread_id());
		// É¾³ý¶¯Ì¬´´½¨µÄÏß³Ì¶ÔÏó
		delete *it;

	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double spent = util::stamp_sub(&end, &begin);
	printf("total: %d, spent: %.2f, speed: %.f\r\n", count,
		spent, (count * 1000) / (spent > 1 ? spent : 1));

	return 0;
}
