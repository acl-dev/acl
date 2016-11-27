#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.hpp"
#include "user_client.h"

#define	STACK_SIZE	32000

static int __rw_timeout = 0;

static acl::channel<int> __chan_monitor;
static std::map<acl::string, user_client*> __users;

static void remove_user(user_client* uc)
{
	const char* name = uc->get_name();
	if (name == NULL || *name == 0)
	{
		printf("%s(%d): no name!\r\n", __FUNCTION__, __LINE__);
		return;
	}

	std::map<acl::string, user_client*>::iterator it = __users.find(name);
	if (it == __users.end())
		printf("%s(%d): not exist, name: %s\r\n",
			__FUNCTION__, __LINE__, name);
	else
	{
		__users.erase(it);
		printf("delete user ok, name: %s\r\n", name);
	}
}

static void client_logout(user_client* client)
{
	if (client->already_login())
		remove_user(client);

	if (client->is_reading())
	{
		printf("%s(%d): user: %s, kill_reader\r\n",
			__FUNCTION__, __LINE__, client->get_name());
		client->kill_reader();
	}

	if (client->is_waiting())
	{
		printf("fiber-%d: %s(%d): user: %s, notify logout\r\n",
			acl_fiber_self(), __FUNCTION__, __LINE__,
			client->get_name());
		client->notify(MT_LOGOUT);
	}

	if (!client->is_reading() && !client->is_waiting())
		client->notify_exit();
}

static bool client_flush(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	acl::string* msg;

	bool ret = true;

	while ((msg = client->pop()) != NULL)
	{
		if (conn.write(*msg) == -1)
		{
			printf("flush to user: %s error %s\r\n",
				client->get_name(), acl::last_serror());
			delete msg;
			ret = false;
			break;
		}

		delete msg;
	}

	return ret;
}

static int __nwriter = 0;

static void fiber_writer(user_client* client)
{
	client->set_waiter();
	client->set_waiting(true);

	while (true)
	{
		int mtype;
		client->wait(mtype);
		if (client_flush(client) == false)
		{
			printf("%s(%d), user: %s, flush error %s\r\n",
				__FUNCTION__, __LINE__, client->get_name(),
				acl::last_serror());
			break;
		}

#ifdef USE_CHAN
		if (mtype == MT_LOGOUT)
		{
			printf("%s(%d), user: %s, MT_LOGOUT\r\n",
				__FUNCTION__, __LINE__, client->get_name());
			break;
		}
		if (mtype == MT_KICK)
		{
			printf("%s(%d), user: %s, MT_KICK\r\n",
				__FUNCTION__, __LINE__, client->get_name());
			client->get_stream().write("You're kicked\r\n");
			break;
		}
#endif
	}

	client->set_waiting(false);
	printf(">>%s(%d), user: %s, logout\r\n", __FUNCTION__, __LINE__,
		client->get_name());
	client_logout(client);

	printf("-------__nwriter: %d-----\r\n", --__nwriter);
}

static bool client_login(user_client* uc)
{
	acl::string buf;

	while (true)
	{
		if (uc->get_stream().gets(buf) == false)
		{
			printf("%s(%d): gets error %s\r\n",
				__FUNCTION__, __LINE__, acl::last_serror());

			if (errno == ETIMEDOUT)
			{
				printf("Login timeout\r\n");
				uc->get_stream().write("Login timeout\r\n");
			}
			return false;
		}
		if (!buf.empty())
			break;
	}

	std::vector<acl::string>& tokens = buf.split2("|");
	if (tokens.size() < 2)
	{
		acl::string tmp;
		tmp.format("invalid argc: %d < 2\r\n", (int) tokens.size());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	acl::string msg;

	const acl::string& name = tokens[1];
	std::map<acl::string, user_client*>::iterator it = __users.find(name);
	if (it == __users.end())
	{
		__users[name] = uc;
		uc->set_name(name);
		msg.format("user %s login ok\r\n", name.c_str());
	}
	else
		msg.format("user %s already login\r\n", name.c_str());

	printf("%s", msg.c_str());

	return uc->get_stream().write(msg) != -1;
}

static bool client_chat(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 3)
	{
		printf("invalid argc: %d < 3\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];
	const acl::string& msg = tokens[2];

	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("chat >> from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	it->second->push(msg);
	it->second->notify(MT_MSG);
	return true;
}

static bool client_kick(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("invalid argc: %d < 2\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];

	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("kick >> from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	it->second->notify(MT_KICK);

	return true;
}

static int __nreader = 0;

static void fiber_reader(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	conn.set_rw_timeout(0);

	client->set_reader();
	client->set_reading(true);

	if (client_login(client) == false)
	{
		client->set_reading(false);
		printf("----------client_logout-------\r\n");
		client_logout(client);

		printf("----__nreader: %d-----\r\n", --__nreader);
		return;
	}

	go_stack(STACK_SIZE) [&] {
		__nwriter++;
		fiber_writer(client);
	};

	conn.set_rw_timeout(0);

	bool stop = false;
	acl::string buf;

	while (true)
	{
		bool ret = conn.gets(buf);
		if (ret == false)
		{
			printf("%s(%d): user: %s, gets error %s, fiber: %d\r\n",
				__FUNCTION__, __LINE__, client->get_name(),
				acl::last_serror(), acl_fiber_self());

			if (client->exiting())
			{
				printf("----exiting now----\r\n");
				break;
			}

			if (errno == ETIMEDOUT)
			{
				if (conn.write("ping\r\n") == -1)
				{
					printf("ping error\r\n");
					break;
				}
			}
			else if (errno == EAGAIN)
				printf("EAGAIN\r\n");
			else {
				printf("gets error: %d, %s\r\n",
					errno, acl::last_serror());
				break;
			}

			continue;
		}

		if (buf.empty())
			continue;

		std::vector<acl::string>& tokens = buf.split2("|");
		if (tokens[0] == "quit" || tokens[0] == "exit")
		{
			conn.write("Bye!\r\n");
			break;
		}
		else if (tokens[0] == "chat")
		{
			if (client_chat(client, tokens) == false)
				break;
		}
		else if (tokens[0] == "kick")
		{
			if (client_kick(client, tokens) == false)
				break;
		}
		else if (tokens[0] == "stop")
		{
			stop = true;
			break;
		}
		else
			printf("invalid data: %s, cmd: [%s]\r\n",
				buf.c_str(), tokens[0].c_str());
	}

	printf(">>%s(%d), user: %s, logout\r\n", __FUNCTION__, __LINE__,
		client->get_name());

	client->set_reading(false);
	client_logout(client);

	printf("----__nreader: %d-----\r\n", --__nreader);

	if (stop)
	{
		int dumy = 1;
		__chan_monitor.put(dumy);
	}
}

static int __nclients = 0;

static void fiber_client(acl::socket_stream* conn)
{
	user_client* client = new user_client(*conn);

	go_stack(STACK_SIZE) [=] {
		__nreader++;
		fiber_reader(client);
	};

	client->wait_exit();

	printf("----- client (%s), exit now -----\r\n", client->get_name());
	delete client;
	delete conn;

	printf("----__nclients: %d-----\r\n", --__nclients);
	printf("----dead fibers: %d---\r\n", acl_fiber_ndead());
}

static ACL_FIBER *__fiber_accept = NULL;

static void fiber_accept(acl::server_socket& ss)
{
	__fiber_accept = acl_fiber_running();

	while (true)
	{
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		go_stack(STACK_SIZE) [=] {
			__nclients++;
			fiber_client(conn);
		};
	}
}

static void fiber_monitor(void)
{
	int n;

	__chan_monitor.pop(n);

	printf("--- kill fiber_accept ---\r\n");
	acl_fiber_kill(__fiber_accept);

	printf("--- stop fiber schedule ---\r\n");
	acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -r rw_timeout\r\n" , procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	int  ch;

	acl::log::stdout_open(true);
	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:")) > 0) {
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
		default:
			break;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	go[&] {
		fiber_accept(ss);
	};

	go[] {
		fiber_monitor();
	};

	acl::fiber::schedule();

	return 0;
}
