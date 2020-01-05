#pragma once

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

extern char *var_cfg_str;
extern acl::master_str_tbl var_conf_str_tab[];

extern int  var_cfg_bool;
extern acl::master_bool_tbl var_conf_bool_tab[];

extern int  var_cfg_int;
extern acl::master_int_tbl var_conf_int_tab[];

extern long long int  var_cfg_int64;
extern acl::master_int64_tbl var_conf_int64_tab[];

////////////////////////////////////////////////////////////////////////////////

//class acl::socket_stream;

class master_service : public acl::master_threads
{
public:
	master_service();
	~master_service();

	const char* get_addr(const char* from) const;

protected:
	/**
	 * 纯虚函数：当某个客户端连接有数据可读或关闭或出错时调用此函数
	 * @param stream {socket_stream*}
	 * @return {bool} 返回 false 则表示当函数返回后需要关闭连接，
	 *  否则表示需要保持长连接，如果该流出错，则应用应该返回 false
	 */
	virtual bool thread_on_read(acl::socket_stream* stream);

	/**
	 * 当线程池中的某个线程获得一个连接时的回调函数，
	 * 子类可以做一些初始化工作
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 */
	virtual bool thread_on_accept(acl::socket_stream* stream);

	/**
	 * 当某个网络连接的 IO 读写超时时的回调函数，如果该函数返回 true 则表示继续等待下一次
	 * 读写，否则则希望关闭该连接
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 */
	virtual bool thread_on_timeout(acl::socket_stream* stream);

	/**
	 * 当与某个线程绑定的连接关闭时的回调函数
	 * @param stream {socket_stream*}
	 */
	virtual void thread_on_close(acl::socket_stream* stream);

	/**
	 * 当线程池中一个新线程被创建时的回调函数
	 */
	virtual void thread_on_init();

	/**
	 * 当线程池中一个线程退出时的回调函数
	 */
	virtual void thread_on_exit();

	/**
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	virtual void proc_on_init();

	/**
	 * 当进程退出前调用的回调函数
	 */
	virtual void proc_on_exit();

private:
	acl::sslbase_conf* server_ssl_conf_;
	acl::sslbase_conf* client_ssl_conf_;
	std::map<acl::string, acl::string> addrs_map_;
	acl::ofstream out_;

	acl::sslbase_io* setup_ssl(acl::socket_stream& conn,
			acl::sslbase_conf& conf);

	void create_addrs_map();
};
