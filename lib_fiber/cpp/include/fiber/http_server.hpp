#pragma once

#include <string>
#include "detail/http_server_impl.hpp"

namespace acl {

class http_server : public http_server_impl {
public:
	http_server(const char* addr) : http_server_impl(addr) {}
	~http_server(void) {}

	void run(int nthreads) {
		http_server_impl::run(nthreads);
	}

public:
	void get(const std::string path, handler_t fn) {
		this->service(path, fn);
	}

	void post(const std::string& path, handler_t fn) {
		this->service(path, fn);
	}
};

} // namespace acl
