#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include "ipc_server.hpp"

struct acl_pthread_pool_t;

namespace acl {

class ipc_client;

class ACL_CPP_API ipc_request : public noncopyable
{
public:
	ipc_request();
	virtual ~ipc_request();

	/**
	 * 虚接口，子类实现此类用于处理具体的任务，该接口适用
	 * 于采用 IO 消息的模式
	 */
	virtual void run(ipc_client* ipc);

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 虚接口，子类实现此类用于处理具体的任务，该接口适用
	 * 于采用 _WIN32 消息的模式
	 */
	virtual void run(HWND hWnd);

	/**
	 * 设置 _WIN32 窗口句柄
	 * @param hWnd {HWND} 窗口句柄
	 */
	void set_hwnd(HWND hWnd)
	{
		hWnd_ = hWnd;
	}

	/**
	 * 获得 _WIN32 窗口句柄
	 * @return {HWND} 窗口句柄
	 */
	HWND get_hwnd(void) const
	{
		return (hWnd_);
	}
#endif
private:
#if defined(_WIN32) || defined(_WIN64)
	HWND  hWnd_;
#endif
};

class ACL_CPP_API ipc_service : public ipc_server
{
public:
	/**
	 * 构造函数
	 * @param nthread {int} 如果该值 > 1 则内部自动采用线程池，否则
	 *  则是一个请求一个线程
	 * @param ipc_keep {bool} 内部 IPC 消息流是否保持长连接，保持长
	 *  连接有助于提高消息传递的效率
	 */
	ipc_service(int nthread, bool ipc_keep = true);

	virtual ~ipc_service();

#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 当采用 _WIN32 消息模式时，子类需要实现此虚接口用于处理具体的
	 * 消息过程，子类必须实现该接口
	 * @param hWnd {HWND} 窗口句柄
	 * @param msg {UINT} 用户自定义消息号
	 * @param wParam {WPARAM} 参数
	 * @param lParam {LPARAM} 参数
	 */
	virtual void win32_proc(HWND hWnd, UINT msg,
		WPARAM wParam, LPARAM lParam);
#endif

	/**
	 * 从 ipc 消息流连接池中取得一个连接
	 * @return {ipc_client*} 返回 NULL 表示无法连接消息服务器
	 */
	ipc_client* peek_conn();

	/**
	 * 将用完的 ipc 消息连接放回连接池中
	 * @param conn {ipc_client*} ipc 消息连接流
	 */
	void push_conn(ipc_client* conn);
protected:
#if defined(_WIN32) || defined(_WIN64)
	__int64   magic_;
#else
	long long int magic_;
#endif

	/**
	 * 子类调用此函数发送请求服务
	 * @param req {ipc_request*}
	 */
	void request(ipc_request* req);
private:
	bool  ipc_keep_;
	acl_pthread_pool_t* thread_pool_;
#if defined(_WIN32) || defined(_WIN64)
	HWND hWnd_;
	HINSTANCE hInstance_;

	/**
	 * 基类虚函数：Windows 消息方式下，创建隐藏窗口句柄
	 */
	virtual bool create_window(void);

	/**
	 * 基类虚函数：Windows 消息方式下，关闭隐藏窗口句柄
	 */
	virtual void close_window(void);
#endif

	locker lock_;
	std::list<ipc_client*> conn_pool_;
};

}  // namespace acl
