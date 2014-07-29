#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stream/stream.hpp"

namespace acl {

stream::stream(void)
: stream_(NULL)
, eof_(true)
, opened_(false)
, default_ctx_(NULL)
{
}

stream::~stream(void)
{
	if (stream_)
		acl_vstream_free(stream_);
}

bool stream::eof(void) const
{
	return eof_;
}

void stream::clear_eof(void)
{
	eof_ = false;
}

bool stream::opened(void) const
{
	return opened_;
}

ACL_VSTREAM* stream::get_vstream() const
{
	return stream_;
}

void stream::set_rw_timeout(int n)
{
	if (stream_)
		stream_->rw_timeout = n;
}

int stream::get_rw_timeout() const
{
	if (stream_ == NULL)
		return -1;
	return stream_->rw_timeout;
}

ACL_VSTREAM* stream::unbind()
{
	eof_ = true;
	opened_ = false;
	ACL_VSTREAM* vstream = stream_;
	stream_ = NULL;
	return vstream;
}

void stream::open_stream(void)
{
	if (stream_ != NULL)
		return;
	stream_ = acl_vstream_fdopen(ACL_SOCKET_INVALID, O_RDWR,
		8192, 0, ACL_VSTREAM_TYPE_SOCK);
}

void stream::reopen_stream(void)
{
	if (stream_)
		acl_vstream_free(stream_);
	open_stream();
}

void stream::set_ctx(void* ctx, const char* key /* = NULL */)
{
	if (key == NULL || *key == 0)
		default_ctx_ = ctx;
	else
		ctx_table_[key] = ctx;
}

void* stream::get_ctx(const char* key /* = NULL */) const
{
	if (key == NULL || *key == 0)
		return default_ctx_;
	std::map<string, void*>::const_iterator it = ctx_table_.find(key);
	if (it != ctx_table_.end())
		return it->second;
	else
		return NULL;
}

void* stream::del_ctx(const char* key /* = NULL */)
{
	if (key == NULL || *key == 0)
	{
		if (default_ctx_ == NULL)
			return false;
		void* ctx = default_ctx_;
		default_ctx_ = NULL;
		return ctx;
	}

	std::map<string, void*>::iterator it = ctx_table_.find(key);
	if (it == ctx_table_.end())
		return NULL;
	void *ctx = it->second;
	ctx_table_.erase(it);
	return ctx;
}

} // namespace acl
