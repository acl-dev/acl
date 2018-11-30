#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.hpp"
#include "stamp.h"
#include "user_client.h"

#define	STACK_SIZE	128000

static const char* __user_prefix = "user";
static int __rw_timeout = 0;
static int __max_client = 10;
static int __cur_client = 10;
static struct timeval __begin;
static int __total_read = 0;

static bool client_login(user_client* uc)
{
	acl::string buf;
	buf.format("login|%s\r\n", uc->get_name());

	if (uc->get_stream().write(buf) == -1)
	{
		printf("login failed error, user: %s\r\n", uc->get_name());
		return false;
	}

	if (uc->get_stream().gets(buf) == false)
	{
		printf("login failed, gets error %s\r\n", acl::last_serror());
		return false;
	}

	return true;
}

static void client_logout(user_client* client)
{
	client->notify_exit();
}

static void fiber_reader(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	conn.set_rw_timeout(5);

	if (client_login(client) == false)
	{
		client_logout(client);
		return;
	}

	printf("fiber-%d: user: %s login OK\r\n", acl_fiber_self(),
		client->get_name());

	acl::string buf;

	const char* to = client->get_to();
	acl::string msg;
	int max_loop = client->get_max_loop(), i = 0, n = 0;

	for (;;)
	{
		msg.format("chat|%s|hello world\r\n", to);
		if (conn.write(msg) != (int) msg.size())
		{
			printf("fiber-%d: msg(%s) write error %s\r\n",
				acl_fiber_self(), msg.c_str(),
				acl::last_serror());
		}

		if (conn.gets(buf))
		{
			if (++i <= 1)
				printf("fiber-%d: gets->%s\r\n",
					acl_fiber_self(), buf.c_str());
			n++;
			__total_read++;
			if (n == max_loop)
				break;
			continue;
		}

		printf("%s(%d): user: %s, gets error %s, fiber: %d\r\n",
			__FUNCTION__, __LINE__, client->get_name(),
			acl::last_serror(), acl_fiber_self());

		if (client->existing())
		{
			printf("----existing now----\r\n");
			break;
		}

		if (errno == ETIMEDOUT)
			printf("ETIMEDOUT\r\n");
		else if (errno == EAGAIN)
			printf("EAGAIN\r\n");
		else {
			printf("gets error: %d, %s\r\n",
				errno, acl::last_serror());
			break;
		}
	}

	printf(">>%s(%d), user: %s, logout, loop: %d, ngets: %d\r\n",
		__FUNCTION__, __LINE__, client->get_name(), i, n);

	client_logout(client);
}

static void fiber_client(const char* addr, const char* user,
	const char* to, int loop)
{
	acl::socket_stream conn;
	if (conn.open(addr, 0, 10) == false)
	{
		printf("connect %s error %s\r\n", addr, acl::last_serror());
	}
	else
	{
		printf("fiber-%d, connect %s ok\r\n", acl_fiber_self(), addr);

		user_client* client = new user_client(conn, user, to, loop);
		go[=] {
			fiber_reader(client);
		};

		client->wait_exit();
		printf("--- client %s exit now ----\r\n", client->get_name());
		delete client;
	}

	__cur_client--;

	if (__cur_client == 0)
	{
		printf("-----All fibers over!-----\r\n");
		struct timeval end;
		gettimeofday(&end, NULL);
		double spent = stamp_sub(&end, &__begin);
		printf("---Total read: %d, spent: %.2f, speed: %.2f---\r\n",
			__total_read, spent,
			(1000 * __total_read) / (spent < 1 ? 1 : spent));

		acl_fiber_schedule_stop();
	}
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s server_addr\r\n"
		" -n max_loop\r\n"
		" -c max_client\r\n"
		" -r rw_timeout\r\n" , procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	int  ch, max_loop = 100;

	acl::log::stdout_open(true);
	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:c:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			snprintf(addr, sizeof(addr), "%s", optarg);
			break;
		case 'r':
			__rw_timeout = atoi(optarg);
			break;
		case 'c':
			__max_client = atoi(optarg);
			break;
		case 'n':
			max_loop = atoi(optarg);
			break;
		default:
			break;
		}
	}

	__cur_client = __max_client;

	gettimeofday(&__begin, NULL);

	for (int i = 0; i < __max_client; i++)
	{
		char user[128], to[128];

		snprintf(user, sizeof(user), "%s-%d", __user_prefix, i);

		if (i + 1 == __max_client)
			snprintf(to, sizeof(to), "%s-0", __user_prefix);
		else
			snprintf(to, sizeof(to), "%s-%d", __user_prefix, i);

		go[=] {
			fiber_client(addr, user, to, max_loop);
		};
	}

	acl::fiber::schedule();

	return 0;
}
