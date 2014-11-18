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
	 * 单例初始化函数
	 * @param handle {acl::aio_handle*} 异步引擎句柄，当该值为空时，
	 *  内部会自动生成一个
	 * @param max_threads {int} 子线程池的最大线程数量
	 * @param type {acl::aio_handle_type} 当需要内部自动创建异步引擎时，
	 *  此值规定了内部所创建的异步引擎的类型，当 handle 为空时该参数
	 *  没有意义
	 */
	void init(acl::aio_handle* handle, int max_threads = 10,
		acl::aio_handle_type type = acl::ENGINE_SELECT,
		const char* addr = NULL);

	/**
	 * 发起一个阻塞过程，将该过程交由子线程处理
	 * @param req {acl::rpc_request*} 阻塞任务对象
	 */
	void fork(acl::rpc_request* req);

private:
	// 异步消息句柄
	acl::aio_handle* handle_;
	bool internal_handle_;
	// 异步 RPC 通信服务句柄
	acl::rpc_service* service_;
};
