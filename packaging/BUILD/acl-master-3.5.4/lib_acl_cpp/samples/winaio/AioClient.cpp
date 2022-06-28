#include "StdAfx.h"
#include "lib_acl.h"
#include <iostream>
#include <assert.h>
#include "acl_cpp/stream/aio_handle.hpp"
#include "AioClient.h"

#ifdef WIN32
# ifndef snprintf
#  define snprintf _snprintf
# endif
#endif

using namespace acl;

CConnectClientCallback::CConnectClientCallback(IO_CTX* ctx,
	aio_socket_stream* client, int id)
: client_(client)
, ctx_(ctx)
, nwrite_(0)
, id_(id)
{
}

CConnectClientCallback::~CConnectClientCallback()
{
	std::cout << ">>>ID: " << id_ << ", CConnectClientCallback deleted now!"
		<< std::endl;
}

bool CConnectClientCallback::read_callback(char* data, int len)
{
	(void) data;
	(void) len;

	ctx_->nread_total++;

	if (ctx_->debug)
	{
		if (nwrite_ < 10)
			std::cout << "gets(" << nwrite_ << "): " << data;
		else if (nwrite_ % 2000 == 0)
			std::cout << ">>ID: " << id_ << ", I: "
			<< nwrite_ << "; "<<  data;
	}

	// 如果收到服务器的退出消息，则也应退出
	if (strncasecmp(data, "quit", 4) == 0)
	{
		// 向服务器发送数据
		client_->format("Bye!\r\n");
		// 关闭异步流连接
		client_->close();
		return (true);
	}

	if (nwrite_ >= ctx_->nwrite_limit)
	{
		if (ctx_->debug)
			std::cout << "ID: " << id_
			<< ", nwrite: " << nwrite_
			<< ", nwrite_limit: " << ctx_->nwrite_limit
			<< ", quiting ..." << std::endl;

		// 向服务器发送退出消息
		client_->format("quit\r\n");
		client_->close();
	}
	else
	{
		char  buf[256];
		snprintf(buf, sizeof(buf), "hello world: %d\n", nwrite_);
		client_->write(buf, (int) strlen(buf));

		// 向服务器发送数据
		//client_->format("hello world: %d\n", nwrite_);
	}

	return (true);
}

bool CConnectClientCallback::write_callback()
{
	ctx_->nwrite_total++;
	nwrite_++;

	// 从服务器读一行数据
	client_->gets(ctx_->read_timeout, false);
	return (true);
}

void CConnectClientCallback::close_callback()
{
	if (client_->is_opened() == false)
	{
		std::cout << "Id: " << id_ << " connect "
			<< ctx_->addr << " error: "
			<< acl_last_serror();

		// 如果是第一次连接就失败，则退出
		if (ctx_->nopen_total == 0)
		{
			std::cout << ", first connect error, quit";
			/* 获得异步引擎句柄，并设置为退出状态 */
			client_->get_handle().stop();
		}
		std::cout << std::endl;
		delete this;
		return;
	}

	// 必须在此处删除该动态分配的回调类对象以防止内存泄露
	delete this;
}

bool CConnectClientCallback::timeout_callback()
{
	std::cout << "Connect " << ctx_->addr << " Timeout ..." << std::endl;
	client_->close();
	return (false);
}

bool CConnectClientCallback::open_callback()
{
	// 连接成功，设置IO读写回调函数
	client_->add_read_callback(this);
	client_->add_write_callback(this);
	ctx_->nopen_total++;

	acl_assert(id_ > 0);
	if (ctx_->nopen_total < ctx_->nopen_limit)
	{
		// 开始进行下一个连接过程
		if (connect_server(ctx_, id_ + 1) == false)
			std::cout << "connect error!" << std::endl;
	}

	// 异步向服务器发送数据
	//client_->format("hello world: %d\n", nwrite_);
	char  buf[256];
	snprintf(buf, sizeof(buf), "hello world: %d\n", nwrite_);
	client_->write(buf, (int) strlen(buf));

	// 异步从服务器读取一行数据
	client_->gets(ctx_->read_timeout, false);

	// 表示继续异步过程
	return (true);
}

bool CConnectClientCallback::connect_server(IO_CTX* ctx, int id)
{
	// 开始异步连接远程服务器
	// const char* addr = "221.194.139.155:18887";
	// ctx->connect_timeout = 1;
	aio_socket_stream* stream = aio_socket_stream::open(ctx->handle,
		ctx->addr, ctx->connect_timeout);
	if (stream == NULL)
	{
		std::cout << "connect " << ctx->addr << " error!" << std::endl;
		std::cout << "stoping ..." << std::endl;
		if (id == 0)
			ctx->handle->stop();
		return (false);
	}

	// 创建连接后的回调函数类
	CConnectClientCallback* callback = new CConnectClientCallback(ctx, stream, id);

	// 添加连接成功的回调函数类
	stream->add_open_callback(callback);

	// 添加连接失败后回调函数类
	stream->add_close_callback(callback);

	// 添加连接超时的回调函数类
	stream->add_timeout_callback(callback);
	return (true);
}
