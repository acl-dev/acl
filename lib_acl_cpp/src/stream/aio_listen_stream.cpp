#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/aio_listen_stream.hpp"
#endif

namespace acl
{

aio_listen_stream::aio_listen_stream(aio_handle *handle)
	: aio_stream(handle)
	, accept_hooked_(false)
{
	addr_[0] = 0;
}

aio_listen_stream::~aio_listen_stream()
{
	accept_callbacks_.clear();
}

void aio_listen_stream::destroy()
{
	delete this;
}

void aio_listen_stream::add_accept_callback(aio_accept_callback* callback)
{
	std::list<aio_accept_callback*>::iterator it =
		accept_callbacks_.begin();
	for (; it != accept_callbacks_.end(); ++it)
	{
		if (*it == callback)
			return;
	}
	accept_callbacks_.push_back(callback);
}

bool aio_listen_stream::open(const char* addr)
{
	ACL_VSTREAM *sstream = acl_vstream_listen(addr, 128);
	if (sstream == NULL)
		return false;

	safe_snprintf(addr_, sizeof(addr_), "%s", ACL_VSTREAM_LOCAL(sstream));

	stream_ = acl_aio_open(handle_->get_handle(), sstream);

	// 调用基类的 hook_error 以向 handle 中增加异步流计数,
	// 同时 hook 关闭及超时回调过程
	hook_error();

	// hook 监听的回调过程
	hook_accept();
	return true;
}

const char* aio_listen_stream::get_addr() const
{
	return addr_;
}

void aio_listen_stream::hook_accept()
{
	acl_assert(stream_);
	if (accept_hooked_)
		return;
	accept_hooked_ = true;

	acl_aio_ctl(stream_,
		ACL_AIO_CTL_ACCEPT_FN, accept_callback,
		ACL_AIO_CTL_CTX, this,
		ACL_AIO_CTL_END);
	acl_aio_accept(stream_);
}

int aio_listen_stream::accept_callback(ACL_ASTREAM* stream, void* ctx)
{
	aio_listen_stream* as = (aio_listen_stream*) ctx;
	std::list<aio_accept_callback*>::iterator it =
		as->accept_callbacks_.begin();
	aio_socket_stream* ss = NEW aio_socket_stream(as->handle_,
			stream, true);

	for (; it != as->accept_callbacks_.end(); ++it)
	{
		if ((*it)->accept_callback(ss) == false)
			return -1;
	}
	return 0;
}

}  // namespace acl
