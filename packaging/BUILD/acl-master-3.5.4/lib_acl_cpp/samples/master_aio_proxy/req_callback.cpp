#include "stdafx.h"
#include "res_callback.h"
#include "req_callback.h"

req_callback::req_callback(acl::aio_socket_stream* conn,
	acl::ofstream* req_fp, acl::ofstream* res_fp)
: conn_(conn)
, res_(NULL)
, req_fp_(req_fp)
, res_fp_(res_fp)
{

}

req_callback::~req_callback()
{
	acl_assert(res_ == NULL);
}

bool req_callback::read_callback(char* data, int len)
{
	if (res_ == NULL)
	{
		logger_warn("server peer disconnected!");
		return false;
	}

	// 取得服务端连接，并将数据写入
	acl::aio_socket_stream& peer = res_->get_conn();
	peer.write(data, len);

	// 将数据写入本地请求文件
	if (req_fp_)
		req_fp_->write(data, len);

	return true;
}

void req_callback::close_callback()
{
	logger("disconnect from %s, fd: %d", conn_->get_peer(),
		conn_->sock_handle());
	if (res_)
	{
		res_callback* res = res_;
		res_ = NULL;
		res->disconnect();
	}

	// 必须在此处删除该动态分配的回调类对象以防止内存泄露  

	delete this;
}

void req_callback::start(const char* server_addr)
{
	// 注册异步流的读回调过程
	conn_->add_read_callback(this);

	// 注册异步流的写回调过程
	conn_->add_write_callback(this);

	// 注册异步流的关闭回调过程
	conn_->add_close_callback(this);

	// 注册异步流的超时回调过程
	conn_->add_timeout_callback(this);

	// 开始连接远程服务器
	acl::aio_handle& handle = conn_->get_handle();
	res_ = new res_callback(this, res_fp_);
	if (res_->start(handle, server_addr) == false)
	{
		logger_error("connect server %s error %s",
			server_addr, acl::last_serror());
		delete res_;
		res_ = NULL;
		conn_->close();
	}
}

acl::aio_socket_stream& req_callback::get_conn()
{
	acl_assert(conn_);
	return *conn_;
}

void req_callback::on_connected()
{
	// 当连接服务端成功后调用此函数开始从客户端读取数据
	conn_->read();
}

void req_callback::disconnect()
{
	if (conn_)
	{
		res_ = NULL;
		conn_->close();
	}
	else
		delete this;
}
