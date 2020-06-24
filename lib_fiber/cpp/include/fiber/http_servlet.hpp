#pragma once

#include <map>
#include <string>
#include "detail/http_servlet_impl.hpp"

namespace acl {

class http_servlet : public http_servlet_impl {
public:
	http_servlet(std::map<acl::string, handler_t>& handlers,
		socket_stream* stream, session* session)
	: http_servlet_impl(handlers, stream, session) {}

	~http_servlet(void) {}
};

} // namespace acl
