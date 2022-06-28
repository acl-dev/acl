#pragma once
#include "../stream/aio_handle.hpp"
#include "../ipc/ipc_service.hpp"
#include "http_header.hpp"

namespace acl {

class string;

/**
 * HTTP 服务请求类，子类必须继承该类
 */
class ACL_CPP_API http_service_request : public http_header
{
public:
	/**
	 * 构造函数
	 * @param domain {const char*} HTTP 服务器的域名(也可以是IP)，非空
	 *  如果传入了空值，则会 fatal
	 * @param port {unsigned short} HTTP 服务端口
	 */
	http_service_request(const char* domain, unsigned short port);

	/**
	 * 获得由构造函数输入的 domain
	 * @return {const char*} 永不为空
	 */
	const char* get_domain(void) const;

	/**
	 * 获得由构造函数输入的 port
	 * @return {unsigned short}
	 */
	unsigned short get_port(void) const;

	/**
	 * 当任务处理完毕或出错时，内部处理过程会自动调用 destroy 接口，
	 * 子类可以在该接口内进行一些释放过程，尤其当该对象是动态创建时，
	 * 子类应该在该函数内 delete this 以删除自己，因为该函数最终肯定
	 * 会被调用，所以子类不应在其它地方进行析构操作
	 */
	virtual void destroy(void) {}

	//////////////////////////////////////////////////////////////////////
	// 子类必须实现如此虚接口

	/**
	 * 获得 HTTP 请求体数据，该函数会在请求过程中被循环调用，直到返回的数据
	 * 对象中的数据为空
	 * @return {const string*} 请求体结果数据，如果返回空指针或返回的缓冲区
	 *  对象的数据为空(即 string->empty()) 则表示 HTTP 请求体数据结束
	 * 注意：与其它函数不同，该虚接口是另外的子线程中被调用的，所以如果子类
	 * 实现了该接口，如果需要调用与原有线程具备竞争的资源时应该注意加锁保护
	 */
	virtual const string* get_body(void);

	/**
	 * 当获得 HTTP 服务器的 HTTP 响应头时的回调接口
	 * @param addr {const char*} 与服务器之间的连接地址，格式：IP:PORT
	 * @param hdr {const HTTP_HDR_RES*} HTTP 响应头，该结构定义参见：
	 *  acl_project/lib_protocol/include/http/lib_http_struct.h
	 */
	virtual void on_hdr(const char* addr, const HTTP_HDR_RES* hdr) = 0;

	/**
	 * 当获得 HTTP 服务器的 HTTP 响应体时的回调接口，当 HTTP 响应体数据
	 * 比较大时，该回调会被多次调用，直到出错(会调用 on_error)或数据读完
	 * 时，该回调的两个参数均 0，当 data 及 dlen 均为 0 时，表明读 HTTP
	 * 响应体结束
	 * @param data {const char*} 某次读操作时 HTTP 响应体数据
	 * @param dlen {size_t} 某次读操作时 HTTP 响应体数据长度
	 * 注：如果 HTTP 响应只有头数据而没有数据体，则也会调用该函数通知用户
	 *     HTTP 会话结束
	 */
	virtual void on_body(const char* data, size_t dlen) = 0;

	/**
	 * 在 HTTP 请求或响应过程中如果出错，则会调用此接口，通知子类出错，
	 * 在调用此接口后
	 * @param errnum {http_status_t} 出错码
	 */
	virtual void on_error(http_status_t errnum) = 0;
protected:
	virtual ~http_service_request(void);
private:
	char* domain_;
	unsigned short port_;
};

class aio_socket_stream;

class ACL_CPP_API http_service : public ipc_service
{
public:
	/**
	 * 构造函数
	 * @param nthread {int} 如果该值 > 1 则内部自动采用线程池，否则
	 *  则是一个请求一个线程
	 * @param nwait {int} 当异步引擎采用 ENGINE_WINMSG 时，为了避免
	 *  因任务线程发送的数据消息过快而阻塞了主线程的 _WIN32 消息循环，
	 *  在任务线程发送数据消息时自动休眠的毫秒数；对于其它异步引擎，
	 *  该值也可以用于限速功能
	 * @param win32_gui {bool} 是否是窗口类的消息，如果是，则内部的
	 *  通讯模式自动设置为基于 _WIN32 的消息，否则依然采用通用的套接
	 *  口通讯方式
	 */
	http_service(int nthread = 1, int nwait = 1, bool win32_gui = false);
	~http_service(void);

	/**
	 * 应用调用此函数开始 HTTP 会话过程，由 http_service 类对象负责
	 * 向服务器异步发出 HTTP 请求，同时异步读取来自于 HTTP 服务器的响应
	 * @param req {http_service_request*} HTTP 请求类对象
	 */
	void do_request(http_service_request* req);
protected:
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 基类虚函数，当收到来自于子线程的 win32 消息时的回调函数
	 * @param hWnd {HWND} 窗口句柄
	 * @param msg {UINT} 用户自定义消息号
	 * @param wParam {WPARAM} 参数
	 * @param lParam {LPARAM} 参数
	 */
	virtual void win32_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif
	/**
	 * 基类虚函数，当有新连接到达时基类回调此函数
	 * @param client {aio_socket_stream*} 接收到的新的客户端连接
	 */
	virtual void on_accept(aio_socket_stream* client);

	/**
	 * 基类虚函数，当监听流成功打开后的回调函数
	 * @param addr {const char*} 实际的监听地址，格式：IP:PORT
	 */
	virtual void on_open(const char*addr);

	/**
	 * 基类虚函数，当监听流关闭时的回调函数
	 */
	virtual void on_close(void);
private:
	char* addr_;
	int   nwait_;
	aio_handle_type handle_type_;
};

}  // namespace acl
