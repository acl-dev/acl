#pragma once

#include <string>
#include "detail/http_server_impl.hpp"

namespace acl {

class http_server : public http_server_impl {
public:
	http_server(void) {}
	~http_server(void) {}

public:
	http_server& get(const std::string path, handler_t fn) {
		this->service(path, fn);
		return *this;
	}

	http_server& post(const std::string& path, handler_t fn) {
		this->service(path, fn);
		return *this;
	}

public:
	http_server& before_proc_jail(proc_jail_t fn) {
		this->proc_jail_ = fn;
		return *this;
	}

	http_server& on_proc_init(proc_init_t fn) {
		this->proc_init_ = fn;
		return *this;
	}

	http_server& on_proc_exit(proc_exit_t fn) {
		this->proc_exit_ = fn;
		return *this;
	}

	http_server& on_proc_sighup(proc_sighup_t fn) {
		this->proc_sighup_ = fn;
		return *this;
	}

	http_server& on_thread_init(thread_init_t fn) {
		this->thread_init_ = fn;
		return *this;
	}
};

} // namespace acl
