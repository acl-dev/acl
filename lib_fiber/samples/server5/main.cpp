#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static bool echo(acl::socket_stream& conn)
{
	char buf[8192];
	int ret = conn.read(buf, sizeof(buf) - 1, false);
	if (ret == -1) {
		printf("read error %s\r\n", acl::last_serror());
		return false;
	} else if (conn.write(buf, ret) == -1) {
		printf("write error %s\r\n", acl::last_serror());
		return false;
	}

	buf[ret] = 0;
	if (strncmp(buf, "quit", 4) == 0) {
		printf("client wants to quit now\r\n");
		return false;
	}
	return true;
}

static void fiber_client_echo(acl::socket_stream* conn,
	acl::fiber_sem& sem, bool& finished, unsigned n)
{
//	printf("echo fiber-%d running\r\n", acl::fiber::self());

	finished = false;
	for (int i = 0; i < 10; i++) {
		if (echo(*conn) == false) {
			finished = true;
			break;
		}
	}

	n++;
	sem.post();
}

static void readable_callback(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *cstream, void *ctx acl_unused)
{
	acl_event_disable_readwrite(event, cstream);
	acl::socket_stream* conn = (acl::socket_stream*) cstream->context;
	bool finished = false;
	unsigned int n = 0;
	acl::fiber_sem sem(0);

	go_stack(64000) [&] {
		fiber_client_echo(conn, sem, finished, n);
	};

	sem.wait();
//	printf("wait over, finished=%s, %p, n=%u\r\n",
//		finished ? "yes" : "no", &finished, n);

	if (!finished) {
		acl_event_enable_read(event, cstream, 120, readable_callback, NULL);
	} else {
		bool* stop = (bool *) conn->get_ctx();
		*stop = true;

		printf("closing client\r\n");
	}
}

static void fiber_client(acl::socket_stream* conn)
{
	printf("fiber-%d running\r\n", acl::fiber::self());

	bool stop = false;
	ACL_EVENT *event = acl_event_new(ACL_EVENT_POLL, 0, 1, 0);
	ACL_VSTREAM *cstream = conn->get_vstream();
	cstream->context = conn;
	conn->set_ctx(&stop);
	acl_event_enable_read(event, cstream, 120, readable_callback, NULL);

	while (!stop) {
		acl_event_loop(event);
	}

	acl_event_free(event);
	delete conn;
	printf("delete client ok, fiber-%u will exit\r\n", acl::fiber::self());
}

static void fiber_server(acl::server_socket& ss)
{
	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		printf("accept ok, fd: %d\r\n", conn->sock_handle());

		go_stack(32000) [=] {
			fiber_client(conn);
		};
	}
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help] -s listen_addr\r\n", procname);
}

int main(int argc, char *argv[])
{
	int  ch;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::server_socket ss;
	if (ss.open(addr) == false) {
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	go[&] {
		fiber_server(ss);
	};

	acl::fiber::schedule();	// start fiber schedule

	return 0;
}
