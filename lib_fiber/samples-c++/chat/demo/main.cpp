#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "fiber/lib_fiber.h"
#include "user_client.h"

#define	STACK_SIZE	128000

static void client_read_callback(int type, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *ctx);

static int __rw_timeout = 0;
static int __stop       = 0;
static int __use_kernel = 0;

static std::map<acl::string, user_client*> __users;

static void remove_user(user_client* uc)
{
	const char* name = uc->get_name();
	if (name == NULL || *name == 0)
	{
		printf("no name!\r\n");
		return;
	}

	std::map<acl::string, user_client*>::iterator it = __users.find(name);
	if (it == __users.end())
		printf("not exist, name: %s\r\n", name);
	else
	{
		__users.erase(it);
		printf("delete user ok, name: %s\r\n", name);
	}
}

static bool client_login(user_client* uc, ACL_ARGV* tokens)
{
	if (tokens->argc < 2)
	{
		acl::string tmp;
		tmp.format("invalid argc: %d < 2\r\n", tokens->argc);
		printf("%s", tmp.c_str());

		return acl_vstream_writen(uc->get_stream(), tmp.c_str(),
			(int) tmp.size()) != ACL_VSTREAM_EOF;
	}

	acl::string msg;

	const char* name = tokens->argv[1];
	std::map<acl::string, user_client*>::iterator it = __users.find(name);
	if (it == __users.end())
	{
		__users[name] = uc;
		uc->set_name(name);
		msg.format("user %s login ok\r\n", name);
	}
	else
		msg.format("user %s already login\r\n", name);

	printf("%s", msg.c_str());

	return acl_vstream_writen(uc->get_stream(), msg.c_str(),
			(int) msg.size()) != ACL_VSTREAM_EOF;
}

static bool client_chat(user_client* uc, ACL_ARGV* tokens)
{
	if (tokens->argc < 3)
	{
		acl::string tmp;
		tmp.format("invalid argc: %d < 3\r\n", tokens->argc);
		printf("%s", tmp.c_str());

		return acl_vstream_writen(uc->get_stream(), tmp.c_str(),
			(int) tmp.size()) != ACL_VSTREAM_EOF;
	}

	const char* to = tokens->argv[1];
	const char* msg = tokens->argv[2];

	std::map<acl::string, user_client*>::iterator it = __users.find(to);
	if (it == __users.end())
	{
		acl::string tmp;
		tmp.format("from user: %s, to user: %s not exist\r\n",
			uc->get_name(), to);
		printf("%s", tmp.c_str());

		return acl_vstream_writen(uc->get_stream(), tmp.c_str(),
			(int) tmp.size()) != ACL_VSTREAM_EOF;
	}
	else
	{
		it->second->push(msg);
		return true;
	}
}

static void fiber_client_read(ACL_FIBER *fiber acl_unused, void *ctx)
{
	user_client* uc = (user_client*) ctx;
	ACL_VSTREAM *conn = uc->get_stream();
	ACL_EVENT *event = uc->get_event();
	char  buf[8192];

	int ret = acl_vstream_gets_nonl(conn, buf, sizeof(buf) - 1);
	printf(">>>ret: %d\r\n", ret);
	if (ret == ACL_VSTREAM_EOF)
	{
		if (uc->already_login())
			remove_user(uc);
		acl_vstream_close(conn);
		delete uc;
		return;
	}
	buf[ret] = 0;

	printf("gets: %s\r\n", buf);

	bool status;
	ACL_ARGV* tokens = acl_argv_split(buf, "|");

	if (strcasecmp(tokens->argv[0], "login") == 0)
	{
		if (uc->already_login())
		{
			status = true;
			printf("user: %s already logined\r\n", uc->get_name());
		}
		else
			status = client_login(uc, tokens);
	}
	else if (!uc->already_login())
	{
		acl::string tmp;
		tmp.format("you're not login yet!\r\n");
		printf("%s", tmp.c_str());

		status = acl_vstream_writen(conn, tmp.c_str(), (int) tmp.size())
			== ACL_VSTREAM_EOF ? false : true;
	}
	else if (strcasecmp(tokens->argv[0], "chat") == 0)
		status = client_chat(uc, tokens);
	else
	{
		acl::string tmp;
		tmp.format("invalid data: %s\r\n", buf);
		printf("%s", tmp.c_str());

		status = acl_vstream_writen(conn, tmp.c_str(), (int) tmp.size())
			== ACL_VSTREAM_EOF ? false : true;
	}

	acl_argv_free(tokens);

	if (status == false)
	{
		if (uc->already_login())
			remove_user(uc);

		acl_vstream_close(conn);
		delete uc;
		return;
	}

	acl_event_enable_read(event, conn, 120, client_read_callback, uc);
}

static void client_read_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *ctx)
{
	user_client* uc = (user_client*) ctx;

	acl_event_disable_readwrite(event, conn);
	acl_fiber_create(fiber_client_read, uc, STACK_SIZE);
}

static void listen_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *sstream, void *ctx acl_unused)
{
	char ip[64];
	ACL_VSTREAM *conn = acl_vstream_accept(sstream, ip, sizeof(ip));

	if (conn == NULL) {
		printf("accept error %s\r\n", acl_last_serror());
		return;
	}

	printf(">>>accept one, fd: %d\r\n", ACL_VSTREAM_SOCK(conn));

	user_client* uc = new user_client(event, conn);
	acl_event_enable_read(event, conn, 120, client_read_callback, uc);
}

static void fiber_flush(ACL_FIBER* fiber acl_unused, void* ctx)
{
	user_client* uc = (user_client*) ctx;
	ACL_EVENT* event = uc->get_event();
	ACL_VSTREAM* conn = uc->get_stream();
	acl::string* msg;

	uc->set_busy(true);

	while ((msg = uc->pop()) != NULL)
	{
		if (acl_vstream_writen(conn, msg->c_str(), (int) msg->size())
			== ACL_VSTREAM_EOF)
		{
			printf("flush to user: %s error\r\n", uc->get_name());
			delete msg;
			break;

			// don't delete uc here, which will be remove in
			// read callback
			//acl_vstream_close(conn);
			//delete uc;
			//return;
		}
	}

	uc->set_busy(false);
	acl_event_enable_read(event, conn, 120, client_read_callback, uc);
}

static void fflush_all(void)
{
	std::map<acl::string, user_client*>::iterator it = __users.begin();
	std::map<acl::string, user_client*>::iterator next = it;
	for (; it != __users.end(); it = next)
	{
		++next;
		if (it->second->empty() || it->second->is_busy())
			continue;

		ACL_EVENT* event = it->second->get_event();
		ACL_VSTREAM* conn = it->second->get_stream();
		acl_event_disable_readwrite(event, conn);

		acl_fiber_create(fiber_flush, it->second, STACK_SIZE);
	}
}

static void fiber_event(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;
	ACL_EVENT *event;
	time_t last_ping = time(NULL);

	if (__use_kernel)
		event = acl_event_new(ACL_EVENT_KERNEL, 0, 0, 100000);
	else
		event = acl_event_new(ACL_EVENT_POLL, 0, 0, 100000);

	printf(">>>enable read fd: %d\r\n", ACL_VSTREAM_SOCK(sstream));
	acl_event_enable_listen(event, sstream, 0, listen_callback, NULL);

	while (!__stop)
	{
		acl_event_loop(event);
		fflush_all();

		if (time(NULL) - last_ping >= 1)
		{
			//ping_all();
			last_ping = time(NULL);
		}
	}

	acl_vstream_close(sstream);
	acl_event_free(event);

	acl_fiber_schedule_stop();
}

static void usage(const char *procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n"
		" -r rw_timeout\r\n"
		" -R [use real http]\r\n"
		" -k [use kernel event]\r\n", procname);
}

int main(int argc, char *argv[])
{
	char addr[64];
	ACL_VSTREAM *sstream;
	int  ch;

	acl::log::stdout_open(true);
	snprintf(addr, sizeof(addr), "%s", "127.0.0.1:9002");

	while ((ch = getopt(argc, argv, "hs:r:k")) > 0) {
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
		case 'k':
			__use_kernel = 1;
			break;
		default:
			break;
		}
	}

	sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	printf("listen %s ok\r\n", addr);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	acl_fiber_create(fiber_event, sstream, STACK_SIZE);

	printf("call fiber_schedule\r\n");
	acl_fiber_schedule();

	return 0;
}
