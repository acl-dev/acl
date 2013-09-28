#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

struct ACL_VSTREAM;
typedef struct _ssl_session ssl_session;
typedef struct _ssl_context ssl_context;

namespace acl
{

class ACL_CPP_API ssl_stream : public socket_stream
{
public:
	ssl_stream(void);
	virtual ~ssl_stream(void);

	/**
	 * 根据套接字打开的一个网络流
	 * @param fd 套接字
	 * @param use_ssl {bool} 是否采用 SSL 连接方式
	 * @return {bool} 连接是否成功
	 */
#ifdef	WIN32
	bool open_ssl(SOCKET fd, bool use_ssl = true);
#else
	bool open_ssl(int fd, bool use_ssl = true);
#endif

	/**
	 * 根据 ACL_VSTREAM 流打开网络流
	 * @param vstream {ACL_VSTREAM*}
	 * @param use_ssl {bool} 是否采用 SSL 加密传输方式
	 * @return {bool} 连接是否成功
	 */
	bool open_ssl(ACL_VSTREAM* vstream, bool use_ssl = true);

	/**
	 * 连接远程服务器并打开网络连接流
	 * @param addr {const char*} 远程服务器监听地址, 格式: IP:PORT,
	 *  对UNIX平台, 该地址还可以为域套接口地址, 如: /tmp/mysock
	 * @param conn_timeout {int} 连接超时时间(秒)
	 * @param rw_timeout {int} 读写超时时间(秒)
	 * @param use_ssl {bool} 是否采用 SSL 加密传输方式
	 * @return {bool} 连接是否成功
	 */
	bool open_ssl(const char* addr, int conn_timeout,
		int rw_timeout, bool use_ssl = true);

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
private:
	ssl_context* ssl_;
	ssl_session* ssn_;
	void* hs_;

	bool ssl_client_init(void);

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
