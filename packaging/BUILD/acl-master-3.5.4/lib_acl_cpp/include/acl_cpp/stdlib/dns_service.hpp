#pragma once
#include "../acl_cpp_define.hpp"
#include <list>
#include "../ipc/ipc_service.hpp"
#include "../stream/aio_handle.hpp"
#include "../stream/aio_delay_free.hpp"
#include "string.hpp"

namespace acl
{

class ACL_CPP_API dns_res
{
public:
	dns_res(const char* domain) : domain_(domain) {}
	~dns_res() { ips_.clear(); }

	string domain_;
	std::list<string> ips_;
protected:
private:
};

class ACL_CPP_API dns_result_callback
{
public:
	dns_result_callback(const char* domain) : domain_(domain) {}

	/**
	 * 当任务处理完毕或出错时，内部处理过程会自动调用 destroy 接口，
	 * 子类可以在该接口内进行一些释放过程，尤其当该对象是动态创建时，
	 * 子类应该在该函数内 delete this 以删除自己，因为该函数最终肯定
	 * 会被调用，所以子类不应在其它地方进行析构操作
	 */
	virtual void destroy(void) {}

	/**
	 * 子类实现此接口，以获得查询结果，如果 res.ips_.size() == 0
	 * 则说明查询结果为空
	 * @param domain {const char*} 用户输入的查询的域名
	 * @param res {const dns_res&} 查询结果集
	 *  注：在该回调中不得删除 dns_service 对象，否则将会造成
	 *      内存非法访问，因为该回调是在 dns_service 中被调用的，
	 *      在该函数返回后 dns_service 对象还会继续使用
	 */
	virtual void on_result(const char* domain, const dns_res& res) = 0;

	/**
	 * 获得在构造函数中设置的域名值
	 */
	const string& get_domain() const { return (domain_); }
protected:
	virtual ~dns_result_callback() {}
private:
	string domain_;
};

class ipc_client;

class ACL_CPP_API dns_service
	: public ipc_service
	, public aio_delay_free
{
public:
	/**
	 * 构造函数
	 * @param nthread {int} 如果该值 > 1 则内部自动采用线程池，否则
	 *  则是一个请求一个线程
	 * @param win32_gui {bool} 是否是窗口类的消息，如果是，则内部的
	 *  通讯模式自动设置为基于 _WIN32 的消息，否则依然采用通用的套接
	 *  口通讯方式
	 */
	dns_service(int nthread = 1, bool win32_gui = false);
	~dns_service();

	/**
	 * 开始域名解析过程
	 * @param callback {dns_result_callback*} 当解析完毕后回调此类的
	 *  的回调函数 on_result
	 */
	void lookup(dns_result_callback* callback);

	/**
	 * 当查询线程完成域名解析后会通知主线程的查询对象，该查询对象会
	 * 调用本回调函数通知主类查询结果
	 * @param res {const dns_res&} 查询结果集
	 */
	void on_result(const dns_res& res);
protected:
	/**
	 * 基类虚函数，当有新连接到达时基类回调此函数
	 * @param client {aio_socket_stream*} 接收到的新的客户端连接
	 */
	virtual void on_accept(aio_socket_stream* client);

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
private:
	std::list<dns_result_callback*> callbacks_;
};

}
