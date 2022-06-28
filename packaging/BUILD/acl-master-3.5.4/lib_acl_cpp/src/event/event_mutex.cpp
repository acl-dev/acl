#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/event/event_mutex.hpp"
#endif

namespace acl
{

event_mutex::event_mutex(bool recursive /* = true */)
: recursive_(recursive)
, nested_(0)
, count_(0)
, tid_(0)
{
	ACL_SOCKET fds[2];

	if (acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
		logger_fatal("socketpair error=%s", last_serror());
	}

	in_  = fds[0];
	out_ = fds[1];
//	acl_tcp_nodelay(in_, 1);
//	acl_tcp_nodelay(out_, 1);
}

event_mutex::~event_mutex(void)
{
	acl_socket_close(in_);
	acl_socket_close(out_);
}

#define	DUMMY	1

bool event_mutex::lock(void)
{
	if (++count_ == 1) {
		tid_ = thread::self();
		return true;
	}

	if (recursive_ && tid_ == thread::self()) {
		nested_++;
		count_--;
		return true;
	}

	int n, timeout = -1;

	while (true) {
#ifdef ACL_UNIX
		if (acl_read_poll_wait(in_, timeout) < 0) {
#else
		if (acl_read_select_wait(in_, timeout) < 0) {
#endif
			if (errno == ACL_ETIMEDOUT) {
				continue;
			}

			--count_;
			logger_error("read wait error=%s", last_serror());
			return false;
		}

		if (acl_socket_read(in_, &n, sizeof(n), 0, NULL, NULL) > 0) {
			break;
		}

		int e = last_error();
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (e != ACL_EINTR && e != ACL_EWOULDBLOCK) {
#else
		if (e != ACL_EINTR && e != ACL_EWOULDBLOCK && e != ACL_EAGAIN) {
#endif
			logger_error("write notify error=%s", last_serror());
			return false;
		}
	}

	if (n != DUMMY) {
		logger_error("invalid read=%d(!=%d)", n, DUMMY);
		return false;
	}

	tid_ = thread::self();
	return true;
}

bool event_mutex::unlock(void)
{
	if (tid_ != thread::self()) {
		logger_error("current thread=%lu, mutex's owner=%lu",
			(unsigned long) thread::self(), tid_);
		return false;
	}

	if (nested_ > 0) {
		nested_--;
		return true;
	}

	tid_ = 0;

	if (--count_ == 0) {
		return true;
	}

	acl_assert(count_.value() > 0);

	static int n = DUMMY;
	while (true) {
		if (acl_socket_write(out_, &n, sizeof(n), 0, NULL, NULL) > 0) {
			break;
		}

		int e = last_error();
#if ACL_EWOULDBLOCK == ACL_EAGAIN
		if (e != ACL_EINTR && e != ACL_EWOULDBLOCK) {
#else
		if (e != ACL_EINTR && e != ACL_EWOULDBLOCK && e != ACL_EAGAIN) {
#endif
			logger_error("write notify error=%s", last_serror());
			return false;
		}
	}

	return true;
}

} // namespace acl
