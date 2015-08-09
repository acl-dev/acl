// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <sys/time.h>
#include "util.h"

class test_buf
{
public:
	test_buf()
	{
	}

	~test_buf()
	{
	}
};

class test_buf2
{
public:
	test_buf2(acl::dbuf_pool* pool)
		: pool_(pool)
	{
		(void) pool_;
	}

	~test_buf2()
	{
	}

	void *operator new(size_t size, acl::dbuf_pool* pool)
	{
		return pool->dbuf_alloc(size);
	}

	static void operator delete(void*)
	{
	}

private:
	acl::dbuf_pool* pool_;
};

class test_thread : public acl::thread
{
public:
	test_thread(int max_loop, int max_count, bool use_pool)
	: max_loop_(max_loop)
	, max_count_(max_count)
	, use_pool_(use_pool)
	{
	}

	~test_thread()
	{
	}

protected:
	void* run()
	{
		struct timeval begin;
		gettimeofday(&begin, NULL);

		if (use_pool_)
			test_pool();
		else
			test_malloc();

		struct timeval end;
		gettimeofday(&end, NULL);

		double spent = util::stamp_sub(&end, &begin);
		long long total = max_loop_ * max_count_;

		printf("loop: %lld, spent: %.4f, speed: %.4f\r\n",
			total, spent, total * 1000 / (spent > 0 ? spent : 1));

		return NULL;
	}

private:
	void test_pool()
	{
		acl::dbuf_pool* pool = new acl::dbuf_pool;
		test_buf2* buf;

		for (int i = 0; i < max_loop_; i++)
		{
			for (int j = 0; j < max_count_; j++)
			{
				buf = new (pool) test_buf2(pool);
				delete buf;
			}

			pool->dbuf_reset();
		}

		pool->destroy();
	}

	void test_malloc()
	{
		test_buf* buf;

		for (int i = 0; i < max_loop_; i++)
		{
			for (int j = 0; j < max_count_; j++)
			{
				buf = new test_buf;
				delete buf;
			}
		}
	}

private:
	int max_loop_;
	int max_count_;
	bool use_pool_;
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help] \r\n"
		"\t-n loop \r\n"
		"\t-m count \r\n"
		"\t-c max_threads \r\n"
		"\t-p [use memory pool]\r\n", procname);
}

int main(int argc, char* argv[])
{
	bool use_pool = false;
	int  ch, loop = 100, count = 10000, nthreads = 1;

	while ((ch = getopt(argc, argv, "hpn:m:c:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'p':
			use_pool = true;
			break;
		case 'n':
			loop = atoi(optarg);
			break;
		case 'm':
			count = atoi(optarg);
			break;
		case 'c':
			nthreads = atoi(optarg);
			break;
		default:
			break;
		}
	}

	std::vector<test_thread*> threads;
	for (int i = 0; i < nthreads; i++)
	{
		test_thread* thread = new test_thread(loop, count, use_pool);

		thread->set_detachable(false);
		thread->start();
		threads.push_back(thread);
	}

	struct timeval begin;
	gettimeofday(&begin, NULL);

	std::vector<test_thread*>::iterator it;
	for (it = threads.begin(); it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	double spent = util::stamp_sub(&end, &begin);
	long long total = loop * count * nthreads;

	printf("All over, total: %lld, spent: %.4f ms, speed: %.4f tps\r\n",
		total, spent, total * 1000 / (spent > 0 ? spent : 1));

	printf("total: %d, threads: %d\r\n", loop * count, nthreads);

	return 0;
}
