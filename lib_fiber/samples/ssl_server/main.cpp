#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#define	 STACK_SIZE	32000

static int __rw_timeout = 0;
static acl::string __ssl_crt("../ssl_crt.pem");
static acl::string __ssl_key("../ssl_key.pem");
static acl::polarssl_conf *__ssl_conf = NULL;
static bool __check_ssl = false;

static void http_server(ACL_FIBER *, void *ctx)
{
	acl::socket_stream *conn = (acl::socket_stream *) ctx;

	printf("start one server\r\n");

	if (__ssl_conf != NULL)
	{
		acl::polarssl_io* ssl =
			new acl::polarssl_io(*__ssl_conf, true, false);

		if (conn->setup_hook(ssl) == ssl)
		{
			printf("setup_hook error\r\n");
			ssl->destroy();
			delete conn;
			return;
		}

		if (__check_ssl)
		{
			if (ssl->handshake() == false)
			{
				printf("ssl handshake error\r\n");
				ssl->destroy();
				delete conn;
				return;
			}
			if (ssl->handshake_ok() == false)
			{
				printf("ssl handshake error\r\n");
				ssl->destroy();
				delete conn;
				return;
			}
		}
	}

	printf("ssl handshake_ok\r\n");

	acl::string buf;

	while (true)
	{
		if (conn->gets(buf, false) == false)
		{
			printf("gets error: %s\r\n", acl::last_serror());
			break;
		}

		if (conn->write(buf) == -1)
		{
			printf("write error: %s\r\n", acl::last_serror());
			break;
		}
	}

	printf("close one connection: %d, %s\r\n", conn->sock_handle(),
		acl::last_serror());
	delete conn;
}

static acl::polarssl_conf* ssl_init(const acl::string& crt,
	const acl::string& key)
{

	acl::polarssl_conf* conf = new acl::polarssl_conf;
	conf->enable_cache(1);

	if (conf->add_cert(crt) == false)
	{
		printf("load %s error\r\n", crt.c_str());
		delete conf;
		return NULL;
	}

	if (conf->set_key(key) == false)
	{
		printf("set_key %s error\r\n", key.c_str());
		delete conf;
		return NULL;
	}

	printf(">>> ssl_init ok, ssl_crt: %s, ssl_key: %s\r\n",
		crt.c_str(), key.c_str());

	return conf;
}

static void fiber_accept(ACL_FIBER *, void *ctx)
{
	const char* addr = (const char* ) ctx;
	acl::server_socket server;

	__ssl_conf = ssl_init(__ssl_crt, __ssl_key);

	if (server.open(addr) == false)
	{
		printf("open %s error\r\n", addr);
		exit (1);
	}
	else
		printf(">>> listen %s ok\r\n", addr);

	while (true)
	{
		acl::socket_stream* client = server.accept();
		if (client == NULL)
		{
			printf("accept failed: %s\r\n", acl::last_serror());
			break;
		}

		client->set_rw_timeout(__rw_timeout);
		printf("accept one: %d\r\n", client->sock_handle());
		acl_fiber_create(http_server, client, STACK_SIZE);
	}

	delete __ssl_conf;
	exit (0);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -r rw_timeout\r\n"
		" -c ssl_crt.pem\r\n"
		" -C [if check ssl status]\r\n"
		" -k ssl_key.pem\r\n", procname);
}

int main(int argc, char *argv[])
{
	acl::string addr(":9001");
	int  ch;

	while ((ch = getopt(argc, argv, "hs:r:c:k:C")) > 0)
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
		case 'c':
			__ssl_crt = optarg;
			break;
		case 'k':
			__ssl_key = optarg;
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

	acl_fiber_create(fiber_accept, addr.c_str(), STACK_SIZE);

	acl_fiber_schedule();

	return 0;
}
