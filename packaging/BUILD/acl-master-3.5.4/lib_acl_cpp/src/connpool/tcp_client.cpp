#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/connpool/tcp_sender.hpp"
#include "acl_cpp/connpool/tcp_reader.hpp"
#include "acl_cpp/connpool/tcp_client.hpp"
#endif

namespace acl
{

tcp_client::tcp_client(const char* addr, int conn_timeout, int rw_timeout)
: conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{
	addr_   = acl_mystrdup(addr);
	conn_   = NEW socket_stream;
	sender_ = NEW tcp_sender(*conn_);
	reader_ = NULL;
}

tcp_client::~tcp_client(void)
{
	delete reader_;
	delete sender_;
	delete conn_;
	acl_myfree(addr_);
}

bool tcp_client::open(void)
{
	if (!conn_->open(addr_, conn_timeout_, rw_timeout_)) {
		logger_error("connnect %s error %s", addr_, last_serror());
		return false;
	}

	return true;
}

bool tcp_client::try_open(bool* reuse_conn)
{
	if (conn_->opened()) {
		*reuse_conn = true;
		return true;
	}

	*reuse_conn = false;
	if (!conn_->open(addr_, conn_timeout_, rw_timeout_)) {
		logger_error("connect %s error %s", addr_, last_serror());
		return false;
	}

	return true;
}

bool tcp_client::send(const void* data, unsigned int len, string* out /* = NULL */)
{
	bool have_retried = false, reuse_conn;

	while (true) {
		if (!try_open(&reuse_conn)) {
			logger_error("connect server error");
			return false;
		}

		if (!sender_->send(data, len)) {
			if (have_retried || !reuse_conn) {
				logger_error("send head error");
				return false;
			}

			have_retried = true;
			continue;
		}

		if (out == NULL) {
			return true;
		}

		if (reader_ == NULL) {
			reader_ = NEW tcp_reader(*conn_);
		}
		if (reader_->read(*out)) {
			return true;
		}
		if (have_retried || !reuse_conn) {
			logger_error("read data error");
			return false;
		}

		have_retried = true;
	}
}

} // namespace acl
