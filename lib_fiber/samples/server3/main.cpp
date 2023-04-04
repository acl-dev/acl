#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

static bool __use_c_api = false;

static void handle_client(acl::socket_stream& conn) {
	char buf[8192];

	while (true) {
		int ret = conn.read(buf, sizeof(buf), false);
		if (ret == -1) {
			break;
		}

		if (conn.write(buf, ret) == -1) {
			break;
		}
	}
}

class fiber_client : public acl::fiber {
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override
	void run(void) {
		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());
		handle_client(*conn_);
		delete this;
	}

private:
	acl::socket_stream* conn_;

	~fiber_client(void) {
		delete conn_;
	}
};

static void test_file(void) {
	const char* path = "./Makefile";
	acl::string buf;
	acl::ifstream in;
	if (in.open_read(path) == false) {
		printf("open file %s error %s\r\n", path, acl::last_serror());
	} else if (in.gets(buf) == false) {
		printf("gets from %s error %s", path, acl::last_serror());
	} else {
		printf("gets ok: %s\r\n", buf.c_str());
	}
}

void fiber_main(ACL_FIBER*, void* ctx) {
	acl::socket_stream* conn = (acl::socket_stream*) ctx;
	handle_client(*conn);
	delete conn;
}

static void create_client_fiber(acl::socket_stream* conn, bool stack_share) {
	if (__use_c_api) {
		ACL_FIBER_ATTR attr;
		acl_fiber_attr_init(&attr);

		if (stack_share) {
			acl_fiber_attr_setsharestack(&attr, 1);
			acl_fiber_attr_setstacksize(&attr, 8000);
		} else {
			acl_fiber_attr_setstacksize(&attr, 32000);
		}

		acl_fiber_create2(&attr, fiber_main, conn);
	} else {
		// create one fiber for one connection
		fiber_client* fc = new fiber_client(conn);

		// start the client fiber
		fc->start(stack_share ? 8000 : 32000, stack_share);
	}
}

static void wait_connect(acl::server_socket& ss, bool stack_share) {
	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		printf("accept ok, fd: %d\r\n", conn->sock_handle());
		create_client_fiber(conn, stack_share);
	}
}

static bool __stack_share = false;

void fiber_server_main(ACL_FIBER*, void* ctx) {
	acl::server_socket* ss = (acl::server_socket*) ctx;
	wait_connect(*ss, __stack_share);
}

class fiber_server : public acl::fiber {
public:
	fiber_server(acl::server_socket& ss, bool stack_share)
	: ss_(ss), stack_share_(stack_share) {}
	~fiber_server(void) {}

protected:
	// @override
	void run(void) {
		test_file();
		wait_connect(ss_, stack_share_);
	}

private:
	acl::server_socket& ss_;
	bool stack_share_;
};

class thread_server : public acl::thread {
public:
	thread_server(acl::server_socket& ss, bool stack_share)
	: ss_(ss), stack_share_(stack_share)  {}
	~thread_server(void) {}

protected:
	void* run(void) {
		if (__use_c_api) {
			__stack_share = stack_share_;

			ACL_FIBER_ATTR attr;
			acl_fiber_attr_init(&attr);

			// xxx: 监听协程采用共享栈模式时,valgrind 会报警告?
			if (stack_share_) {
				acl_fiber_attr_setsharestack(&attr, 1);
				acl_fiber_attr_setstacksize(&attr, 8000);
			} else {
				acl_fiber_attr_setstacksize(&attr, 32000);
			}

			acl_fiber_create2(&attr, fiber_server_main, &ss_);

			acl::fiber::schedule();	// start fiber schedule
		} else {
			fiber_server* fs = new fiber_server(ss_, stack_share_);

			// start listen fiber
			stack_share_ = false;
			fs->start(stack_share_ ? 32000 : 32000, stack_share_);
			acl::fiber::schedule();	// start fiber schedule
			delete fs;
		}

		return NULL;
	}

private:
	acl::server_socket& ss_;
	bool stack_share_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -s listen_addr\r\n"
		" -t nthreads\r\n"
		" -S [if use stack sharing]\r\n"
		" -C [if use c fiber API]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, nthreads = 1;
	bool stack_share = false;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:t:SC")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 't':
			nthreads = atoi(optarg);
			break;
		case 'S':
			stack_share = true;
			break;
		case 'C':
			__use_c_api = true;
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

	std::vector<acl::thread*> threads;

	for (int i = 0; i < nthreads; i++) {
		acl::thread* thread = new thread_server(ss, stack_share);
		thread->start();
		threads.push_back(thread);
	}

	for (std::vector<acl::thread*>::iterator it = threads.begin();
		it != threads.end(); ++it) {
		(*it)->wait(NULL);
		delete *it;
	}

	return 0;
}
