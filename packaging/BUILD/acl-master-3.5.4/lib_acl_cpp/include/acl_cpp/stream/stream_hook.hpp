#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

/**
 * 流对象 IO 注册回调类，子类需实现该类中的虚方法，子类对象通过 setup_hook 注册，
 * 然后该子类对象中的 IO 过程作为 stream/aio_stream 类对象的底层 IO 过程被使用；
 * 如果不调用 stream/aio_stream 的 setup_hook 注册过程，则 stream/aio_stream
 * 类对象的底层 IO 过程为默认过程
 * XXX： 本纯虚类被声明为堆对象类，建议子类也应该声明为堆对象类
 */
class ACL_CPP_API stream_hook : public noncopyable
{
public:
	stream_hook(void) {}

	/**
	 * 读数据接口
	 * @param buf {void*} 读缓冲区地址，读到的数据将存放在该缓冲区中
	 * @param len {size_t} buf 缓冲区大小
	 * @return {int} 读到字节数，当返回值 < 0 时表示出错
	 */
	virtual int read(void* buf, size_t len) = 0;

	/**
	 * 发送数据接口
	 * @param buf {const void*} 发送缓冲区地址
	 * @param len {size_t} buf 缓冲区中数据的长度(必须 > 0)
	 * @return {int} 写入的数据长度，返回值 <０　时表示出错
	 */
	virtual int send(const void* buf, size_t len) = 0;

	/**
	 * 在 stream/aio_stream 的 setup_hook 内部将会调用 stream_hook::open
	 * 过程，以便于子类对象用来初始化一些数据及会话
	 * @param s {ACL_VSTREAM*} 在 setup_hook 内部调用该方法将创建的流对象
	 *  作为参数传入
	 * @return {bool} 如果子类实例返回 false，则 setup_hook 调用失败且会恢复原样
	 */
	virtual bool open(ACL_VSTREAM* s) = 0;

	/**
	 * 当 stream/aio_stream 流对象关闭前将会回调该函数以便于子类实例做一些善后工作
	 * @param alive {bool} 该连接是否依然正常
	 * @return {bool}
	 */
	virtual bool on_close(bool alive) { (void) alive; return true; }

	/**
	 * 当 stream/aio_stream 对象需要释放 stream_hook 子类对象时调用此方法
	 */
	virtual void destroy(void) {}

protected:
	virtual ~stream_hook(void) {}
};

} // namespace acl
