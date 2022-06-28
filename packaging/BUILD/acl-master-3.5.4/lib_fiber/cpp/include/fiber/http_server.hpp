#pragma once

#include <string>
#include "detail/http_server_impl.hpp"

namespace acl {

class http_server : public http_server_impl {
public:
	http_server(const char* addr = "127.0.0.1|6379", bool use_redis = true)
	: http_server_impl(addr, use_redis) {}
	~http_server(void) {}

public:
	http_server& Get(const char* path, http_handler_t fn) {
		this->Service(http_handler_get, path, fn);
		return *this;
	}

	http_server& Post(const char* path, http_handler_t fn) {
		this->Service(http_handler_post, path, fn);
		return *this;
	}

	http_server& Head(const char* path, http_handler_t fn) {
		this->Service(http_handler_head, path, fn);
		return *this;
	}

	http_server& Put(const char* path, http_handler_t fn) {
		this->Service(http_handler_put, path, fn);
		return *this;
	}

	http_server& Patch(const char* path, http_handler_t fn) {
		this->Service(http_handler_patch, path, fn);
		return *this;
	}

	http_server& Connect(const char* path, http_handler_t fn) {
		this->Service(http_handler_connect, path, fn);
		return *this;
	}

	http_server& Purge(const char* path, http_handler_t fn) {
		this->Service(http_handler_purge, path, fn);
		return *this;
	}

	http_server& Delete(const char* path, http_handler_t fn) {
		this->Service(http_handler_delete, path, fn);
		return *this;
	}

	http_server& Options(const char* path, http_handler_t fn) {
		this->Service(http_handler_options, path, fn);
		return *this;
	}

	http_server& Propfind(const char* path, http_handler_t fn) {
		this->Service(http_handler_profind, path, fn);
		return *this;
	}

	http_server& Websocket(const char* path, http_handler_t fn) {
		this->Service(http_handler_websocket, path, fn);
		return *this;
	}

	http_server& Unknown(const char* path, http_handler_t fn) {
		this->Service(http_handler_unknown, path, fn);
		return *this;
	}

	http_server& Error(const char* path, http_handler_t fn) {
		this->Service(http_handler_error, path, fn);
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

	http_server& on_thread_accept(thread_accept_t fn) {
		this->thread_accept_ = fn;
		return *this;
	}
};

} // namespace acl
