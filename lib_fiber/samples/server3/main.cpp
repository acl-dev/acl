#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

class fiber_client : public acl::fiber {
public:
	fiber_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	// @override
	void run(void) {
		printf("fiber-%d-%d running\r\n", get_id(), acl::fiber::self());

		char buf[8192];
		while (true) {
			int ret = conn_->read(buf, sizeof(buf), false);
			if (ret == -1) {
				break;
			}

			if (conn_->write(buf, ret) == -1) {
				break;
			}
		}

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

class fiber_server : public acl::fiber {
public:
	fiber_server(acl::server_socket& ss, bool stack_share)
	: ss_(ss), stack_share_(stack_share) {}
	~fiber_server(void) {}

protected:
	// @override
	void run(void) {
		test_file();

		while (true) {
			acl::socket_stream* conn = ss_.accept();
			if (conn == NULL) {
				printf("accept error %s\r\n", acl::last_serror());
				break;
			}

			printf("accept ok, fd: %d\r\n", conn->sock_handle());
			// create one fiber for one connection
			fiber_client* fc = new fiber_client(conn);
			// start the client fiber
			fc->start(stack_share_ ? 8000 : 32000, stack_share_);
		}
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
		fiber_server fs(ss_, stack_share_);

		// start listen fiber
		fs.start(stack_share_ ? 8000 : 32000, stack_share_);

		acl::fiber::schedule();	// start fiber schedule
		return NULL;
	}

private:
	acl::server_socket& ss_;
	bool stack_share_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s listen_addr -t nthreads -S [if use stack sharing]\r\n", procname);
}

int main(int argc, char *argv[]) {
	int  ch, nthreads = 1;
	bool stack_share = false;

	acl::acl_cpp_init();
	acl::string addr("127.0.0.1:9006");
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:t:S")) > 0) {
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
