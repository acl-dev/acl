#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/stream_hook.hpp"
#include "acl_cpp/stream/aio_stream.hpp"
#endif

namespace acl
{

aio_stream::aio_stream(aio_handle* handle)
: handle_(handle)
, stream_(NULL)
, hook_(NULL)
, error_hooked_(false)
{
	acl_assert(handle);
	dummy_[0] = 0;
	peer_ip_[0] = 0;
	local_ip_[0] = 0;
}

aio_stream::~aio_stream()
{
	if (hook_)
		hook_->destroy();

	if (stream_)
	{
		handle_->decrease();
		acl_aio_iocp_close(stream_);
	}
	std::list<AIO_CALLBACK*>::iterator it = close_callbacks_.begin();
	for (; it != close_callbacks_.end(); ++it)
		acl_myfree((*it));
	close_callbacks_.clear();

	it = timeout_callbacks_.begin();
	for (; it != timeout_callbacks_.end(); ++it)
		acl_myfree((*it));
	timeout_callbacks_.clear();
}

void aio_stream::destroy()
{
	delete this;
}

void aio_stream::close()
{
	acl_assert(stream_);
	acl_aio_iocp_close(stream_);
}

const char* aio_stream::get_peer(bool full /* = false */) const
{
	if (stream_ == NULL)
		return dummy_;

	ACL_VSTREAM* vs = acl_aio_vstream(stream_);
	const char* ptr = ACL_VSTREAM_PEER(vs);

	if (ptr == NULL || *ptr == 0)
	{
		char  buf[256];
		ACL_SOCKET fd = ACL_VSTREAM_SOCK(vs);

		if (acl_getpeername(fd, buf, sizeof(buf)) == -1)
			return dummy_;
		acl_vstream_set_peer(vs, buf);
	}

	ptr = ACL_VSTREAM_PEER(vs);
	if (full)
		return ptr;
	else if (peer_ip_[0] != 0)
		return peer_ip_;

	return const_cast<aio_stream*> (this)->get_ip(ptr,
		const_cast<aio_stream*>(this)->peer_ip_, sizeof(peer_ip_));
}

const char* aio_stream::get_local(bool full /* = false */) const
{
	if (stream_ == NULL)
		return dummy_;

	ACL_VSTREAM* vs = acl_aio_vstream(stream_);
	const char* ptr = ACL_VSTREAM_LOCAL(vs);
	ACL_SOCKET fd = ACL_VSTREAM_SOCK(vs);
	if (ptr == NULL || *ptr == 0)
	{
		char  buf[256];
		if (acl_getsockname(fd, buf, sizeof(buf)) == -1)
			return dummy_;
		acl_vstream_set_local(vs, buf);
	}

	ptr = ACL_VSTREAM_LOCAL(vs);
	if (full)
		return ptr;
	else if (local_ip_[0] != 0)
		return local_ip_;

	return const_cast<aio_stream*> (this)->get_ip(ptr,
		const_cast<aio_stream*>(this)->local_ip_, sizeof(local_ip_));
}

const char* aio_stream::get_ip(const char* addr, char* buf, size_t size)
{
	safe_snprintf(buf, size, "%s", addr);
	char* ptr = strchr(buf, ':');
	if (ptr)
		*ptr = 0;
	return buf;
}

aio_handle& aio_stream::get_handle() const
{
	return *handle_;
}

ACL_ASTREAM* aio_stream::get_astream() const
{
	return stream_;
}

ACL_VSTREAM* aio_stream::get_vstream() const
{
	if (stream_ == NULL)
		return NULL;
	return acl_aio_vstream(stream_);
}

ACL_SOCKET aio_stream::get_socket() const
{
	ACL_VSTREAM* stream = get_vstream();
	if (stream == NULL)
		return ACL_SOCKET_INVALID;
	return ACL_VSTREAM_SOCK(stream);
}

void aio_stream::add_close_callback(aio_callback* callback)
{
	acl_assert(callback);

	// 先查询该回调对象已经存在
	std::list<AIO_CALLBACK*>::iterator it =
		close_callbacks_.begin();
	for (; it != close_callbacks_.end(); ++it)
	{
		if ((*it)->callback == callback)
		{
			if ((*it)->enable == false)
				(*it)->enable = true;
			return;
		}
	}

	// 找一个空位
	it = close_callbacks_.begin();
	for (; it != close_callbacks_.end(); ++it)
	{
		if ((*it)->callback == NULL)
		{
			(*it)->enable = true;
			(*it)->callback = callback;
			return;
		}
	}

	// 分配一个新的位置
	AIO_CALLBACK* ac = (AIO_CALLBACK*)
		acl_mycalloc(1, sizeof(AIO_CALLBACK));
	ac->enable = true;
	ac->callback = callback;

	// 添加进回调对象队列中
	close_callbacks_.push_back(ac);
}

void aio_stream::add_timeout_callback(aio_callback* callback)
{
	acl_assert(callback);

	// 先查询该回调对象已经存在
	std::list<AIO_CALLBACK*>::iterator it =
		timeout_callbacks_.begin();
	for (; it != timeout_callbacks_.end(); ++it)
	{
		if ((*it)->callback == callback)
		{
			if ((*it)->enable == false)
				(*it)->enable = true;
			return;
		}
	}

	// 找一个空位
	it = timeout_callbacks_.begin();
	for (; it != timeout_callbacks_.end(); ++it)
	{
		if ((*it)->callback == NULL)
		{
			(*it)->enable = true;
			(*it)->callback = callback;
			return;
		}
	}

	// 分配一个新的位置
	AIO_CALLBACK* ac = (AIO_CALLBACK*)
		acl_mycalloc(1, sizeof(AIO_CALLBACK));
	ac->enable = true;
	ac->callback = callback;

	// 添加进回调对象队列中
	timeout_callbacks_.push_back(ac);
}

int aio_stream::del_close_callback(aio_callback* callback)
{
	std::list<AIO_CALLBACK*>::iterator it = close_callbacks_.begin();
	int   n = 0;

	if (callback == NULL)
	{
		for (; it != close_callbacks_.end(); ++it)
		{
			if ((*it)->callback == NULL)
				continue;
			(*it)->enable = false;
			(*it)->callback = NULL;
			n++;
		}
	}
	else
	{
		for (; it != close_callbacks_.end(); ++it)
		{
			if ((*it)->callback != callback)
				continue;
			(*it)->enable = false;
			(*it)->callback = NULL;
			n++;
			break;
		}
	}

	return n;
}

int aio_stream::del_timeout_callback(aio_callback* callback)
{
	std::list<AIO_CALLBACK*>::iterator it = timeout_callbacks_.begin();
	int   n = 0;

	if (callback == NULL)
	{
		for (; it != timeout_callbacks_.end(); ++it)
		{
			if ((*it)->callback == NULL)
				continue;
			(*it)->enable = false;
			(*it)->callback = NULL;
			n++;
		}
	}
	else
	{
		for (; it != timeout_callbacks_.end(); ++it)
		{
			if ((*it)->callback != callback)
				continue;
			(*it)->enable = false;
			(*it)->callback = NULL;
			n++;
			break;
		}
	}

	return n;
}

int aio_stream::disable_close_callback(aio_callback* callback)
{
	std::list<AIO_CALLBACK*>::iterator it = close_callbacks_.begin();
	int   n = 0;

	if (callback == NULL)
	{
		for (; it != close_callbacks_.end(); ++it)
		{
			if ((*it)->callback == NULL || !(*it)->enable)
				continue;
			(*it)->enable = false;
			n++;
		}
	}
	else
	{
		for (; it != close_callbacks_.end(); ++it)
		{
			if ((*it)->callback != callback || !(*it)->enable)
				continue;
			(*it)->enable = false;
			n++;
			break;
		}
	}

	return n;
}

int aio_stream::disable_timeout_callback(aio_callback* callback)
{
	std::list<AIO_CALLBACK*>::iterator it = timeout_callbacks_.begin();
	int   n = 0;

	if (callback == NULL)
	{
		for (; it != timeout_callbacks_.end(); ++it)
		{
			if ((*it)->callback == NULL || !(*it)->enable)
				continue;
			(*it)->enable = false;
			n++;
		}
	}
	else
	{
		for (; it != timeout_callbacks_.end(); ++it)
		{
			if ((*it)->callback != callback || !(*it)->enable)
				continue;
			(*it)->enable = false;
			n++;
			break;
		}
	}

	return n;
}

int aio_stream::enable_close_callback(aio_callback* callback /* = NULL */)
{
	std::list<AIO_CALLBACK*>::iterator it = close_callbacks_.begin();
	int   n = 0;

	if (callback == NULL)
	{
		for (; it != close_callbacks_.end(); ++it)
		{
			if (!(*it)->enable && (*it)->callback != NULL)
			{
				(*it)->enable = true;
				n++;
			}
		}
	}
	else
	{
		for (; it != close_callbacks_.end(); ++it)
		{
			if (!(*it)->enable && (*it)->callback == callback)
			{
				(*it)->enable = true;
				n++;
			}
		}
	}

	return n;
}

int aio_stream::enable_timeout_callback(aio_callback* callback /* = NULL */)
{
	std::list<AIO_CALLBACK*>::iterator it = timeout_callbacks_.begin();
	int   n = 0;

	if (callback == NULL)
	{
		for (; it != timeout_callbacks_.end(); ++it)
		{
			if (!(*it)->enable && (*it)->callback != NULL)
			{
				(*it)->enable = true;
				n++;
			}
		}
	}
	else
	{
		for (; it != timeout_callbacks_.end(); ++it)
		{
			if (!(*it)->enable && (*it)->callback == callback)
			{
				(*it)->enable = true;
				n++;
			}
		}
	}

	return n;
}

void aio_stream::hook_error()
{
	acl_assert(stream_);
	if (error_hooked_)
		return;

	error_hooked_ = true;
	handle_->increase();  // 增加异步流计数

	// 注册回调函数以截获关闭时的过程
	/*
	acl_aio_ctl(stream_,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, close_callback, this,
		ACL_AIO_CTL_END);
	*/
	acl_aio_add_close_hook(stream_, close_callback, this);

	// 注册回调函数以截获超时时的过程
	/*
	acl_aio_ctl(stream_,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, timeout_callback, this,
		ACL_AIO_CTL_END);
	*/
	acl_aio_add_timeo_hook(stream_, timeout_callback, this);
}

int aio_stream::close_callback(ACL_ASTREAM* stream acl_unused, void* ctx)
{
	aio_stream* as = (aio_stream*) ctx;
	std::list<AIO_CALLBACK*>::iterator it = as->close_callbacks_.begin();
	for (; it != as->close_callbacks_.end(); ++it)
	{
		if ((*it)->enable == false || (*it)->callback == NULL)
			continue;

		(*it)->callback->close_callback();
	}

	as->destroy();
	return 0;
}

int aio_stream::timeout_callback(ACL_ASTREAM* stream acl_unused, void* ctx)
{
	aio_stream* as = (aio_stream*) ctx;
	std::list<AIO_CALLBACK*>::iterator it = as->timeout_callbacks_.begin();
	for (; it != as->timeout_callbacks_.end(); ++it)
	{
		if ((*it)->enable == false || (*it)->callback == NULL)
			continue;

		if ((*it)->callback->timeout_callback() == false)
			return -1;
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////

stream_hook* aio_stream::get_hook() const
{
	return hook_;
}

stream_hook* aio_stream::remove_hook()
{
	ACL_VSTREAM* vstream = get_vstream();
	if (vstream == NULL)
	{
		logger_error("vstream null");
		return NULL;
	}

	stream_hook* hook = hook_;
	hook_ = NULL;

	if (vstream->type == ACL_VSTREAM_TYPE_FILE)
	{
		vstream->fread_fn  = acl_file_read;
		vstream->fwrite_fn = acl_file_write;
		vstream->fwritev_fn = acl_file_writev;
		vstream->fclose_fn = acl_file_close;
	}
	else
	{
		vstream->read_fn  = acl_socket_read;
		vstream->write_fn = acl_socket_write;
		vstream->writev_fn = acl_socket_writev;
		vstream->close_fn = acl_socket_close;
	}

	return hook;
}

#define	HOOK_KEY	"aio_stream::setup_hook"

stream_hook* aio_stream::setup_hook(stream_hook* hook)
{
	ACL_VSTREAM* vstream = get_vstream();
	if (vstream == NULL)
	{
		logger_error("vstream null");
		return NULL;
	}

	stream_hook* old_hook = hook_;

	if (vstream->type == ACL_VSTREAM_TYPE_FILE)
	{
		ACL_FSTREAM_RD_FN read_fn = vstream->fread_fn;
		ACL_FSTREAM_WR_FN write_fn = vstream->fwrite_fn;

		vstream->fread_fn = fread_hook;
		vstream->fwrite_fn = fsend_hook;
		acl_vstream_add_object(vstream, HOOK_KEY, this);

		if (hook->open(vstream) == false)
		{
			// 如果打开失败，则恢复

			vstream->fread_fn = read_fn;
			vstream->fwrite_fn = write_fn;
			acl_vstream_del_object(vstream, HOOK_KEY);
			return hook;
		}
	}
	else
	{
		ACL_VSTREAM_RD_FN read_fn = vstream->read_fn;
		ACL_VSTREAM_WR_FN write_fn = vstream->write_fn;

		vstream->read_fn = read_hook;
		vstream->write_fn = send_hook;
		acl_vstream_add_object(vstream, HOOK_KEY, this);

		acl_tcp_set_nodelay(ACL_VSTREAM_SOCK(vstream));

		if (hook->open(vstream) == false)
		{
			// 如果打开失败，则恢复

			vstream->read_fn = read_fn;
			vstream->write_fn = write_fn;
			acl_vstream_del_object(vstream, HOOK_KEY);
			return hook;
		}
	}

	hook_ = hook;
	return old_hook;
}

int aio_stream::read_hook(ACL_SOCKET, void *buf, size_t len,
	int, ACL_VSTREAM* vs, void *)
{
	aio_stream* s = (aio_stream*) acl_vstream_get_object(vs, HOOK_KEY);
	acl_assert(s);

	if (s->hook_ == NULL)
	{
		logger_error("hook_ null");
		return -1;
	}
	return s->hook_->read(buf, len);
}

int aio_stream::send_hook(ACL_SOCKET, const void *buf, size_t len,
	int, ACL_VSTREAM* vs, void *)
{
	aio_stream* s = (aio_stream*) acl_vstream_get_object(vs, HOOK_KEY);
	acl_assert(s);

	if (s->hook_ == NULL)
	{
		logger_error("hook_ null");
		return -1;
	}
	return s->hook_->send(buf, len);
}

int aio_stream::fread_hook(ACL_FILE_HANDLE, void *buf, size_t len,
	int, ACL_VSTREAM* vs, void *)
{
	aio_stream* s = (aio_stream*) acl_vstream_get_object(vs, HOOK_KEY);
	acl_assert(s);

	if (s->hook_ == NULL)
	{
		logger_error("hook_ null");
		return -1;
	}
	return s->hook_->read(buf, len);
}

int aio_stream::fsend_hook(ACL_FILE_HANDLE, const void *buf, size_t len,
	int, ACL_VSTREAM* vs, void *)
{
	aio_stream* s = (aio_stream*) acl_vstream_get_object(vs, HOOK_KEY);
	acl_assert(s);

	if (s->hook_ == NULL)
	{
		logger_error("hook_ null");
		return -1;
	}
	return s->hook_->send(buf, len);
}

}  // namespace acl
