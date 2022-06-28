#include "stdafx.h"
#include "rpc_manager.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_str;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "str", "test_msg", &var_cfg_str },

	{ 0, 0, 0 }
};

int   var_cfg_bool;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "bool", 1, &var_cfg_bool },

	{ 0, 0, 0 }
};

int   var_cfg_thread_pool_limit;
int   var_cfg_read_timeout;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "thread_pool_limit", 100, &var_cfg_thread_pool_limit, 0, 0 },
	{ "read_timeout", 120, &var_cfg_read_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int  var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

//////////////////////////////////////////////////////////////////////////////

class request_rpc : public acl::rpc_request
{
public:
	request_rpc(acl::aio_socket_stream* client)
	: io_error_(false)
	, client_(client)
	{
	}

	~request_rpc(void) {}

protected:
	// @override
	// 基类虚函数，由子线程处理
	void rpc_run(void)
	{
		// 将非阻塞流转为阻塞流

		// 打开阻塞流对象
		acl::socket_stream stream;

		// 必须用 get_vstream() 获得的 ACL_VSTREAM 流对象做参数
		// 来打开 stream 对象，因为在 acl_cpp 和 acl 中的阻塞流
		// 和非阻塞流最终都是基于 ACL_VSTREAM，而 ACL_VSTREAM 流
		// 内部维护着了一个读/写缓冲区，所以在长连接的数据处理中，
		// 必须每次将 ACL_VSTREAM 做为内部流的缓冲流来对待
		ACL_VSTREAM* vstream = client_->get_vstream();
		ACL_VSTREAM_SET_RWTIMO(vstream, 10);

		// 设置为阻塞模式以提高同步写的效率
		acl_non_blocking(ACL_VSTREAM_SOCK(vstream), ACL_BLOCKING);

		// 打开同步流读写对象
		(void) stream.open(vstream);

		// 开始处理该请求
		handle_conn(stream);

		// 将 ACL_VSTREAM 与阻塞流对象解绑定，这样才能保证当释放阻塞
		// 流对象时不会关闭与请求者的连接，因为该连接本身是属于非阻塞
		// 流对象的，需要采用异步流关闭方式进行关闭
		stream.unbind();
	}

	// @override
	// 基类虚函数，由主线程处理，收到子线程任务完成的消息
	void rpc_onover(void)
	{
		// 关闭异步流
		client_->close();

		// 自销毁
		delete this;
	}

	/**
	 * @override
	 * 基类虚接口：当子线程调用本对象的 rpc_signal 时，在主线程中会
	 * 调用本接口，通知在任务未完成前(即调用 rpc_onover 前)收到
	 * 子线程运行的中间状态信息；内部自动支持套接口或 WIN32 窗口
	 * 消息；应用场景，例如，对于 HTTP 下载应用，在子线程中可以
	 * 一边下载，一边向主线程发送(调用 rpc_signal 方法)下载进程，
	 * 则主线程会调用本类实例的此方法来处理此消息
	 */
	void rpc_wakeup(void*)
	{
	}

private:
	bool io_error_;
	acl::aio_socket_stream* client_;

	void handle_conn(acl::socket_stream& stream)
	{
		acl::string buf;

		// 将异步流转为同步流后，以阻塞方式对该连接池进行读写

		if (!stream.gets(buf, false)) {
			logger_warn("gets error!");
			io_error_ = true;
		} else if (stream.write(buf) == -1) {
			logger_warn("write error!");
			io_error_ = true;
		}
	}
};

//////////////////////////////////////////////////////////////////////////////

// acl::aio_callback 虚类的子类定义
class io_callback : public acl::aio_callback
{
public:
	io_callback(acl::aio_socket_stream* client) : client_(client) {}

	~io_callback(void) {}

protected:
	/** 
	 * @override
	 * 实现父类中的虚函数，客户端流的读成功回调过程 
	 * @param data {char*} 读到的数据地址 
	 * @param len {int} 读到的数据长度 
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流 
	 */  
	bool read_callback(char* data, int len)  
	{
		if (strncmp(data, "quit", len) == 0) {
			// 可以显式调用异步流的关闭过程，也可以直接返回 false
			// 通知异步框架自动关闭该异步流
			// client_->close();
			return false;
		}
		return true;
	}

	// @override
	bool read_wakeup(void)
	{
		// 为防止当子线程正处理客户端连接对象过程中连接被关闭，
		// 所以必须从异步监听集合中去掉对该异步流的监控
		// 该连接会在 request_rpc::rpc_onover 中被关闭
		client_->disable_read();

		// 创建同步流处理过程
		request_rpc* req = new request_rpc(client_);

		// 将该流的数据处理过程通过 rpc 通道交给子线程处理
		rpc_manager::get_instance().fork(req);

		return true;
	}

	/** 
	 * @override
	 * 实现父类中的虚函数，客户端流的写成功回调过程 
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流 
	 */  
	bool write_callback(void)
	{
		return true;  
	}

	/** 
	 * @override
	 * 实现父类中的虚函数，客户端流的关闭回调过程 
	 */  
	void close_callback(void)
	{
		// 必须在此处删除该动态分配的回调类对象以防止内存泄露  
		delete this;  
	}

	/** 
	 * @override
	 * 实现父类中的虚函数，客户端流的超时回调过程 
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流 
	 */  
	bool timeout_callback(void)
	{
		// 返回 false 通知异步框架关闭该异步流
		return false;
	}

private:
	acl::aio_socket_stream* client_;
};

//////////////////////////////////////////////////////////////////////////////

master_service::master_service(void)
{
}

master_service::~master_service(void)
{
}

bool master_service::on_accept(acl::aio_socket_stream* client)
{
	// 创建异步客户端流的回调对象并与该异步流进行绑定
	io_callback* callback = new io_callback(client);

	// 注册异步流的读回调过程
	client->add_read_callback(callback);

	// 注册异步流的写回调过程
	client->add_write_callback(callback);

	// 注册异步流的关闭回调过程
	client->add_close_callback(callback);

	// 注册异步流的超时回调过程
	client->add_timeout_callback(callback);

	// 写欢迎信息
	// client->format("hello, you're welcome\r\n");

	// 监控异步流的读状态，当有数据可读时，回调 acl::aio_callback
	// 中的 read_wakeup 虚函数
	client->read_wait(var_cfg_read_timeout);
	return true;
}

void master_service::proc_on_init(void)
{
	// 获得异步框架的事件引擎句柄
	acl::aio_handle* handle = this->get_handle();
	if (handle == NULL) {
		logger_fatal("aio handle null!");
	}

	// 初始化 rpc 服务对象
	rpc_manager::get_instance().init(*handle, var_cfg_thread_pool_limit);
}

void master_service::proc_on_exit(void)
{
}
