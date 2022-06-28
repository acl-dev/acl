#include "stdafx.h"
#include "master_service.h"

//////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_str;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "str", "test_msg", &var_cfg_str },

	{ 0, 0, 0 }
};

int  var_cfg_bool;
acl::master_bool_tbl var_conf_bool_tab[] = {
	{ "bool", 1, &var_cfg_bool },

	{ 0, 0, 0 }
};

int  var_cfg_int;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "int", 120, &var_cfg_int, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

long long int  var_cfg_int64;
acl::master_int64_tbl var_conf_int64_tab[] = {
	{ "int64", 120, &var_cfg_int64, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
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
		if (strncmp(data, "quit", 4) == 0) {
			// 可以显式调用异步流的关闭过程，也可以直接返回 false
			// 通知异步框架自动关闭该异步流
			// client_->close();
			return false;
		}
		client_->write(data, len);
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
	logger("connect from %s, fd %d", client->get_peer(true),
		client->sock_handle());

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

	// 从异步流读一行数据，当读到完整一行数据时回调 acl::aio_callback
	// 中的 read_callback 虚函数
	client->gets(10, false);  

	// 从异步流中读取不定长数据，当读到数据后回调 acl::aio_callback
	// 中的 read_callback 虚函数
	// client->read();  

	// 监控异步流的读状态，当有数据可读时，回调 acl::aio_callback
	// 中的 read_wakeup 虚函数
	// client->read_wait();
	return true;
}

void master_service::proc_on_listen(acl::server_socket& ss)
{
	logger(">>>listen %s ok<<<", ss.get_addr());
}

void master_service::proc_on_init(void)
{
	logger(">>>proc_on_init<<<");
}

void master_service::proc_on_exit(void)
{
	logger(">>>proc_on_exit<<<");
}

bool master_service::proc_on_sighup(acl::string&)
{
	logger(">>>proc_on_sighup<<<");
	return true;
}
