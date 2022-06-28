#pragma once

class res_callback;

class req_callback : public acl::aio_callback
{
public:
	req_callback(acl::aio_socket_stream* conn,
		acl::ofstream* req_fp, acl::ofstream* res_fp);
	~req_callback();

	void start(const char* server_addr);

	void disconnect();
	void on_connected();

	acl::aio_socket_stream& get_conn();

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

private:
	acl::aio_socket_stream* conn_;
	res_callback* res_;
	acl::ofstream* req_fp_;
	acl::ofstream* res_fp_;
};
