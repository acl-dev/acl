#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

using namespace acl;

static void fiber1(void)
{
	printf("in fiber: %d\r\n", acl::fiber::self());
}

static void fiber2(int n, const char* s)
{
	printf("in fiber: %d, n: %d, s: %s\r\n", acl::fiber::self(), n, s);
}

static void fiber3(int id, acl::string& buf)
{
	printf("in fiber: %d, buf: %s, id=%d\r\n", acl::fiber::self(), buf.c_str(), id);
	buf = "world";
}

static void fiber4(const acl::string& buf)
{
	printf("in fiber: %d, buf: %s\r\n", acl::fiber::self(), buf.c_str());
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

	go[=] {
		fiber2(n, "hello world");
	};

	acl::string buf("hello");
	int id = 0;

	go[&] {
		fiber3(id++, buf);
	};

	go[&] {
		fiber4(buf);
	};

	go[=] {
		fiber4(buf);
	};

	//////////////////////////////////////////////////////////////////////

	acl::gofiber([&] {
		buf = "gofiber";
		fiber3(id++, buf);
	});

	acl::gofiber_stack([&] {
		buf = "gofiber_stack";
		fiber3(id++, buf);
	});

	acl::gofiber_stack([&] {
		buf = "gofiber_stack";
		fiber3(id++, buf);
	}, 64000);

	acl::gofiber_share([&] {
		buf = "gofiber_share";
		fiber3(id++, buf);
	});

	acl::gofiber_share([&] {
		buf = "gofiber_share";
		fiber3(id++, buf);
	}, 16000);

	go[&] {
		buf = "gofiber_wait_fiber";
		acl::gofiber_wait_fiber([&] {
			fiber3(id++, buf);
		});
	};

	acl::gofiber_wait_thread([&] {
		buf = "gofiber_wait_thread";
		printf("The thread: %ld, id=%d\n", acl::thread::self(), id);
		fiber3(id++, buf);
	});

	acl::fiber::schedule();
	return 0;
}
