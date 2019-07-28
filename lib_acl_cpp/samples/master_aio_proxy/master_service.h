#pragma once

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

extern acl::master_str_tbl var_conf_str_tab[];

extern acl::master_bool_tbl var_conf_bool_tab[];

extern acl::master_int_tbl var_conf_int_tab[];

////////////////////////////////////////////////////////////////////////////////

//class acl::aio_socket_stream;

class master_service : public acl::master_aio
{
public:
	master_service();
	~master_service();

protected:
	/**
	 * 纯虚函数：当接收到一个客户端连接时调用此函数
	 * @param stream {aio_socket_stream*} 新接收到的客户端异步流对象
	 * @return {bool} 该函数如果返回 false 则通知服务器框架不再接收
	 *  远程客户端连接，否则继续接收客户端连接
	 */
	bool on_accept(acl::aio_socket_stream* stream);

	/**
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	virtual void proc_on_init();

	/**
	 * 当进程退出前调用的回调函数
	 */
	virtual void proc_on_exit();
};
