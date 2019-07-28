#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/connpool/tcp_reader.hpp"
#endif

namespace acl
{

tcp_reader::tcp_reader(socket_stream& conn)
: conn_(&conn)
{
}

bool tcp_reader::read(string& out)
{
	int len;
	if (!conn_->read(len)) {
		//logger_error("read head error!");
		return false;
	}

	len = ntohl(len);
	if (len <= 0) {
		logger_error("invalid len=%d", len);
		return false;
	}

	if (!conn_->read(out, (size_t) len)) {
		logger_error("read body error %s", last_serror());
		return false;
	}

	return true;
}

} // namespace acl
