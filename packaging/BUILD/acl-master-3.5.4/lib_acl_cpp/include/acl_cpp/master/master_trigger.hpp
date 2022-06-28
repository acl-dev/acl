#pragma once
#include "master_base.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTRING;

namespace acl {

/**
 * acl_master 服务器框架中触发器的模板类，该类对象只能有一个实例运行
 */
class ACL_CPP_API master_trigger : public master_base
{
public:
	/**
	 * 开始运行，调用该函数是指该服务进程是在 acl_master 服务框架
	 * 控制之下运行，一般用于生产机状态
	 * @param argc {int} 从 main 中传递的第一个参数，表示参数个数
	 * @param argv {char**} 从 main 中传递的第二个参数
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * 在单独运行时的处理函数，用户可以调用此函数进行一些必要的调试工作
	 * @param path {const char*} 配置文件全路径
	 * @param count {int} 当该值 > 0 时，则接收的连接次数达到此值且完成
	 *  后，该函数将返回，否则一直循环接收远程连接
	 * @param interval {int} 触发器时间间隔(秒)
	 */
	void run_alone(const char* path = NULL, int count = 1, int interval = 1);

	/**
	 * 获得配置文件路径
	 * @return {const char*} 返回值为 NULL 表示没有设配置文件
	 */
	const char* get_conf_path(void) const;

protected:
	master_trigger();
	virtual ~master_trigger();

	/**
	 * 当触发器时间到时调用此函数
	 */
	virtual void on_trigger() = 0;

private:
	// 当触发器时间到时由 acl_master 框架回调此函数
	static void service_main(void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_pre_jail(void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_init(void*);

	// 当进程退出时调用的回调函数
	static void service_exit(void*);

	// 当进程收到 SIGHUP 信号后会回调本函数
	static int service_on_sighup(void*, ACL_VSTRING*);
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
