// xml.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#if !defined(_WIN32) && !defined(_WIN64)
#include <sys/time.h>
#endif

#define	CHECK(r, x, y) do { \
	if ((x) == (r)) { \
		printf("check OK %d\r\n", y); \
	} else { \
		printf("check error %d\r\n", y); \
	} \
} while(0)

static void usage(const char* procname)
{
	printf("usage: %s -h[help] -r [use dbuf_reset]\r\n", procname);
}

int main(int argc, char* argv[])
{
	int  ch;
	bool use_reset = false;

	while ((ch = getopt(argc, argv, "hr")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'r':
			use_reset = true;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	acl::dbuf_pool* dbuf = new (100) acl::dbuf_pool;

	for (int i = 0; i < 102400; i++)
		dbuf->dbuf_alloc(10);

	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(2048);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	dbuf->dbuf_alloc(1024);
	if (use_reset)
		dbuf->dbuf_keep(dbuf->dbuf_alloc(1024));
	dbuf->dbuf_alloc(10240);

	CHECK(true, dbuf->dbuf_reset(32999), 32999);
	CHECK(true, dbuf->dbuf_reset(22999), 22999);
	CHECK(true, dbuf->dbuf_reset(12999), 12999);
	CHECK(true, dbuf->dbuf_reset(8192), 8192);
	CHECK(true, dbuf->dbuf_reset(819), 819);
	CHECK(true, dbuf->dbuf_reset(19), 19);
	CHECK(true, dbuf->dbuf_reset(9), 9);
	CHECK(true, dbuf->dbuf_reset(0), 9);
	CHECK(use_reset, dbuf->dbuf_reset(8192), 8192);

	dbuf->destroy();
	return 0;
}
