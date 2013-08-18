#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/master/master_conf.hpp"

namespace acl
{

class ACL_CPP_API master_base
{
public:
	/**
	 * 设置 bool 类型的配置项
	 * @param table {master_bool_tbl*}
	 */
	void set_cfg_bool(master_bool_tbl* table);

	/**
	 * 设置 int 类型的配置项
	 * @param table {master_int_tbl*}
	 */
	void set_cfg_int(master_int_tbl* table);

	/**
	 * 设置 int64 类型的配置项
	 * @param table {master_int64_tbl*}
	 */
	void set_cfg_int64(master_int64_tbl* table);

	/**
	 * 设置 字符串 类型的配置项
	 * @param table {master_str_tbl*}
	 */
	void set_cfg_str(master_str_tbl* table);

	/**
	 * 判断是否是由 acl_master 控制的 daemon 模式
	 * @return {bool}
	 */
	bool daemon_mode(void) const;
protected:
	bool daemon_mode_;
	bool proc_inited_;

	master_base();
	virtual ~master_base();

	/**
	 * 当进程切换用户身份前调用的回调函数，可以在此函数中做一些
	 * 用户身份为 root 的权限操作
	 */
	virtual void proc_pre_jail() {}

	/**
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	virtual void proc_on_init() {}

	/**
	 * 当进程退出前调用的回调函数
	 */
	virtual void proc_on_exit() {}

	// 配置对象
	master_conf conf_;
};

}  // namespace acl
