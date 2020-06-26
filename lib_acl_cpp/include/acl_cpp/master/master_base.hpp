#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"
#include "master_conf.hpp"
#include <vector>

#ifndef ACL_CLIENT_ONLY

struct ACL_EVENT;

namespace acl
{

class server_socket;
class event_timer;
class string;

class ACL_CPP_API master_base : public noncopyable
{
public:
	/**
	 * 设置 bool 类型的配置项
	 * @param table {master_bool_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_bool(master_bool_tbl* table);

	/**
	 * 设置 int 类型的配置项
	 * @param table {master_int_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_int(master_int_tbl* table);

	/**
	 * 设置 int64 类型的配置项
	 * @param table {master_int64_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_int64(master_int64_tbl* table);

	/**
	 * 设置 字符串 类型的配置项
	 * @param table {master_str_tbl*}
	 * @return {master_base&}
	 */
	master_base& set_cfg_str(master_str_tbl* table);

	/**
	 * 判断是否是由 acl_master 控制的 daemon 模式
	 * @return {bool}
	 */
	bool daemon_mode(void) const;

	/////////////////////////////////////////////////////////////////////
	
	/**
	 * 设置进程级别的定时器，该函数只可在主线程的运行空间 (如在函数
	 * proc_on_init) 中被设置，当该定时器任务都执行完毕后会自动被
	 * 销毁(即内部会自动调用 master_timer::destroy 方法)
	 * @param timer {event_timer*} 定时任务
	 * @return {bool} 设置定时器是否成功
	 */
	bool proc_set_timer(event_timer* timer);

	/**
	 * 删除进程级别定时器
	 * @param timer {event_timer*} 由 proc_set_timer 设置的定时任务
	 */
	void proc_del_timer(event_timer* timer);

protected:
	bool daemon_mode_;
	bool proc_inited_;
	std::vector<server_socket*> servers_;

	master_base();
	virtual ~master_base();

	/**
	 * 在进程启动时，服务进程每成功监听一个本地地址，便调用本函数
	 * @param ss {const server_socket&} 监听对象
	 */
	virtual void proc_on_listen(server_socket& ss) { (void) ss; }

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

	/**
	 * 当收到 SIGHUP 信号时的回调虚方法
	 */
	virtual bool proc_on_sighup(string&) { return true; }

	// 配置对象
	master_conf conf_;

protected:
	// 子类必须调用本方法设置事件引擎句柄
	void set_event(ACL_EVENT* event);

	/**
	 * 获得事件引擎够本
	 * @return {ACL_EVENT*}
	 */
	ACL_EVENT* get_event(void) const
	{
		return event_;
	}

private:
	ACL_EVENT* event_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY

