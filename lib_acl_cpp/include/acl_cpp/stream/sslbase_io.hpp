#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "stream_hook.hpp"

struct ACL_VSTREAM;

namespace acl {

class sslbase_conf;
class atomic_long;

class ACL_CPP_API sslbase_io : public stream_hook {
public:
	/**
	 * 构造函数
	 * @param conf {sslbase_conf&} 对每一个 SSL 连接进行配置的类对象
	 * @param server_side {bool} 是否为服务端模式，因为客户端模式与服务端
	 *  模式的握手方法不同，所以通过此参数来进行区分
	 * @param nblock {bool} 是否为非阻塞模式
	 */
	sslbase_io(sslbase_conf& conf, bool server_side, bool nblock = false);
	virtual ~sslbase_io();

	/**
	 * ssl 握手纯虚方法
	 * @return {bool}
	 */
	virtual bool handshake() = 0;

	/**
	 * 设置套接字为阻塞模式/非阻塞模式
	 * @param yes {bool} 当为 false 时则设为阻塞模式，否则设为非阻塞模式
	 */
	void set_non_blocking(bool yes);

	/**
	 * 判断当前设置的 SSL IO 是否阻塞模式还是非阻塞模式
	 * @return {bool} 返回 true 则表示为非阻塞模式，否则为阻塞模式
	 */
	bool is_non_blocking() const {
		return nblock_;
	}

	/**
	 * 判断 SSL 握手是否成功
	 * @return {bool}
	 */
	bool handshake_ok() const {
		return handshake_ok_;
	}

	/**
	 * 客户端用来设置 SNI HOST 字段
	 * @param host {const char*}
	 */
	void set_sni_host(const char* host, const char* prefix = NULL,
		const char* suffix = NULL);

	/**
	 * 服务端设置客户端是否发送了 SNI 信息
	 * @param yes {bool}
	 */
	void set_has_sni(bool yes);

	/**
	 * 服务端判断客户端是否发送了 SNI 信息
	 * @return {bool}
	 */
	bool has_sni() const {
		return has_sni_;
	}

	/**
	 * 设置本 SSL IO 对象的绑定对象，方便应用处理自身业务逻辑
	 * @param ctx {void*}
	 */
	void set_ctx(void* ctx);

	/**
	 * 获得由 set_ctx() 设置的绑定对象
	 * @return {void*}
	 */
	void* get_ctx() const {
		return ctx_;
	}

protected:
	sslbase_conf& base_conf_;
	bool server_side_;
	bool nblock_;
	bool handshake_ok_;
	atomic_long* refers_;
	ACL_VSTREAM* stream_;
	string sni_host_;	// Just for client to set SNI.
	bool has_sni_;		// Just for server to check SNI.
	void* ctx_;		// The context for every SSL IO.
};

} // namespace acl
