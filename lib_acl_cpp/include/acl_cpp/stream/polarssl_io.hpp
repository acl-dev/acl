#pragma once

#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stream/stream.hpp"

namespace acl {

class stream;
class polarssl_conf;

/**
 * stream 流对象底层 IO 处理过程的处理类，该类对象中的读写的过程将会替代
 * stream 流对象中 默认的底层 IO 过程；该类对象必须是动态创建的(即为堆对象)，
 * stream 流对象通过调用本类对象的 destroy()　方法释放本类对象
 */
class ACL_CPP_API polarssl_io : public stream_hook
{
public:
	/**
	 * 构造函数
	 * @param conf {polarssl_conf&} 对每一个 SSL 连接进行配置的类对象
	 * @param server_side {bool} 是否为服务端模式，因为客户端模式与服务端
	 *  模式的握手方法不同，所以通过此参数来进行区分
	 */
	polarssl_io(polarssl_conf& conf, bool server_side);

	virtual void destroy();

private:
	~polarssl_io();

	virtual bool open(stream* s);
	virtual bool on_close(bool alive);
	virtual int read(void* buf, size_t len);
	virtual int send(const void* buf, size_t len);

private:
	polarssl_conf& conf_;
	bool server_side_;
	void* ssl_;
	void* ssn_;
	void* rnd_;
	const stream* stream_;

	static int sock_read(void *ctx, unsigned char *buf, size_t len);
	static int sock_send(void *ctx, const unsigned char *buf, size_t len);
};

} // namespace acl
