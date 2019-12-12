#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#endif

namespace acl
{

aio_listen_stream::aio_listen_stream(aio_handle *handle)
: aio_stream(handle)
, listen_hooked_(false)
{
	addr_[0] = 0;
}

aio_listen_stream::~aio_listen_stream(void)
{
	accept_callbacks_.clear();
}

void aio_listen_stream::destroy(void)
{
	delete this;
}

void aio_listen_stream::add_accept_callback(aio_accept_callback* callback)
{
	std::list<aio_accept_callback*>::iterator it =
		accept_callbacks_.begin();
	for (; it != accept_callbacks_.end(); ++it) {
		if (*it == callback) {
			return;
		}
	}
	accept_callbacks_.push_back(callback);
}

void aio_listen_stream::add_listen_callback(aio_listen_callback* callback)
{
	std::list<aio_listen_callback*>::iterator it =
		listen_callbacks_.begin();
	for (; it != listen_callbacks_.end(); ++it) {
		if (*it == callback) {
			return;
		}
	}
	listen_callbacks_.push_back(callback);
}

bool aio_listen_stream::open(const char* addr, unsigned flag /* = 0 */)
{
	unsigned oflag = 0;
	if (flag & OPEN_FLAG_REUSEPORT) {
		oflag |= ACL_INET_FLAG_REUSEPORT;
	}
	if (flag & OPEN_FLAG_EXCLUSIVE) {
		oflag |= ACL_INET_FLAG_EXCLUSIVE;
	}
	ACL_VSTREAM *sstream = acl_vstream_listen_ex(addr, 128, oflag, 0, 0);
	if (sstream == NULL) {
		return false;
	}

	return open(sstream);
}

bool aio_listen_stream::open(ACL_SOCKET fd)
{
	unsigned fdtype = 0;
	int type = acl_getsocktype(fd);
	switch (type) {
#ifdef ACL_UNIX
	case AF_UNIX:
		fdtype |= ACL_VSTREAM_TYPE_LISTEN_UNIX;
		break;
#endif
	case AF_INET:
#ifdef AF_INET6
	case AF_INET6:
#endif
		fdtype |= ACL_VSTREAM_TYPE_LISTEN_INET;
		break;
	default: // xxx?
		fdtype |= ACL_VSTREAM_TYPE_LISTEN_INET;
		break;
	}

	ACL_VSTREAM* vstream = acl_vstream_fdopen(fd, 0, 0, -1, fdtype);
	return open(vstream);
}

bool aio_listen_stream::open(ACL_VSTREAM* vstream)
{
	ACL_ASTREAM* astream = acl_aio_open(handle_->get_handle(), vstream);
	return open(astream);
}

bool aio_listen_stream::open(ACL_ASTREAM* astream)
{
	if (astream == NULL) {
		return false;
	}

	ACL_VSTREAM* vstream = acl_aio_vstream(astream);
	if (vstream == NULL) {
		return false;
	}
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vstream);
	if (fd == ACL_SOCKET_INVALID) {
		return false;
	}
	(void) acl_getsockname(fd, addr_, sizeof(addr_));

	stream_ = astream;

	// 调用基类的 hook_error 以向 handle 中增加异步流计数,
	// 同时 hook 关闭及超时回调过程
	hook_error();

	// hook 监听通知过程
	hook_listen();

	return true;
}

const char* aio_listen_stream::get_addr(void) const
{
	return addr_;
}

void aio_listen_stream::hook_listen(void)
{
	acl_assert(stream_);
	if (listen_hooked_) {
		return;
	}
	listen_hooked_ = true;

	acl_aio_ctl(stream_,
		ACL_AIO_CTL_LISTEN_FN, listen_callback,
		ACL_AIO_CTL_CTX, this,
		ACL_AIO_CTL_END);
	acl_aio_listen(stream_);
}

int aio_listen_stream::accept_callback(aio_socket_stream* conn)
{
	std::list<aio_accept_callback*>::iterator it = accept_callbacks_.begin();

	for (; it != accept_callbacks_.end(); ++it) {
		if (!(*it)->accept_callback(conn)) {
			return -1;
		}
	}
	return 0;
}

int aio_listen_stream::listen_callback(ACL_ASTREAM*, void* ctx)
{
	aio_listen_stream* ss = (aio_listen_stream*) ctx;

	// first, we use proactor mode.

	if (!ss->accept_callbacks_.empty()) {
		aio_socket_stream* conn = ss->accept();
		if (conn != NULL) {
			return ss->accept_callback(conn);
		}
		int ret = last_error();
		if (ret == ACL_EAGAIN || ret == ACL_ECONNABORTED) {
			return 0;
		}
		logger_error("accept error=%s", last_serror());
		return -1;
	}

	// then use reactor mode

	std::list<aio_listen_callback*>::iterator it =
		ss->listen_callbacks_.begin();
	for (; it != ss->listen_callbacks_.end(); ++it) {
		if (!(*it)->listen_callback(*ss)) {
			return -1;
		}
	}
	return 0;
}

aio_socket_stream* aio_listen_stream::accept(void)
{
	acl_assert(stream_);

	ACL_VSTREAM* ss = acl_aio_vstream(stream_);
	if (ss == NULL) {
		return NULL;
	}

	ACL_VSTREAM* cs = acl_vstream_accept(ss, NULL, 0);
	if (cs == NULL) {
		return NULL;
	}

	ACL_ASTREAM* as = acl_aio_open(handle_->get_handle(), cs);
	aio_socket_stream* conn = NEW aio_socket_stream(handle_, as, true);
	return conn;
}

}  // namespace acl
