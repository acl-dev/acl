#pragma once

/**
 * 本类对象将阻塞任务交给子线程处理；为了方便使用，将该类声明为单例类
 */
class rpc_manager : public acl::singleton<rpc_manager>
{
public:
	rpc_manager();
	~rpc_manager();

	/**
	 * 初始化函数
	 * @param handle {acl::aio_handle*} 异步引擎句柄
	 * @param max_threads {int} 子线程池的最大线程数量
	 * @param rpc_addr {const char*} RPC 通道监听地址
	 */
	void init(acl::aio_handle*, int max_threads = 10,
		const char* rpc_addr = NULL);

	void finish();

	/**
	 * 发起一个阻塞过程，将该过程交由子线程处理
	 * @param req {acl::rpc_request*} 阻塞任务对象
	 */
	void fork(acl::rpc_request* req);
private:
	// 异步消息句柄
	acl::aio_handle* handle_;
	// 异步 RPC 通信服务句柄
	acl::rpc_service* service_;
};
