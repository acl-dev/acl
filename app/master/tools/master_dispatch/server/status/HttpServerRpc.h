#pragma once

class HttpServerRpc : public acl::rpc_request
{
public:
	HttpServerRpc(acl::aio_socket_stream* client);
	~HttpServerRpc();

protected:
	// 基类 rpc_request 虚函数

	// 子线程处理函数
	void rpc_run();

	// 主线程处理过程，收到子线程任务完成的消息
	void rpc_onover();

private:
	acl::aio_socket_stream* client_;
	bool keep_alive_;

	// 处理 HTTP 请求过程
	void handle_http(acl::socket_stream& stream);
};
