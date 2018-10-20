#pragma once
#include "../acl_cpp_define.hpp"
#include "HttpServlet.hpp"

namespace acl
{

class websocket;
class session;
class HttpServletRequest;
class HttpServletResponse;

class WebSocketServlet: public HttpServlet
{
public:
	WebSocketServlet(void);

	/**
	 * 构造函数
	 * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
	 *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
	 *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
	 *  的关闭情况，这样可以方便与 acl_master 架构结合
	 * @param session {session*} 每一个 HttpServlet 对象一个 session 对象
	 */
	WebSocketServlet(socket_stream* stream, session* session);

	/**
	 * 构造函数
	 * @param stream {socket_stream*} 当在 acl_master 服务器框架控制下
	 *  运行时，该参数必须非空；当在 apache 下以 CGI 方式运行时，该参数
	 *  设为 NULL；另外，该函数内部不会关闭流连接，应用应自行处理流对象
	 *  的关闭情况，这样可以方便与 acl_master 架构结合
	 * @param memcache_addr {const char*}
	 */
	WebSocketServlet(socket_stream* stream,
		const char* memcache_addr = "127.0.0.1:11211");

	/**
	 * HttpServlet 对象开始运行，接收 HTTP 请求，并回调以下 doXXX 虚函数，
	 * 该函数首先会调用 start 过程，然后根据 start 的返回结果及请求/响应
	 * 对象是否要求保持长连接来决定是否需要与客户端保持长连接.
	 * @return {bool} 返回处理结果，返回 false 表示处理失败或处理成功且
	 *  不保持长连接，应关闭连接
	 */

	virtual ~WebSocketServlet(void);

	// @override
	bool doRun(void);

	// @override
	bool doRun(session& session, socket_stream* stream = NULL);

	// @override
	bool doRun(const char* memcached_addr, socket_stream* stream);

	/**
	 * 发送二进制数据.
	 * @param rw_timeout {const char *} 发送的数据
	 * @return {bool} 错误 false.否则 true
	 */

	bool sendBinary(const char *buf, int len);

	/**
	 * 发送文本数据.
	 * @param rw_timeout {const char *} 发送的数据
	 * @return {bool} 错误 false.否则 true
	 */

	bool sendText(const char *text);

	/**
	 * 发送pong 消息.
	 * @param rw_timeout {const char *} 发送的数据
	 * @return {bool} 错误 false.否则 true
	 */
	bool sendPong(const char *buffer = NULL);

	/**
	 * 发送pong 消息.
	 * @param rw_timeout {const char *} 发送的数据
	 * @return {bool} 错误 false.否则 true
	 */
	bool sendPing(const char *buffer = NULL);

	unsigned long long getMaxMsgLen(void) const
	{
		return max_msg_len_;
	}

	/**
	 * 设置最大消息长度，当websocket 消息大于这个值，将断开websocket连接.
	 * @param unsigned long long{len} 新的长度
	 */
	void setMaxMsgLen(unsigned long long len)
	{
		max_msg_len_ = len;
	}

protected:
	/**
	 * websocket 关闭消息回调
	 * @return {void}
	 */
	virtual void onClose(void) {}

	/**
	 * websocket ping 消息回调.
	 * @param {const char *} buf 消息数据
	 * @param {int} len 消息数据长度
	 * @return {bool} false 断开连接。
	 */
	virtual bool onPing(const char *buf, unsigned long long  len) = 0;

	/**
	 * websocket pong 消息回调.
	 * @param {const char *} buf 消息数据
	 * @param {int} len 消息数据长度
	 * @return {bool} false 断开连接。
	 */
	virtual bool onPong(const char *buf, unsigned long long  len) = 0;

	/**
	 * websocket ping 消息回调.
	 * @param data{char *} 回调数据缓存区。
	 * @param len{unsigned long long}回调数据缓存区长度。
	 * @param text{bool } true 为文本数据。否则是 二进制数据。
	 * @return {bool} false 断开连接。
	 */
	virtual bool onMessage(char *data, unsigned long long len, bool text) = 0;

private:
	// @override
	bool doWebsocket(HttpServletRequest&, HttpServletResponse&);

private:

	unsigned long long max_msg_len_;
	websocket *ws_;

	char *buffer_;
	int   wpos_;
	int   opcode_;
};

} // namespace acl
