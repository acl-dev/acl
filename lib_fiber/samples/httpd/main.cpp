#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include "http_servlet.h"

#define	 STACK_SIZE	320000
static int __rw_timeout = 0;
static int __schedule_event = FIBER_EVENT_KERNEL;

static void http_server(ACL_FIBER *, void *ctx)
{
	acl::socket_stream *conn = (acl::socket_stream *) ctx;

	//printf("start one http_server\r\n");

	acl::memcache_session session("127.0.0.1:11211");
	http_servlet servlet(conn, &session);
	servlet.setLocalCharset("gb2312");

	while (true)
	{
		if (servlet.doRun() == false)
			break;
	}

	printf("close one connection: %d, %s\r\n", conn->sock_handle(),
		acl::last_serror());
	delete conn;
}

static void fiber_accept(ACL_FIBER *, void *ctx)
{
	acl::server_socket* server = (acl::server_socket *) ctx;
	while (true)
	{
		acl::socket_stream* client = server->accept();
		if (client == NULL)
		{
			printf("accept failed: %s\r\n", acl::last_serror());
			break;
		}

		client->set_rw_timeout(__rw_timeout);
		printf("accept one: %d\r\n", client->sock_handle());
		acl_fiber_create(http_server, client, STACK_SIZE);
	}

	exit (1);
}

class thread_server : public acl::thread
{
public:
	thread_server(acl::server_socket& server)
	: server_inner_(NULL)
	, server_(&server)
	{
	}

	thread_server(const char* addr)
	{
		server_inner_ = new acl::server_socket(acl::OPEN_FLAG_REUSEPORT);
		if (server_inner_->open(addr) == false)
		{
			printf("open %s error %s\r\n",
				addr, acl::last_serror());
			exit (1);
		}

		server_ = server_inner_;
	}

	~thread_server(void) { delete server_inner_; }

private:
	acl::server_socket* server_inner_;
	acl::server_socket* server_;

	void* run(void)
	{
		acl_fiber_create(fiber_accept, server_, STACK_SIZE);
		acl_fiber_schedule_with(__schedule_event);
		return NULL;
	}
};

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -e event\r\n"
		" -R reuse_port\r\n"
		" -t threads\r\n"
		" -r rw_timeout\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl::string addr("127.0.0.1:9001");
	int  ch, nthreads = 2;
	bool reuse_port = false;

	while ((ch = getopt(argc, argv, "hs:r:t:Re:")) > 0)
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
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'R':
			reuse_port = true;
			break;
		case 'e':
			if (strcasecmp(optarg, "kernel") == 0)
				__schedule_event = FIBER_EVENT_KERNEL;
			else if (strcasecmp(optarg, "poll") == 0)
				__schedule_event = FIBER_EVENT_SELECT;
			else if (strcasecmp(optarg, "select") == 0)
				__schedule_event = FIBER_EVENT_POLL;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::server_socket server;

	if (!reuse_port)
	{
		if (server.open(addr) == false)
		{
			printf("open %s error\r\n", addr.c_str());
			exit (1);
		}
		else
			printf("open %s ok\r\n", addr.c_str());
	}

	std::vector<acl::thread*> threads;

	for (int i = 0; i < nthreads; i++)
	{
		acl::thread* thr;
		if (reuse_port)
			thr = new thread_server(addr);
		else
			thr = new thread_server(server);
		threads.push_back(thr);
		thr->set_detachable(false);
		thr->start();

	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it)
	{
		(*it)->wait();
		delete *it;
	}

	return 0;
}
