#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static void fiber1(void)
{
	printf("in fiber: %d\r\n", acl::fiber::self());
}

static void fiber2(int n, const char* s)
{
	printf("in fiber: %d, n: %d, s: %s\r\n", acl::fiber::self(), n, s);
}

static void fiber3(acl::string& buf)
{
	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
	buf = "world";
}

static void fiber4(const acl::string& buf)
{
	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
}

//////////////////////////////////////////////////////////////////////////////

static void fiber_check(int& n)
{
	for (int i = 0; i < 6; i++) {
		printf("%s, thread=%lu: check n=%d, sleep %d seconds\r\n",
			__FUNCTION__, acl::thread::self(), n, i);
		sleep(1);
	}
}

static void incr(int& n)
{
	for (int i = 0; i < 5; i++) {
		printf("%s, thread=%lu, n=%d, sleep %d seconds \r\n",
			__FUNCTION__, acl::thread::self(), n, i);
		sleep(1);
		n++;
	}
}

static void fiber_wait4thread(void)
{
	static int n = 100;

	printf("\r\n");
	printf("%s, thread=%lu, begin n=%d\r\n",
		__FUNCTION__, acl::thread::self(), n);
	printf("\r\n");

	go[&] {
		fiber_check(n);
	};

	go_wait_thread[&] {
		incr(n);
	};

	printf("\r\n");
	printf("%s, thread=%lu, end n=%d\r\n",
		__FUNCTION__, acl::thread::self(), n);
	printf("\r\n");
}

static void fiber_wait4fiber(void)
{
	static int n = 200;

	printf("\r\n");
	printf("%s: thrad=%lu, begin n=%d\r\n",
		__FUNCTION__, acl::thread::self(), n);
	printf("\r\n");

	go[&] {
		fiber_check(n);
	};

	go_wait_fiber[&] {
		incr(n);
	};

	printf("\r\n");
	printf("%s: thrad=%lu, end n=%d\r\n",
		__FUNCTION__, acl::thread::self(), n);
	printf("\r\n");
}

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -n fibers_count\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch, n = 10;

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hn:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			break;
		}
	}

	go fiber1;
	go fiber_wait4thread;
	go fiber_wait4fiber;

	go[=] {
		fiber2(n, "hello world");
	};

	acl::string buf("hello");

	go[&] {
		fiber3(buf);
	};

	go[&] {
		fiber4(buf);
	};

	go[=] {
		fiber4(buf);
	};

	acl::fiber::schedule();
	return 0;
}
