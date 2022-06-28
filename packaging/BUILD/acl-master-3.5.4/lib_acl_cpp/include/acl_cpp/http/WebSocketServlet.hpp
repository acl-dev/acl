#pragma once
#include "../acl_cpp_define.hpp"
#include "HttpServlet.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

class websocket;
class session;
class HttpServletRequest;
class HttpServletResponse;

class ACL_CPP_API WebSocketServlet : public HttpServlet
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
	 * @param buf {const char *} 发送的数据
	 * @param len {int} buf 数据长度
	 * @return {bool} 错误 false, 否则 true
	 */
	bool sendBinary(const char *buf, int len);

	/**
	 * 发送文本数据.
	 * @param text {const char *} 发送的数据
	 * @return {bool} 错误 false, 否则 true
	 */
	bool sendText(const char *text);

	/**
	 * 发送pong 消息.
	 * @param buffer {const char *} 发送的数据
	 * @return {bool} 错误 false, 否则 true
	 */
	bool sendPong(const char *buffer = NULL);

	/**
	 * 发送pong 消息.
	 * @param buffer {const char *} 发送的数据
	 * @return {bool} 错误 false, 否则 true
	 */
	bool sendPing(const char *buffer = NULL);

protected:
	/**
	 * websocket 关闭消息回调
	 */
	virtual void onClose(void) {}

	/**
	 * websocket ping 消息回调.
	 * @param payload_len {unsigned long long} 消息数据总长度
	 * @param finish {bool} 本数据包是否最后一个
	 * @return {bool} false 断开连接。
	 */
	virtual bool onPing(unsigned long long payload_len, bool finish) = 0;

	/**
	 * websocket pong 消息回调.
	 * @param payload_len {unsigned long long} 消息数据总长度
	 * @param finish {bool} 本数据包是否最后一个
	 * @return {bool} false 断开连接。
	 */
	virtual bool onPong(unsigned long long payload_len, bool finish) = 0;

	/**
	 * websocket ping 消息回调.
	 * @param payload_len {unsigned long long} 消息数据总长度
	 * @param text {bool } true 表示为文本数据, 否则是 二进制数据。
	 * @param finish {bool} 本数据包是否最后一个
	 * @return {bool} false 断开连接。
	 */
	virtual bool onMessage(unsigned long long payload_len,
			bool text, bool finish) = 0;

	/**
	 * 子类可以循环调用此方法获得数据帧的数据体，直至返回 <= 0 为止
	 * @param buf {size_t*} 数据缓冲区用来存放结果数据
	 * @param size {size_t} buf 缓冲区大小
	 * @return {int} 读到的数据长度，分以下三种情形：
	 *   0: 表示数据帧正常读完
	 *  -1: 表示读出错
	 *  >0: 表示读到的数据，应再次调用本方法以便读余下的数据
	 */
	int readPayload(void* buf, size_t size);

	/**
	 * 返回 websocket 对象，如果返回 NULL 表示还未建立 websocket 连接
	 * @return {websocket*}
	 */
	websocket* get_websocket(void) const
	{
		return ws_;
	}

private:
	// @override
	bool doWebSocket(HttpServletRequest&, HttpServletResponse&);

private:
	websocket *ws_;
	int   opcode_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
