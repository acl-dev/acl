#pragma once

#include <string>
#include "detail/http_server_impl.hpp"

namespace acl {

class http_server : public http_server_impl {
public:
	http_server(const char* addr = "127.0.0.1|6379", bool use_redis = true) : http_server_impl(addr, use_redis) {}
	~http_server(void) {}

public:
	http_server& onGet(const char* path, http_handler_t fn) {
		this->onService(http_handler_get, path, fn);
		return *this;
	}

	http_server& onPost(const char* path, http_handler_t fn) {
		this->onService(http_handler_post, path, fn);
		return *this;
	}

	http_server& onHead(const char* path, http_handler_t fn) {
		this->onService(http_handler_head, path, fn);
		return *this;
	}

	http_server& onPut(const char* path, http_handler_t fn) {
		this->onService(http_handler_put, path, fn);
		return *this;
	}

	http_server& onPatch(const char* path, http_handler_t fn) {
		this->onService(http_handler_patch, path, fn);
		return *this;
	}

	http_server& onConnect(const char* path, http_handler_t fn) {
		this->onService(http_handler_connect, path, fn);
		return *this;
	}

	http_server& onPurge(const char* path, http_handler_t fn) {
		this->onService(http_handler_purge, path, fn);
		return *this;
	}

	http_server& onDelete(const char* path, http_handler_t fn) {
		this->onService(http_handler_delete, path, fn);
		return *this;
	}

	http_server& onOptions(const char* path, http_handler_t fn) {
		this->onService(http_handler_options, path, fn);
		return *this;
	}

	http_server& onPropfind(const char* path, http_handler_t fn) {
		this->onService(http_handler_profind, path, fn);
		return *this;
	}

	http_server& onError(const char* path, http_handler_t fn) {
		this->onService(http_handler_error, path, fn);
		return *this;
	}

	http_server& onWebsocket(const char* path, http_handler_t fn) {
		this->onService(http_handler_websocket, path, fn);
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
