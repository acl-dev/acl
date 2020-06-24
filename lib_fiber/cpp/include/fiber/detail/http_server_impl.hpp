#pragma once

#include <map>
#include <cstdio>
#include <string>
#include <functional>
#include <thread>
#include <vector>
#include "../http_servlet.hpp"
#include "../go_fiber.hpp"

namespace acl {

class http_server_impl {
public:
	http_server_impl(const char* addr) : addr_(addr) {}
	virtual ~http_server_impl(void) {}

	void run(int nthreads) {
#if !defined(SO_REUSEPORT)
		server_socket* ss = new server_socket;
		if (!ss->open(addr_.c_str())) {
			printf("open %s error\r\n", addr_.c_str());
			delete ss;
			return;
		}
		printf("listen %s ok\r\n", addr_.c_str());
#endif

		std::vector<std::thread*> threads;
		std::thread* thread;
		for (int i = 0; i < nthreads; i++) {
#if defined(SO_REUSEPORT)
			thread = new std::thread(thread_main, this);
#else
			thread = new std::thread(thread_main2, this, ss);
#endif
			threads.push_back(thread);
		}

		for (std::vector<std::thread*>::iterator it = threads.begin();
			it != threads.end(); ++it) {
			(*it)->join();
			delete *it;
		}
	}

public:
	void service(const std::string& path, handler_t fn) {
		service(path.c_str(), fn);
	}

	void service(const char* path, handler_t fn) {
		if (path && *path) {
			acl::string buf(path);
			if (buf[buf.size() - 1] != '/') {
				buf += '/';
			}
			buf.lower();
			handlers_[buf] = std::move(fn);
		}
	}

protected:
	std::string addr_;
	std::map<acl::string, handler_t> handlers_;

	static void thread_main(http_server_impl* server) {
		unsigned flag = 0;

#if defined(SO_REUSEPORT)
		flag |= OPEN_FLAG_REUSEPORT;
#endif
		server_socket* ss = new server_socket(flag, 128);
		if (!ss->open(server->addr_.c_str())) {
			printf("open %s error\r\n", server->addr_.c_str());
			delete ss;
			return;
		}

		printf("listen %s ok\r\n", server->addr_.c_str());
		thread_main2(server, ss);
	}

	static void thread_main2(http_server_impl* server, server_socket* ss) {
		server->fiber_run(ss);
	}

	void fiber_run(server_socket* ss) {
		go[=] {
			fiber_listener(ss);
		};

		fiber::schedule();
	}

	void fiber_listener(server_socket* ss) {
		while (true) {
			socket_stream* conn = ss->accept();
			if (conn == NULL) {
				break;
			}

			go[=] {
				handle_conn(conn);
			};
		}
		delete ss;
	}

	void handle_conn(socket_stream* conn) {
		memcache_session session("127.0.0.1:11211");
		http_servlet servlet(handlers_, conn, &session);
		servlet.setLocalCharset("gb2312");

		while (servlet.doRun()) {}
		delete conn;
	}
};

} // namespace acl
