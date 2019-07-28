#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"

struct ACL_VSTREAM;

namespace acl
{

class ACL_CPP_API ssl_aio_stream
	: public aio_socket_stream
	, public aio_open_callback
{
public:
	/**
	 * 构造函数，创建网络异步客户端流
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param stream {ACL_ASTREAM*} 非阻塞流
	 * @param opened {bool} 该流是否已经与服务端正常建立了连接，如果是则自动
	 *  hook 读写过程及关闭/超时过程，否则仅 hook 关闭/超时过程
	 * @param use_ssl {bool} 是否使用 SSL 套接口
	 */
	ssl_aio_stream(aio_handle* handle, ACL_ASTREAM* stream,
		bool opened = false, bool use_ssl = true);

	/**
	 * 构造函数，创建网络异步客户端流，并 hook 读写过程及关闭/超时过程
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param fd {ACL_SOCKET} 连接套接口句柄
	 * @param use_ssl {bool} 是否使用 SSL 套接口
	 */
#ifdef	WIN32
	ssl_aio_stream(aio_handle* handle, SOCKET fd, bool use_ssl = true);
#else
	ssl_aio_stream(aio_handle* handle, int fd, bool use_ssl = true);
#endif

	/**
	 * 打开与远程服务器的连接，并自动 hook 流的关闭、超时以及连接成功
	 * 时的回调处理过程
	 * @param handle {aio_handle*} 异步引擎句柄
	 * @param addr {const char*} 远程服务器的地址，地址格式为：
	 *  针对TCP：IP:Port 或 针对域套接口：{filePath}
	 * @param timeout {int} 连接超时时间(秒)
	 * @param use_ssl {bool} 是否使用 SSL 套接口
	 * @return {bool} 如果连接立即返回失败则该函数返回 false，如果返回
	 *  true 只是表示正处于连接过程中，至于连接是否超时或连接是否失败
	 *  应通过回调函数来判断
	 */
	static ssl_aio_stream* open(aio_handle* handle,
		const char* addr, int timeout, bool use_ssl = true);

	/**
	 * 该函数用来对已经打开的流进行操作，以允许后期将流设为 SSL 模式
	 * 或非 SSL 模式
	 * @param on {bool} 是否启用 SSL 模式，当该参数为 false 时，如果
	 *  当前流已经是 SSL 模式，则关闭 SSL 模式，如果当前流为非 SSL
	 *  模式，则直接返回；当该参数为 true 时，如果当前流已经是 SSL
	 *  模式，则直接返回，如果当前流为非 SSL 模式，则打开 SSL 模式
	 * @return {bool}
	 */
	bool open_ssl(bool on);

protected:
	virtual ~ssl_aio_stream();

	/**
	* 基类 aio_open_callback 的虚接口
	*/
	virtual bool open_callback();
private:
	void* ssl_;
	void* ssn_;
	void* hs_;

	bool ssl_client_init();

	static int __sock_read(void *ctx, unsigned char *buf, size_t len);
	static int __sock_send(void *ctx, const unsigned char *buf, size_t len);

#ifdef WIN32
	static int __ssl_read(SOCKET fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int __ssl_send(SOCKET fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#else
	static int __ssl_read(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int __ssl_send(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#endif

	void clear(void);
};

} // namespace acl
