#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/ipc/ipc_client.hpp"
#endif

namespace acl
{

ipc_client::ipc_client(acl_int64 magic /* = -1 */)
: magic_(magic)
, addr_(NULL)
, async_stream_(NULL)
, sync_stream_(NULL)
, sync_stream_inner_(NULL)
, closing_(false)
, status_(IO_WAIT_HDR)
{

}

ipc_client::~ipc_client()
{
	messages_.clear();

	if (addr_)
		acl_myfree(addr_);
	if (async_stream_ && !closing_)
		async_stream_->close();
	delete sync_stream_inner_;
}

void ipc_client::on_message(int nMsg, void* data, int dlen)
{
	(void) nMsg;
	(void) data;
	(void) dlen;

	// 子类必须实现该接口
	logger_fatal("ipc_client be called here");
}

bool ipc_client::open(aio_handle* handle, const char* addr, int timeout)
{
	acl_assert(sync_stream_ == NULL && async_stream_ == NULL);

	async_stream_ = aio_socket_stream::open(handle, addr, timeout);
	if (async_stream_ == NULL)
		return (false);
	addr_ = acl_mystrdup(addr);

	// 添加连接成功的回调函数类
	async_stream_->add_open_callback(this);

	// 添加连接失败后回调函数类
	async_stream_->add_close_callback(this);

	// 添加连接超时的回调函数类
	async_stream_->add_timeout_callback(this);
	return (true);
}

void ipc_client::open(aio_socket_stream* client)
{
	acl_assert(client);
	acl_assert(sync_stream_ == NULL && async_stream_ == NULL);

	async_stream_ = client;

	// 注册异步流的读回调过程
	client->add_read_callback(this);

	// 注册异步流的写回调过程
	client->add_write_callback(this);

	// 添加连接失败后回调函数类
	client->add_close_callback(this);

	// 添加连接超时的回调函数类
	client->add_timeout_callback(this);
}

bool ipc_client::open(const char* addr, int timeout)
{
	acl_assert(sync_stream_ == NULL && async_stream_ == NULL);
	sync_stream_ = NEW socket_stream();
	if (sync_stream_->open(addr, timeout, 0) == false)
	{
		delete sync_stream_;
		sync_stream_ = NULL;
		return (false);
	}
	sync_stream_inner_ = sync_stream_;
	return (true);
}

void ipc_client::open(socket_stream* client)
{
	acl_assert(sync_stream_ == NULL && async_stream_ == NULL);
	acl_assert(client);
	sync_stream_ = client;
}

void ipc_client::wait()
{
	if (closing_)
		return;

	// 同步接收消息
	if (sync_stream_)
	{
		MSG_HDR hdr;
		int   n;
		n = sync_stream_->read(&hdr, sizeof(hdr));
		if (n < 0)
		{
			close();
			return;
		}
		if ((n = hdr.dlen) <= 0)
		{
			trigger(hdr.nMsg, NULL, 0);
			return;
		}
		string buf(n);

		if (sync_stream_->read(buf.c_str(), n) < 0)
		{
			close();
			return;
		}
		trigger(hdr.nMsg, buf.c_str(), n);
	}

	// 异步接收消息
	else if (async_stream_)
	{
		// 进入异步读消息过程
		status_ = IO_WAIT_HDR;
		async_stream_->read(sizeof(MSG_HDR));
	}

	// 未知情况
	else
		acl_assert(0);
}

void ipc_client::close()
{
	if (closing_)
		return;

	closing_ = true;

	if (async_stream_)
		async_stream_->close();
	else if (sync_stream_inner_)
	{
		delete sync_stream_inner_;
		sync_stream_inner_ = sync_stream_ = NULL;
	}
}

bool ipc_client::active() const
{
	if (closing_)
		return false;
	else if (async_stream_ != NULL || sync_stream_ != NULL)
		return true;
	else
		return false;
}

aio_socket_stream* ipc_client::get_async_stream() const
{
	acl_assert(async_stream_);
	return (async_stream_);
}

aio_handle& ipc_client::get_handle() const
{
	return (get_async_stream()->get_handle());
}

socket_stream* ipc_client::get_sync_stream() const
{
	acl_assert(sync_stream_);
	return (sync_stream_);
}

void ipc_client::append_message(int nMsg)
{
	std::list<int>::iterator it = messages_.begin();

	// 该消息是否已经存在于已经注册的消息集合中
	for (; it != messages_.end(); ++it)
	{
		if (*it == nMsg)
			return;
	}

	messages_.push_back(nMsg);
}

void ipc_client::delete_message(int nMsg)
{
	std::list<int>::iterator it = messages_.begin();

	// 该消息是否已经存在于已经注册的消息集合中
	for (; it != messages_.end(); ++it)
	{
		if (*it == nMsg)
		{
			messages_.erase(it);
			break;
		}
	}
}

void ipc_client::send_message(int nMsg, const void* data, int dlen)
{
	struct MSG_HDR hdr;

	hdr.nMsg = nMsg;
	hdr.dlen = dlen > 0 ? dlen : 0;
	hdr.magic = magic_;

	// 发送消息头
	if (sync_stream_ != NULL)
		sync_stream_->write(&hdr, sizeof(hdr));
	else if (async_stream_ != NULL)
		async_stream_->write(&hdr, sizeof(hdr));
	else
		acl_assert(0);

	// 发关消息体
	if (data && dlen > 0)
	{
		if (sync_stream_ == NULL)
			async_stream_->write(data, dlen);
		else if (sync_stream_->write(data, dlen) < 0)
			close();
	}
}

void ipc_client::trigger(int nMsg, void* data, int dlen)
{
	std::list<int>::iterator it = messages_.begin();

	// 该消息是否已经存在于已经注册的消息集合中
	for (; it != messages_.end(); ++it)
	{
		if (*it == nMsg)
		{
			on_message(nMsg, data, dlen);
			return;
		}
	}

	logger_warn("unknown msg: %d", nMsg);
}

bool ipc_client::read_callback(char* data, int len)
{
	if (status_ == IO_WAIT_HDR)
	{
		acl_assert(len == sizeof(MSG_HDR));
		MSG_HDR* hdr = (MSG_HDR*) data;

		// 先检查 IPC 消息数据的有效性
		if (hdr->magic != magic_)
		{
			logger_error("unknown ipc magic: %lld", hdr->magic);
			return false;
		}
		//logger(">>>ok, magic: %d", magic_);
		if (hdr->dlen > 0)
		{
			hdr_.nMsg = hdr->nMsg;
			hdr_.dlen = hdr->dlen;
			// 如果有消息体则继续读消息体
			status_ = IO_WAIT_DAT;
			async_stream_->read(hdr->dlen);
			return (true);
		}

		// 消息处理过程
		trigger(hdr->nMsg, NULL, 0);

		// 异步等待下一条消息
		wait();
		return (true);
	}
	else if (status_ == IO_WAIT_DAT)
	{
		acl_assert(len == hdr_.dlen);
		trigger(hdr_.nMsg, data, len);

		// 异步等待下一条消息
		wait();
		return (true);
	}
	else
		acl_assert(0);

	return (true);
}

bool ipc_client::write_callback()
{
	return (true);
}

void ipc_client::close_callback()
{
	// 通知子类关闭IPC异步流
	on_close();
}

bool ipc_client::timeout_callback()
{
	return (true);
}

bool ipc_client::open_callback()
{
	// 连接成功，设置IO读写回调函数
	async_stream_->add_read_callback(this);
	async_stream_->add_write_callback(this);

	// 通知子类连接消息服务器成功
	on_open();

	// 返回 true 表示继续异步过程
	return (true);
}

}  // namespace acl
