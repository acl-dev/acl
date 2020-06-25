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

class http_server_impl : public master_fiber {
public:
	http_server_impl(void) {}
	virtual ~http_server_impl(void) {}

public:
	http_server_impl& service(const std::string& path, handler_t fn) {
		return service(path.c_str(), fn);
	}

	http_server_impl& service(const char* path, handler_t fn) {
		if (path && *path) {
			acl::string buf(path);
			if (buf[buf.size() - 1] != '/') {
				buf += '/';
			}
			buf.lower();
			handlers_[buf] = std::move(fn);
		}
		return *this;
	}

protected:
	std::map<acl::string, handler_t> handlers_;
	proc_jail_t proc_jail_ = nullptr;
	proc_init_t proc_init_ = nullptr;
	proc_exit_t proc_exit_ = nullptr;
	proc_sighup_t proc_sighup_ = nullptr;
	thread_init_t thread_init_ = nullptr;

	// @override
	void on_accept(socket_stream& conn) {
		memcache_session session("127.0.0.1:11211");
		http_servlet servlet(handlers_, &conn, &session);
		servlet.setLocalCharset("gb2312");

		while (servlet.doRun()) {}
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
