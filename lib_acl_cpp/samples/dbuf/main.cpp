// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <sys/time.h>
#include "util.h"

class test_buf
{
public:
	test_buf(acl::dbuf_pool* pool)
		: pool_(pool)
	{
	}

	~test_buf()
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

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n loop -c count -p [use memory pool]\r\n", procname);
}

int main(int argc, char* argv[])
{
	bool use_pool = false;
	int  ch, n = 100, c = 10000;

	while ((ch = getopt(argc, argv, "hpn:c:")) > 0)
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
			n = atoi(optarg);
			break;
		case 'c':
			c = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (use_pool)
	{
		acl::dbuf_pool* pool = new acl::dbuf_pool;
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < c; j++)
			{
				test_buf* buf = new (pool) test_buf(pool);
				delete buf;
			}

			pool->dbuf_reset();
		}
		delete pool;
	}
	else
	{
		for (int i = 0; i < n; i++)
		{
			for (int j = 0; j < c; j++)
			{
				test_buf* buf = (test_buf*) malloc(sizeof(test_buf));
				free(buf);
			}
		}
	}

	printf("total: %d\r\n", n * c);

	return 0;
}
