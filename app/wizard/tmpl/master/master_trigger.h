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

class master_service : public acl::master_trigger
{
public:
	master_service(void);
	~master_service(void);

protected:
	/**
	 * @override
	 * 当触发器时间到时调用此函数
	 */
	void on_trigger(void);

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
