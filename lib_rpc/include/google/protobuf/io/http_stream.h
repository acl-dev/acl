#pragma once
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/stubs/common.h>
#include "acl_cpp/http/http_request.hpp"
#include "acl_cpp/http/http_response.hpp"

namespace google {
namespace protobuf {

class MessageLite;

namespace io {

/**
 * 使用 HTTP 传输协议发送 protobuf 请求数据至服务端，同时等待服务端响应结果
 */
class LIBPROTOBUF_EXPORT http_request
{
public:
	/**
	 * 构造函数（一）：使用输入的 HTTP 请求对象，用户需自行释放之，长连接时
	 * 可以使用本构造函数，从而允许客户端在一个连接上发送多次请求
	 * @param request {acl::http_request*} HTTP 客户端请求对象，非空
	 */
	explicit http_request(acl::http_request* request);

	/**
	 * 构造函数（二）：内部创建 HTTP 请求对象，用完后自动释放之，短连接时
	 * 可以使用本构造函数
	 * @param addr {const char*} http 服务器监听地址，格式：ip:port
	 * @param conn_timeout {int} 连接服务器的超时时间（秒）
	 * @param rw_timeout {int} IO 超时时间（秒）
	 */
	explicit http_request(const char* addr, int conn_timeout = 60,
		int rw_timeout = 60);

	~http_request();

	/**
	 * 发送经 protobuf 序列化的数据包，同时接收服务器响应的序列化数据包
	 * @param in {const MessageLite&} 客户端生成的请求数据包
	 * @param out {MessageLite*} 存储服务端响应的数据包
	 * @return {bool} 是否成功
	 */
	bool rpc_request(const MessageLite& in, MessageLite* out);

	double request_spent() const
	{
		return request_spent_;
	}

	double response_spent() const
	{
		return response_spent_;
	}

	double build_spent() const
	{
		return build_spent_;
	}

	double parse_spent() const
	{
		return parse_spent_;
	}
private:
	acl::http_request* request_;
	acl::http_request* request_inner_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;

	double request_spent_;
	double response_spent_;
	double build_spent_;
	double parse_spent_;
};

/**
 * 服务端使用本类对象接收客户端请求，同时将结果返回给客户端
 */
class LIBPROTOBUF_EXPORT http_response
{
public:
	/**
	 * 构造函数
	 * @param response {acl::http_response*} 与客户端相连的服务端连接对象
	 */
	explicit http_response(acl::http_response* response);

	~http_response();

	/**
	 * 服务端调用本过程读取客户端的数据请求
	 * @param out {MessageLite*} 存储客户端请求数据
	 * @return {bool} 读过程是否成功
	 */
	bool read_request(MessageLite* out);

	/**
	 * 将处理结果通过本过程返回给客户端
	 * @param int {const MessageLite&} 服务端生成的结果数据
	 * @return {bool} 写过程是否成功
	 */
	bool send_response(const MessageLite& in);

	double header_spent() const
	{
		return header_spent_;
	}

	double body_spent() const
	{
		return body_spent_;
	}

	double parse_spent() const
	{
		return parse_spent_;
	}

	double build_spent() const
	{
		return build_spent_;
	}

	double response_spent() const
	{
		return response_spent_;
	}
private:
	acl::http_response* response_;
	double header_spent_;
	double body_spent_;
	double parse_spent_;
	double build_spent_;
	double response_spent_;
};

}  // namespace io
}  // namespace protobuf
}  // namespace google
