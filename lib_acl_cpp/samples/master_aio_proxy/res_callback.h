#pragma once

class req_callback;

class res_callback : public acl::aio_open_callback
{
public:
	res_callback(req_callback* req, acl::ofstream* res_fp);
	~res_callback();

	bool start(acl::aio_handle& handle, const char* server_addr);

	acl::aio_socket_stream& get_conn();
	void disconnect();
	void on_closed();

protected:
	/** 
	 * 实现父类中的虚函数，客户端流的读成功回调过程 
	 * @param data {char*} 读到的数据地址 
	 * @param len {int} 读到的数据长度 
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流 
	 */
	virtual bool read_callback(char* data, int len);

	/** 
	 * 实现父类中的虚函数，客户端流的写成功回调过程 
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流 
	 */  
	virtual bool write_callback()
	{
		return true;  
	}

	/** 
	 * 实现父类中的虚函数，客户端流的关闭回调过程 
	 */  
	virtual void close_callback();

	/** 
	 * 实现父类中的虚函数，客户端流的超时回调过程 
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流 
	 */  
	virtual bool timeout_callback()
	{
		// 返回 false 通知异步框架关闭该异步流
		return false;
	}

	/**
	 * 基类虚函数, 当异步连接成功后调用此函数
	 * @return {bool} 返回给调用者 true 表示继续，否则表示需要关闭异步流
	 */
	virtual bool open_callback();

private:
	acl::aio_socket_stream* conn_;
	req_callback* req_;
	acl::ofstream* res_fp_;
};
