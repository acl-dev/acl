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

class master_service : public acl::master_udp
{
public:
	master_service();
	~master_service();

protected:
	/**
	 * 纯虚函数：当接收到一个客户端发来的数据时调用此函数
	 * @param stream {aio_socket_stream*} 本地 UDP 网络流
	 */
	virtual void on_read(acl::socket_stream* stream);

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
