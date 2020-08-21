#pragma once

#include "../master_fiber.hpp"
#include "../http_servlet.hpp"
#include "../go_fiber.hpp"

namespace acl {

typedef std::function<void()> proc_jail_t;
typedef std::function<void()> proc_init_t;
typedef std::function<void()> proc_exit_t;
typedef std::function<bool(acl::string&)> proc_sighup_t;
typedef std::function<void()> thread_init_t;
typedef std::function<bool(acl::socket_stream&)> thread_accept_t;

class http_server_impl : public master_fiber {
public:
	http_server_impl(const char* addr, bool use_redis) {
		if (use_redis) {
			redis_ = new redis_client_cluster;
			redis_->init(NULL, addr, 0, 10, 10);
			redis_->bind_thread(true);
		}
	}

	virtual ~http_server_impl(void) {}

protected:
	void Service(int type, const char* path, http_handler_t fn) {
		if (type >= http_handler_get && type < http_handler_max
				&& path && *path) {

			// The path should lookup like as "/xxx/" with
			// lower charactors.

			acl::string buf(path);
			if (buf[buf.size() - 1] != '/') {
				buf += '/';
			}
			buf.lower();
			handlers_[type][buf] = std::move(fn);
		}
	}

protected:
	redis_client_cluster* redis_   = nullptr;
	proc_jail_t     proc_jail_     = nullptr;
	proc_init_t     proc_init_     = nullptr;
	proc_exit_t     proc_exit_     = nullptr;
	proc_sighup_t   proc_sighup_   = nullptr;
	thread_init_t   thread_init_   = nullptr;
	thread_accept_t thread_accept_ = nullptr;
	http_handlers_t handlers_[http_handler_max];

	// @override
	void on_accept(socket_stream& conn) {
		if (thread_accept_ && !thread_accept_(conn)) {
			return;
		}

		acl::session* session;
		if (redis_) {
			session = new redis_session(*redis_, 0);
		} else {
			session = new memcache_session("127.0.0.1|11211");
		}

		http_servlet servlet(handlers_, &conn, session);
		servlet.setLocalCharset("utf-8");

		while (servlet.doRun()) {}

		delete session;
	}

	// @override
	void proc_pre_jail(void) {
		if (proc_jail_) {
			proc_jail_();
		}
	}

	// @override
	void proc_on_init(void) {
		if (proc_init_) {
			proc_init_();
		}
	}

	// @override
	void proc_on_exit(void) {
		if (proc_exit_) {
			proc_exit_();
		}
	}

	// @override
	bool proc_on_sighup(acl::string& s) {
		if (proc_sighup_) {
			return proc_sighup_(s);
		}
		return true;
	}

	// @override
	void thread_on_init(void) {
		if (thread_init_) {
			thread_init_();
		}
	}
};

} // namespace acl
