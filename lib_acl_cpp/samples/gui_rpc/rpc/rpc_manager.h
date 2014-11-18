#pragma once

/**
 * 本类对象将阻塞任务交给子线程处理；为了方便使用，将该类声明为单例类
 */
class rpc_manager : public acl::singleton<rpc_manager>
{
public:
	/**
	 * 构造函数
	 * @param max_threads {int} 子线程池的最大线程数量
	 */
	rpc_manager(int max_threads = 10);
	~rpc_manager();

	/**
	 * 发起一个阻塞过程，将该过程交由子线程处理
	 * @param req {acl::rpc_request*} 阻塞任务对象
	 */
	void fork(acl::rpc_request* req);
protected:
private:
	// 异步消息句柄
	acl::aio_handle* handle_;
	// 异步 RPC 通信服务句柄
	acl::rpc_service* service_;
};
