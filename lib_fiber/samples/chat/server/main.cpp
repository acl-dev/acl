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

// 浠庡鎴风鍒楄〃涓垹闄ゆ寚瀹氱殑瀹㈡埛绔璞�
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

// 瀹㈡埛绔€€鍑鸿繃绋�
static void client_logout(user_client* client)
{
	// 浠庡鎴风鍒楄〃涓垹闄�
	if (client->already_login())
		remove_user(client);

	// 濡傛灉璇ュ鎴风鐨勮鍗忕▼杩樺湪宸ヤ綔锛屽垯閫氱煡璇ヨ鍗忕▼閫€鍑�
	if (client->is_reading())
	{
		printf("%s(%d): user: %s, kill_reader\r\n",
			__FUNCTION__, __LINE__, client->get_name());
		client->kill_reader();
	}

	// 濡傛灉璇ュ鎴风鐨勫啓鍗忕▼杩樺湪宸ヤ綔锛屽垯閫氱煡璇ュ啓鍗忕▼閫€鍑�
	if (client->is_waiting())
	{
		printf("fiber-%d: %s(%d): user: %s, notify logout\r\n",
			acl_fiber_self(), __FUNCTION__, __LINE__,
			client->get_name());
		client->notify(MT_LOGOUT);
	}

	// 濡傛灉璇ュ鎴风鐨勮銆佸啓鍗忕▼閮藉凡缁忛€€鍑猴紝鍒欓€氱煡璇ュ鎴风閫€鍑�
	if (!client->is_reading() && !client->is_waiting())
		client->notify_exit();
}

static bool client_flush(user_client* client)
{
	acl::socket_stream& conn = client->get_stream();
	acl::string* msg;

	bool ret = true;

	// 浠庡鎴风鐨勬秷鎭槦鍒椾腑鎻愬彇娑堟伅骞跺彂閫佽嚦璇ュ鎴风
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

// 瀹㈡埛绔殑鍐欏崗绋嬪鐞嗚繃绋�
static void fiber_writer(user_client* client)
{
	client->set_waiter();
	client->set_waiting(true);

	while (true)
	{
		int mtype;

		// 绛夊緟娑堟伅閫氱煡
		client->wait(mtype);

		// 浠庢湰韬秷鎭槦鍒椾腑鎻愬彇娑堟伅骞跺彂閫佽嚦鏈鎴风
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

	// 閫氱煡璇ュ鎴风閫€鍑�
	client_logout(client);

	printf("-------__nwriter: %d-----\r\n", --__nwriter);
}

// 瀹㈡埛绔櫥鍏ヨ繃绋�
static bool client_login(user_client* uc)
{
	acl::string buf;

	while (true)
	{
		// 璇诲彇涓€琛屾暟鎹紝涓旇嚜鍔ㄥ幓鎺夊熬閮ㄧ殑 \r\n
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

	// 鍒嗘瀽鐧诲叆娑堟伅锛屾暟鎹牸寮忥細login|xxx
	std::vector<acl::string>& tokens = buf.split2("|");
	if (tokens.size() < 2)
	{
		acl::string tmp;
		tmp.format("invalid argc: %d < 2\r\n", (int) tokens.size());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	acl::string msg;

	// 褰撹瀹㈡埛绔笉瀛樺湪鏃舵坊鍔犺繘瀹㈡埛绔垪琛ㄤ腑
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

	// 閫氱煡璇ュ鎴风鐧诲叆鎴愬姛
	return uc->get_stream().write(msg) != -1;
}

// 涓庡叾瀹冨鎴风鑱婂ぉ杩囩▼
static bool client_chat(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 3)
	{
		printf("invalid argc: %d < 3\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];
	const acl::string& msg = tokens[2];

	// 鏌ユ壘鐩爣瀹㈡埛绔璞�
	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("chat >> from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	// 灏嗘秷鎭唴瀹瑰姞鍏ョ洰鏍囧鎴风鐨勬秷鎭槦鍒椾腑
	it->second->push(msg);
	// 閫氱煡鐩爣瀹㈡埛绔殑鍐欏崗绋嬪鐞嗚娑堟伅
	it->second->notify(MT_MSG);
	return true;
}

// 韪㈠嚭涓€涓鎴风瀵硅薄
static bool client_kick(user_client* uc, std::vector<acl::string>& tokens)
{
	if (tokens.size() < 2)
	{
		printf("invalid argc: %d < 2\r\n", (int) tokens.size());
		return true;
	}

	const acl::string& to = tokens[1];

	// 鏌ユ壘灏嗚韪㈠嚭鐨勫鎴风瀵硅薄
	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("kick >> from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to.c_str());
		printf("%s", tmp.c_str());

		return uc->get_stream().write(tmp) != -1;
	}

	// 閫氱煡瀹㈡埛绔啓鍗忕▼锛屽叾琚涪鍑�
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

	// 鐧诲叆鏈嶅姟鍣�
	if (client_login(client) == false)
	{
		client->set_reading(false);
		printf("----------client_logout-------\r\n");

		// 澶辫触锛屽垯閫€鍑哄鎴风
		client_logout(client);

		printf("----__nreader: %d-----\r\n", --__nreader);
		return;
	}

	// 鐧诲叆鎴愬姛锛屽垯鍒涘缓鍐欏崗绋嬬敤鏉ュ悜瀹㈡埛绔彂閫佹秷鎭�
	go_stack(STACK_SIZE) [&] {
		__nwriter++;
		fiber_writer(client);
	};

	conn.set_rw_timeout(0);

	bool stop = false;
	acl::string buf;

	// 浠庡鎴风寰幆璇诲彇娑堟伅
	while (true)
	{
		bool ret = conn.gets(buf);
		if (ret == false)
		{
			printf("%s(%d): user: %s, gets error %s, fiber: %d\r\n",
				__FUNCTION__, __LINE__, client->get_name(),
				acl::last_serror(), acl_fiber_self());

			// 瀹㈡埛绔€€鍑�
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

		// 鍒嗘瀽瀹㈡埛绔彂閫佺殑娑堟伅锛屼氦鐢变笉鍚岀殑澶勭悊杩囩▼
		std::vector<acl::string>& tokens = buf.split2("|");

		// 鏈鎴风瑕佹眰閫€鍑�
		if (tokens[0] == "quit" || tokens[0] == "exit")
		{
			conn.write("Bye!\r\n");
			break;
		}

		// 鏈鎴风鍙戦€佽亰澶╂秷鎭�
		else if (tokens[0] == "chat")
		{
			if (client_chat(client, tokens) == false)
				break;
		}

		// 鏈鎴风韪㈠嚭鍏跺畠瀹㈡埛绔�
		else if (tokens[0] == "kick")
		{
			if (client_kick(client, tokens) == false)
				break;
		}

		// 瑕佹眰鏁翠釜鏈嶅姟杩涚▼閫€鍑�
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

	// 閫€鍑哄鎴风
	client_logout(client);

	printf("----__nreader: %d-----\r\n", --__nreader);

	if (stop)
	{
		int dumy = 1;
		// 濡傛灉瑕佸仠姝㈡湇鍔★紝鍒欓€氱煡鐩戞帶鍗忕▼
		__chan_monitor.put(dumy);
	}
}

static int __nclients = 0;

static void fiber_client(acl::socket_stream* conn)
{
	// 鍒涘缓瀹㈡埛绔璞�
	user_client* client = new user_client(*conn);

	// 鍒涘缓浠庡鎴风杩炴帴璇诲彇鏁版嵁鐨勫崗绋�
	go_stack(STACK_SIZE) [=] {
		__nreader++;
		fiber_reader(client);
	};

	// 绛夊緟璇ュ鎴风杩炴帴瀵硅薄閫€鍑�
	client->wait_exit();

	printf("----- client (%s), exit now -----\r\n", client->get_name());

	// 鍒犻櫎瀹㈡埛绔璞″強瀹㈡埛绔繛鎺ュ璞�
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
		// 绛夊緟鎺ユ敹瀹㈡埛绔繛鎺�
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL)
		{
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		// 鍒涘缓澶勭悊瀹㈡埛绔璞＄殑鍗忕▼
		go_stack(STACK_SIZE) [=] {
			__nclients++;
			fiber_client(conn);
		};
	}
}

static void fiber_monitor(void)
{
	int n;

	// 绛夊緟娑堟伅閫氱煡
	__chan_monitor.pop(n);

	printf("--- kill fiber_accept ---\r\n");
	// 鏉€姝荤洃鍚崗绋�
	acl_fiber_kill(__fiber_accept);

	printf("--- stop fiber schedule ---\r\n");
	// 鍋滄鍗忕▼璋冨害杩囩▼
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

	// 鍏佽 acl 搴撶殑鏃ュ織杈撳嚭鑷冲睆骞�
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

	acl::server_socket ss;	// 鐩戝惉濂楁帴鍙ｅ璞�
	// 鐩戝惉鎸囧畾鍦板潃
	if (ss.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr, acl::last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	// 鍒涘缓鏈嶅姟鍣ㄦ帴鏀惰繛鎺ュ崗绋�
	go[&] {
		fiber_accept(ss);
	};

	// 鍒涘缓鐩戞帶鍗忕▼
	go[] {
		fiber_monitor();
	};

	// 鍚姩鍗忕▼璋冨害杩囩▼
	acl::fiber::schedule();

	return 0;
}
