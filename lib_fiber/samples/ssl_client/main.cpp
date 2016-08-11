#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "stamp.h"

#define	 STACK_SIZE	32000

static int __rw_timeout = 0;
static int __conn_timeout = 0;
static int __max_fibers   = 100;
static int __left_fibers  = 100;
static int __max_loop     = 100;
static struct timeval __begin;

static acl::polarssl_conf *__ssl_conf = NULL;
static bool __check_ssl = false;

static long long int __total_count = 0;
static int __total_clients         = 0;
static int __total_error_clients   = 0;

static bool ssl_init(acl::socket_stream& conn)
{
	assert(__ssl_conf);

	acl::polarssl_io* ssl = new acl::polarssl_io(*__ssl_conf, false);

	if (conn.setup_hook(ssl) == ssl)
	{
		printf("setup_hook error\r\n");
		ssl->destroy();
		return false;
	}

	if (!__check_ssl)
	{
		printf("ssl handshake_ok\r\n");
		return true;
	}

	if (ssl->handshake() == false)
	{
		printf("ssl handshake error\r\n");
		ssl->destroy();
		return false;
	}
	if (ssl->handshake_ok() == false)
	{
		printf("ssl handshake error\r\n");
		ssl->destroy();
		return false;
	}

	printf("ssl handshake_ok\r\n");
	return true;
}

static void run(const char* addr)
{
	acl::socket_stream conn;

	if (conn.open(addr, __conn_timeout, __rw_timeout) == false)
	{
		printf("connect %s error %s\r\n", addr, acl::last_serror());
		__total_error_clients++;
		return;
	}

	if (__ssl_conf != NULL && ssl_init(conn) == false)
		return;

	__total_clients++;
	printf("fiber-%d: connect %s ok, clients: %d, fd: %d\r\n",
		acl_fiber_self(), addr, __total_clients, conn.sock_handle());

	acl::string buf;
	const char req[] = "hello world\r\n";

	for (int i = 0; i < __max_loop; i++)
	{
		if (conn.write(req, sizeof(req) - 1) == -1)
		{
			printf("write error: %s\r\n", acl::last_serror());
			break;
		}

		if (conn.gets(buf, false) == false)
		{
			printf("gets error: %s\r\n", acl::last_serror());
			break;
		}
		buf.clear();
		__total_count++;
	}

	printf("close one connection: %d, %s\r\n",
		conn.sock_handle(), acl::last_serror());
}

static void fiber_connect(ACL_FIBER *fiber acl_unused, void *ctx)
{
	const char *addr = (const char *) ctx;

	run(addr);

	--__left_fibers;
	printf("max_fibers: %d, left: %d\r\n", __max_fibers, __left_fibers);

	if (__left_fibers == 0) {
		double spent;
		struct timeval end;

		gettimeofday(&end, NULL);
		spent = stamp_sub(&end, &__begin);

		printf("fibers: %d, clients: %d, error: %d, count: %lld, "
			"spent: %.2f, speed: %.2f\r\n", __max_fibers,
			__total_clients, __total_error_clients,
			__total_count, spent,
			(__total_count * 1000) / (spent > 0 ? spent : 1));

		acl_fiber_stop();
	}
}

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx)
{
	char *addr = (char *) ctx;
	int i;

	for (i = 0; i < __max_fibers; i++)
	{
		acl_fiber_create(fiber_connect, addr, 32768);
		//acl_fiber_sleep(1);
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -r rw_timeout\r\n"
		" -c max_fibers\r\n"
		" -C [if check ssl status]\r\n"
		" -n max_loop\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl::string addr("127.0.0.1:9001");
	bool use_ssl = true;
	int  ch;

	while ((ch = getopt(argc, argv, "hs:r:Pc:n:C")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'P':
			use_ssl = false;
			break;
		case 'c':
			__max_fibers = atoi(optarg);
			break;
		case 'n':
			__max_loop = atoi(optarg);
			break;
		case 'C':
			__check_ssl = true;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	if (use_ssl)
		__ssl_conf = new acl::polarssl_conf;

	gettimeofday(&__begin, NULL);
	acl_fiber_create(fiber_main, addr.c_str(), 32768);

	acl_fiber_schedule();

	delete __ssl_conf;

	return 0;
}
