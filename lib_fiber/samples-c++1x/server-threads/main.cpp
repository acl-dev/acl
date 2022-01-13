#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>

//////////////////////////////////////////////////////////////////////////////

static void client_echo(acl::socket_stream* conn) {
	bool readable = false;
	acl::string buf;

	printf("thread-%lu, fiber-%d, client: %s\r\n", acl::thread::self(),
		acl::fiber::self(), conn->get_peer(true));

	while (true) {
		if (readable) {
			struct timeval begin, end;
			gettimeofday(&begin, NULL);
			int ret = acl_readable(conn->sock_handle());
			gettimeofday(&end, NULL);
			double cost = acl::stamp_sub(end, begin);

			if (ret == 0) {
				printf("not readable, cost=%.2f\r\n", cost);
			} else if (ret == 1) {
				printf("readable, cost=%.2f\r\n", cost);
			} else {
				printf("readable error\r\n");
			}
		}

		if (!conn->gets(buf, false)) {
			printf("thread-%lu, fiber-%d: client read error %s\r\n",
				acl::thread::self(), acl::fiber::self(), acl::last_serror());
			break;
		}
		if (conn->write(buf) == -1) {
			printf("client write error %s\r\n", acl::last_serror());
			break;
		}
	}

	delete conn;
}

//////////////////////////////////////////////////////////////////////////////

static void box_fiber(acl::fiber_tbox<acl::socket_stream>& box) {
	while (true) {
		// popup one connection from the box
		acl::socket_stream* conn = box.pop();
		if (conn == NULL) {
			printf("pop NULL and exit thread now.\r\n");
			break;
		}

		// start one fiber to handle the connection.
		go[=] {
			client_echo(conn);
		};
	}
}

class fiber_thread : public acl::thread {
public:
	fiber_thread(acl::fiber_event_t type)
	: type_(type)
	{}

	~fiber_thread(void) {}

	// push one connection to the fiber thread
	void push(acl::socket_stream* conn) {
		box_.push(conn);
	}

protected:
	// @override
	void* run(void) {
		// start one fiber to wait for connections
		go[&] {
			box_fiber(std::ref(box_));
		};

		// start the fiber schedule process
		acl::fiber::schedule_with(type_);
		return NULL;
	}

private:
	acl::fiber_event_t type_;
	acl::fiber_tbox<acl::socket_stream> box_;
};

static void start_threads(acl::fiber_event_t type,
		const char* addr, int nthreads) {

	std::vector<fiber_thread*> threads;

	for (int i = 0; i < nthreads; i++) {
		fiber_thread* thread = new fiber_thread(type);
		threads.push_back(thread);
		thread->start();
	}

	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("open %s error %s\r\n", addr, acl::last_serror());
		return;
	}

	printf("Listen on %s ...\r\n", addr);

	size_t i = 0, n = (size_t) nthreads;

	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		// peek one thread and push the connection to it
		fiber_thread* thread = threads[i++ % n];
		thread->push(conn);
	}

	for (std::vector<fiber_thread*>::iterator it = threads.begin();
		 it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}
}

//////////////////////////////////////////////////////////////////////////////

#if !defined(_WIN32) && !defined(_WIN64)

static void listen_fiber(const char* addr) {
	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen on %s error %s\r\n", addr, acl::last_serror());
		return;
	}

	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}

		// start one fiber to handle the connection.
		go[=]{
			client_echo(conn);
		};
	}
}

class fiber_thread2 : public acl::thread {
public:
	fiber_thread2(acl::fiber_event_t type, const char* addr)
	: type_(type)
	, addr_(addr)
	{}

	~fiber_thread2(void) {}

protected:
	// @override
	void* run(void) {
		go[&] {
			listen_fiber(addr_.c_str());
		};

		// start the fiber schedule process
		acl::fiber::schedule_with(type_);
		return NULL;
	}

private:
	acl::fiber_event_t type_;
	acl::string addr_;
};

static void start_threads2(acl::fiber_event_t type,
		const char* addr, int nthreads) {

	std::vector<fiber_thread2*> threads;

	for (int i = 0; i < nthreads; i++) {
		fiber_thread2* thread = new fiber_thread2(type, addr);
		threads.push_back(thread);
		thread->start();
	}

	for (std::vector<fiber_thread2*>::iterator it = threads.begin();
		 it != threads.end(); ++it) {
		(*it)->wait();
		delete *it;
	}
}

#endif // !_WIN32 && !_WIN64

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help]\r\n"
		" -e event_type[kernel|select|poll]\r\n"
		" -s server_addr\r\n"
		" -t threads_count\r\n"
		" -T [if listening in multiple threads]\r\n"
		, procname);
}

int main(int argc, char *argv[]) {
	int  ch, nthreads = 1;
	bool multi_listen = false;
	acl::string addr = "0.0.0.0:9000";
	acl::string event_type("kernel");

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	while ((ch = getopt(argc, argv, "hs:e:t:T")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'e':
			event_type = optarg;
			break;
		case 't':
			nthreads = atoi(optarg);
			if (nthreads <= 0) {
				nthreads = 1;
			}
			break;
		case 'T':
			multi_listen = true;
			break;
		default:
			break;
		}
	}

	acl::fiber_event_t type;
	if (event_type == "select") {
		type = acl::FIBER_EVENT_T_SELECT;
	} else if (event_type == "poll") {
		type = acl::FIBER_EVENT_T_POLL;
	} else {
		type = acl::FIBER_EVENT_T_KERNEL;
	}

#if !defined(_WIN32) && !defined(_WIN64)
	if (multi_listen) {
		start_threads2(type, addr, nthreads);
	} else {
		start_threads(type, addr, nthreads);
	}
#else
	start_threads(type, addr, nthreads);
#endif

	return 0;
}
