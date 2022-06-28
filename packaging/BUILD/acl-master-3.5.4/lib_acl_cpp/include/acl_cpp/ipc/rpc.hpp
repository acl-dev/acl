#pragma once
#include "../acl_cpp_define.hpp"
#include "ipc_service.hpp"
#if defined(_WIN32) || defined(_WIN64)
 struct acl_pthread_mutex_t;
 struct acl_pthread_cond_t;
#else
# include <pthread.h>
# ifndef	acl_pthread_mutex_t
#  define	acl_pthread_mutex_t	pthread_mutex_t
# endif
# ifndef	acl_pthread_cond_t
#  define	acl_pthread_cond_t	pthread_cond_t
# endif
#endif

namespace acl {

class ipc_client;
class rpc_client;
class rpc_service;

class rpc_request;
struct RPC_DAT
{
	rpc_request* req;
	void* ctx;
};

class ACL_CPP_API rpc_request : public ipc_request
{
public:
	rpc_request(void);
	virtual ~rpc_request(void);

protected:
	friend class rpc_client;
	friend class rpc_service;

	/**
	 * 在主线程中被调用，子类必须实现此接口，
	 * 当子线程处理完请求任务后该接口将被调用，所以该类对象只能
	 * 是当本接口调用后才能被释放，禁止在调用本接口前释放本类对象
	 */
	virtual void rpc_onover(void) = 0;

	/**
	 * 虚接口：当子线程调用本对象的 rpc_signal 时，在主线程中会
	 * 调用本接口，通知在任务未完成前(即调用 rpc_onover 前)收到
	 * 子线程运行的中间状态信息；内部自动支持套接口或 _WIN32 窗口
	 * 消息；应用场景，例如，对于 HTTP 下载应用，在子线程中可以
	 * 一边下载，一边向主线程发送(调用 rpc_signal 方法)下载进程，
	 * 则主线程会调用本类实例的此方法来处理此消息
	 */
	virtual void rpc_wakeup(void* ctx) { (void) ctx; }

protected:
	/**
	 * 在子线程中被调用，子类必须实现此接口，用于处理具体任务
	 */
	virtual void rpc_run(void) = 0;

	/**
	 * 在子线程中被调用，内部自动支持套接口或 _WIN32 窗口消息
	 * 子类实例的 rpc_run 方法中可以多次调用此方法向主线程的
	 * 本类实例发送消息，主线程中调用本对象 rpc_wakeup 方法
	 * @param ctx {void*} 传递的参数指针，一般应该是动态地址
	 *  比较好，这样可以避免同一个参数被重复覆盖的问题
	 */
	void rpc_signal(void* ctx);

	/**
	 * 当子线程调用 rpc_signal 给主线程后，调用本方法可以等待
	 * 主线程发来下一步指令
	 * @param timeout {int} 等待超时时间(毫秒)，当该值为 0 时
	 *  则采用非阻塞等待模式，当该值为 < 0 时，则采用完全阻塞
	 *  等待模式(即一直等到主线程发送 cond_signal 通知)，当该
	 *  值 > 0 时，则等待的最大超时时间为 timeout 毫秒
	 * @return {bool} 返回 true 表示收到主线程发来的通知信号，
	 *  否则，需要调用 cond_wait_timeout 判断是否是超时引起的
	 */
	bool cond_wait(int timeout = -1);

	/**
	 * 当 cond_wait 返回 false 时，应用应该调用本方法判断是否
	 * 是因为等待超时引起的
	 * @return {bool} 是否是等待超时
	 */
	bool cond_wait_timeout() const
	{
		return wait_timedout_;
	}

	/**
	 * 当子线程调用 cond_wait 时，在主线程中调用本方法通知子线程
	 * “醒来”
	 * @return {bool} 当有子线程调用 cond_wait 时本函数通知子线程
	 *  “醒来”并且返回 true，否则返回 false
	 */
	bool cond_signal(void);

private:
	RPC_DAT dat_;
	ipc_client* ipc_;
	int   cond_count_;
	acl_pthread_mutex_t* lock_;
	acl_pthread_cond_t* cond_;
	bool wait_timedout_;

	// 基类 ipc_request 虚函数，在子线程中被调用
	virtual void run(ipc_client* ipc);
#if defined(_WIN32) || defined(_WIN64)
	/**
	 * 虚接口，子类实现此类用于处理具体的任务，该接口适用
	 * 于采用 _WIN32 消息的模式
	 * @param hWnd {HWND} WIN2 窗口句柄
	 */
	virtual void run(HWND hWnd);
#endif
};

//////////////////////////////////////////////////////////////////////////

class aio_socket_stream;

class ACL_CPP_API rpc_service : public ipc_service
{
public:
	/**
	 * 构造函数
	 * @param nthread {int} 如果该值 > 1 则内部自动采用线程池，否则
	 *  则是一个请求一个线程
	 * @param ipc_keep {bool} 内部 IPC 消息流是否保持长连接，保持长
	 *  连接有助于提高消息传递的效率
	 */
	rpc_service(int nthread, bool ipc_keep = true);

	~rpc_service(void) {}

	/**
	 * 主线程中运行：将请求任务放入子线程池的任务请求队列中，当线程池
	 * 中的一个子线程接收到该任务后便调用 rpc_request::rpc_run 方法调
	 * 用子类的方法，当任务处理完毕后给主线程发消息，在主线程中再调用
	 * rpc_request::rpc_callback
	 * @param req {rpc_request*} rpc_request 子类实例，非空
	 */
	void rpc_fork(rpc_request* req);

private:
	// 基类虚函数：主线程对象接收到子线程消息的
	// ipc 连接请求时的回调函数
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
};

} // namespace acl
