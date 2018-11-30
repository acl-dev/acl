#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "user_client.h"

#define	STACK_SIZE	32000

static int __rw_timeout = 0;

static acl::channel<int> __chan_monitor;
static std::map<acl::string, user_client*> __users;

// 从客户端列表中删除指定的客户端对象
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

// 客户端退出过程
static void client_logout(user_client* client)
{
	// 从客户端列表中删除
	if (client->already_login())
		remove_user(client);

	// 如果该客户端的读协程还在工作，则通知该读协程退出
	if (client->is_reading())
	{
		printf("%s(%d): user: %s, kill_reader\r\n",
			__FUNCTION__, __LINE__, client->get_name());
		client->kill_reader();
	}

	// 如果该客户端的写协程还在工作，则通知该写协程退出
	if (client->is_waiting())
	{
		printf("fiber-%d: %s(%d): user: %s, notify logout\r\n",
			acl_fiber_self(), __FUNCTION__, __LINE__,
			client->get_name());
		client->notify(MT_LOGOUT);
	}

	// 如果该客户端的读、写协程都已经退出，则通知该客户端退出
	if (!client->is_reading() && !client->is_waiting())
		client->notify_exit();
}

static bool client_flush(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	acl::string* msg;

	bool ret = true;

	// 从客户端的消息队列中提取消息并发送至该客户端
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

// 客户端的写协程处理过程
static void fiber_writer(user_client* client)
{
	client->set_waiter();
	client->set_waiting(true);

	while (true)
	{
		int mtype;

		// 等待消息通知
		client->wait(mtype);

		// 从本身消息队列中提取消息并发送至本客户端
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
#else
		if (client->exiting())
		{
			printf("%s(%d), user: %s exiting\r\n",
				__FUNCTION__, __LINE__, client->get_name());
			break;
		}
#endif
	}

	client->set_waiting(false);
	printf(">>%s(%d), user: %s, logout\r\n", __FUNCTION__, __LINE__,
		client->get_name());

	// 通知该客户端退出
	client_logout(client);

	printf("-------__nwriter: %d-----\r\n", --__nwriter);
}

// 客户端登入过程
static bool client_login(user_client* uc)
{
	acl::string buf;

	while (true)
	{
		// 读取一行数据，且自动去掉尾部的 \r\n
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

	// 分析登入消息，数据格式：login|xxx
	std::vector<acl::string>& tokens = buf.split2("|");
	if (tokens.size() < 2)
	{
		acl::string tmp;
		tmp.format("invalid argc: %d < 2\r\n", (int) tokens.size());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	acl::string msg;

	// 当该客户端不存在时添加进客户端列表中
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

	// 通知该客户端登入成功
	return uc->get_stream().write(msg) != -1;
}

// 与其它客户端聊天过程
static bool client_chat(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 3)
	{
		printf("invalid argc: %d < 3\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];
	const acl::string& msg = tokens[2];

	// 查找目标客户端对象
	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("chat >> from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	// 将消息内容加入目标客户端的消息队列中
	it->second->push(msg);
	// 通知目标客户端的写协程处理该消息
	it->second->notify(MT_MSG);
	return true;
}

// 踢出一个客户端对象
static bool client_kick(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("invalid argc: %d < 2\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];

	// 查找将被踢出的客户端对象
	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("kick >> from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	// 通知客户端写协程，其被踢出
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

	// 登入服务器
	if (client_login(client) == false)
	{
		client->set_reading(false);
		printf("----------client_logout-------\r\n");

		// 失败，则退出客户端
		client_logout(client);

		printf("----__nreader: %d-----\r\n", --__nreader);
		return;
	}

	// 登入成功，则创建写协程用来向客户端发送消息
	go_stack(STACK_SIZE) [&] {
		__nwriter++;
		fiber_writer(client);
	};

	conn.set_rw_timeout(0);

	bool stop = false;
	acl::string buf;

	// 从客户端循环读取消息
	while (true)
	{
		bool ret = conn.gets(buf);
		if (ret == false)
		{
			printf("%s(%d): user: %s, gets error %s, fiber: %d\r\n",
				__FUNCTION__, __LINE__, client->get_name(),
				acl::last_serror(), acl_fiber_self());

			// 客户端退出
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

		// 分析客户端发送的消息，交由不同的处理过程
		std::vector<acl::string>& tokens = buf.split2("|");

		// 本客户端要求退出
		if (tokens[0] == "quit" || tokens[0] == "exit")
		{
			conn.write("Bye!\r\n");
			break;
		}

		// 本客户端发送聊天消息
		else if (tokens[0] == "chat")
		{
			if (client_chat(client, tokens) == false)
				break;
		}

		// 本客户端踢出其它客户端
		else if (tokens[0] == "kick")
		{
			if (client_kick(client, tokens) == false)
				break;
		}

		// 要求整个服务进程退出
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

	// 退出客户端
	client_logout(client);

	printf("----__nreader: %d-----\r\n", --__nreader);

	if (stop)
	{
		int dumy = 1;
		// 如果要停止服务，则通知监控协程
		__chan_monitor.put(dumy);
	}
}

static int __nclients = 0;

static void fiber_client(acl::socket_stream* conn)
{
	// 创建客户端对象
	user_client* client = new user_client(*conn);

	// 创建从客户端连接读取数据的协程
	go_stack(STACK_SIZE) [=] {
		__nreader++;
		fiber_reader(client);
	};

	// 等待该客户端连接对象退出
	client->wait_exit();

	printf("----- client (%s), exit now -----\r\n", client->get_name());

	// 删除客户端对象及客户端连接对象
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
		// 等待接收客户端连接
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		// 创建处理客户端对象的协程
		go_stack(STACK_SIZE) [=] {
			__nclients++;
			fiber_client(conn);
		};
	}
}

static void fiber_monitor(void)
{
	int n;

	// 等待消息通知
	__chan_monitor.pop(n);

	printf("--- kill fiber_accept ---\r\n");
	// 杀死监听协程
	acl_fiber_kill(__fiber_accept);

	printf("--- stop fiber schedule ---\r\n");
	// 停止协程调度过程
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

	// 允许 acl 库的日志输出至屏幕
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

	acl::server_socket ss;	// 监听套接口对象
	// 监听指定地址
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	// 创建服务器接收连接协程
	go[&] {
		fiber_accept(ss);
	};

	// 创建监控协程
	go[] {
		fiber_monitor();
	};

	// 启动协程调度过程
	acl::fiber::schedule();

	return 0;
}
