#pragma once

//////////////////////////////////////////////////////////////////////////////
// 配置内容项

extern char *var_cfg_str;
extern acl::master_str_tbl var_conf_str_tab[];

extern int  var_cfg_bool;
extern acl::master_bool_tbl var_conf_bool_tab[];

extern int  var_cfg_int;
extern acl::master_int_tbl var_conf_int_tab[];

extern long long int  var_cfg_int64;
extern acl::master_int64_tbl var_conf_int64_tab[];

//////////////////////////////////////////////////////////////////////////////

//class acl::aio_socket_stream;

class master_service : public acl::master_aio
{
public:
	master_service(void);
	~master_service(void);

protected:
	/**
	 * @override
	 * 虚函数：当接收到一个客户端连接时调用此函数
	 * @param stream {aio_socket_stream*} 新接收到的客户端异步流对象
	 * @return {bool} 该函数如果返回 false 则通知服务器框架不再接收
	 *  远程客户端连接，否则继续接收客户端连接
	 */
	bool on_accept(acl::aio_socket_stream* stream);

	/**
	 * @override
	 * 在进程启动时，服务进程每成功监听一个本地地址，便调用本函数
	 * @param ss {acl::server_socket&} 监听对象
	 */
	void proc_on_listen(acl::server_socket& ss);

	/**
	 * @override
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	void proc_on_init(void);

	/**
	 * @override
	 * 当进程退出前调用的回调函数
	 */
	void proc_on_exit(void);

	/**
	 * @override
	 * 当进程收到 SIGHUP 信号后的回调函数
	 */
	bool proc_on_sighup(acl::string&);
};
