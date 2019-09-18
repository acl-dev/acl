#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include <list>
#include <string>
#if defined(_WIN32) || defined(_WIN64)
#include <WinSock2.h>
#endif

struct ACL_ASTREAM;
struct ACL_VSTREAM;

namespace acl
{

/**
 * 异步流回调类
 */
class ACL_CPP_API aio_callback : public noncopyable
{
public:
	aio_callback(void) {}
	virtual ~aio_callback(void) {};

	virtual void close_callback(void) {}
	virtual bool timeout_callback(void)
	{
		return false;
	}

	/**
	 * 读回调虚函数，该回调函数当满足了类 aio_istream 实例中的
	 * gets/read 的可读条件后被调用，由异步框架内部将符合条件的数
	 * 据读出，直接传递给用户的子类
	 * @param data {char*} 读到的数据的指针地址
	 * @param len {int} 读到的数据长度(> 0)
	 * @return {bool} 该函数返回 false 通知异步引擎关闭该异步流
	 */
	virtual bool read_callback(char* data, int len)
	{
		(void) data;
		(void) len;
		return true;
	}

	/**
	 * 读回调虚函数，该回调函数当满足了类 aio_istream 实例中的
	 * read_wait 的可读条件即异步流中有数据可读时被调用；当超时时会
	 * 调用 timeout_callback，流异常被关闭时会调用 close_callback
	 */
	virtual bool read_wakeup(void)
	{
		return true;
	}

	/**
	 * 写成功后的回调虚函数
	 * @return {bool} 该函数返回 false 通知异步引擎关闭该异步流
	 */
	virtual bool write_callback(void)
	{
		return true;
	}

	/**
	 * 读回调虚函数，该回调函数当满足了类 aio_ostream 实例中的
	 * write_wait 的可写条件即异步流可写时被调用；当超时时会
	 * 调用 timeout_callback，流异常被关闭时会调用 close_callback
	 */
	virtual bool write_wakeup(void)
	{
		return true;
	}
};

struct AIO_CALLBACK 
{
	aio_callback* callback;
	bool enable;
};

class aio_handle;
class stream_hook;

/**
 * 异步流基类，该类为纯虚类，不能被直接实例化，只能被子类继承使用
 * 该类只能在堆上分配，不能在栈上分配
 */
class ACL_CPP_API aio_stream : public noncopyable
{
public:
	/**
	 * 构造函数
	 * @param handle {aio_handle*}
	 */
	aio_stream(aio_handle* handle);

	/**
	 * 关闭异步流
	 */
	void close(void);

	/**
	 * 添加关闭时的回调类对象指针，如果该回调类对象已经存在，则只是
	 * 使该对象处于打开可用状态
	 * @param callback {aio_callback*} 继承 aio_callback 的子类回调类对象，
	 *  当异步流关闭前会先调用此回调类对象中的 close_callback 接口
	 */
	void add_close_callback(aio_callback* callback);

	/**
	 * 添加超时时的回调类对象指针，如果该回调类对象已经存在，则只是
	 * 使该对象处于打开可用状态
	 * @param callback {aio_callback*} 继承 aio_callback 的子类回调类对象，
	 *  当异步流关闭前会先调用此回调类对象中的 timeout_callback 接口
	 */
	void add_timeout_callback(aio_callback* callback);

	/**
	 * 删除关闭时的回调类对象指针
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则删除所有的关闭回调对象
	 * @return {int} 返回被从回调对象集合中删除的回调对象的个数
	 */
	int del_close_callback(aio_callback* callback = NULL);

	/**
	 * 删除超时时的回调类对象指针
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则删除所有的超时回调对象
	 * @return {int} 返回被从回调对象集合中删除的回调对象的个数
	 */
	int del_timeout_callback(aio_callback* callback = NULL);

	/**
	 * 禁止关闭的回调类对象，但并不从关闭对象集合中删除
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则禁止所有的关闭回调对象
	 * @return {int} 返回被从回调对象集合中禁用的回调对象的个数
	 */
	int disable_close_callback(aio_callback* callback = NULL);

	/**
	 * 禁止超时的回调类对象，但并不从超时对象集合中删除
	 * @param callback {aio_callback*} 从 aio_callback 继承的子类对象指针，
	 *  若该值为空，则禁止所有的超时回调对象
	 * @return {int} 返回被从回调对象集合中禁用的回调对象的个数
	 */
	int disable_timeout_callback(aio_callback* callback = NULL);

	/**
	 * 启用所有的回调对象被调用
	 * @param callback {aio_callback*} 启用指定的回调对象，如果该值为空，
	 *  则启用所有的关闭回调对象
	 * @return {int} 返回被启用的回调对象的个数
	 */
	int enable_close_callback(aio_callback* callback = NULL);

	/**
	 * 启用所有的回调对象被调用
	 * @param callback {aio_callback*} 启用指定的回调对象，如果该值为空，
	 *  则启用所有的超时回调对象
	 * @return {int} 返回被启用的回调对象的个数
	 */
	int enable_timeout_callback(aio_callback* callback = NULL);

	/**
	 * 获得异步流对象 ACL_ASTREAM
	 * @return {ACL_ASTREAM*}
	 */
	ACL_ASTREAM* get_astream(void) const;

	/**
	 * 获得异步流对象中的同步流对象 ACL_VSTREAM
	 * @return {ACL_VSTREAM*}
	 */
	ACL_VSTREAM* get_vstream(void) const;

	/**
	 * 获得异步流中的 SOCKET 描述符
	 * @return {ACL_SOCKET} 若不存在则返回 -1(UNIX) 或 INVALID_SOCKET(win32)
	 */
#if defined(_WIN32) || defined(_WIN64)
	SOCKET get_socket(void) const;
	SOCKET sock_handle(void) const
#else
	int get_socket(void) const;
	int sock_handle(void) const
#endif
	{
		return get_socket();
	}

	/**
	 * 获得远程连接的地址
	 * @param full {bool} 是否获得完整地址，即：IP:PORT，如果该参数
	 *  为 false，则仅返回 IP，否则返回 IP:PORT
	 * @return {const char*} 远程连接地址，若返回值 == '\0' 则表示
	 *  无法获得远程连接地址
	 */
	const char* get_peer(bool full = false) const;

	/**
	 * 获得连接的本地地址
	 * @param full {bool} 是否获得完整地址，即：IP:PORT，如果该参数
	 *  为 false，则仅返回 IP，否则返回 IP:PORT
	 * @return {const char*} 该连接的本地地址，若返回值 == "" 则表示
	 *  无法获得本地地址
	 */
	const char* get_local(bool full = false) const;

	/**
	 * 获得异步流事件句柄
	 * @return {aio_handle&}
	 */
	aio_handle& get_handle(void) const;

	/**
	 * 注册读写流对象，内部自动调用 hook->open 过程，如果成功，则返回之前注册
     * 的对象(可能为NULL)，若失败则返回与输入参数相同的指针，应用可以通过判断
     * 返回值与输入值是否相同来判断注册流对象是否成功
	 * xxx: 在调用此方法前必须保证流连接已经创建
	 * @param hook {stream_hook*} 非空对象指针
	 * @return {stream_hook*} 返回值与输入值不同则表示成功
	 */
	stream_hook* setup_hook(stream_hook* hook);

	/**
	 * 获得当前注册的流读写对象
	 * @return {stream_hook*}
	 */
	stream_hook* get_hook(void) const;

	/**
	 * 删除当前注册的流读写对象并返回该对象，恢复缺省的读写过程
	 * @return {stream_hook*}
	 */
	stream_hook* remove_hook(void);

protected:
	aio_handle*  handle_;
	ACL_ASTREAM* stream_;
	stream_hook* hook_;

	virtual ~aio_stream(void);

	/**
	 * 通过此函数来动态释放只能在堆上分配的异步流类对象
	 */
	virtual void destroy(void);

	/**
	 * 子类应在创建成功后调用该函数通知基类增加异步流句柄数,
	 * 同时 hook 流关闭及流超时时的回调过程
	 */
	void hook_error(void);

protected:
	enum {
		// 是否调用了 hook_xxx 函数对应的标志位
		STATUS_HOOKED_ERROR = 1,
		STATUS_HOOKED_READ  = 1 << 1,
		STATUS_HOOKED_WRITE = 1 << 2,
		STATUS_HOOKED_OPEN  = 1 << 3,

		// 对于 aio_socket_stream 流表示是否连接已建立
		STATUS_CONN_OPENED  = 1 << 4,
	};
	unsigned status_;
private:
	std::list<AIO_CALLBACK*>* close_callbacks_;
	std::list<AIO_CALLBACK*>* timeout_callbacks_;

	static int close_callback(ACL_ASTREAM*, void*);
	static int timeout_callback(ACL_ASTREAM*, void*);

private:
	std::string ip_peer_;
    std::string ip_local_;

	const char* get_ip(const char* addr, std::string& out);

private:
#if defined(_WIN32) || defined(_WIN64)
	static int read_hook(SOCKET fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(SOCKET fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(HANDLE fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(HANDLE fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#else
	static int read_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int send_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);

	static int fread_hook(int fd, void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
	static int fsend_hook(int fd, const void *buf, size_t len,
		int timeout, ACL_VSTREAM* stream, void *ctx);
#endif
};

}  // namespace acl
