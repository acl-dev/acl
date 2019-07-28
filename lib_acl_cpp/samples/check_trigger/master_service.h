#pragma once

class master_service : public acl::master_trigger
{
public:
	master_service();
	~master_service();

protected:
	/**
	 * 当触发器时间到时调用此函数
	 */
	virtual void on_trigger();

	/**
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	virtual void proc_on_init();

	/**
	 * 当进程退出前调用的回调函数
	 */
	virtual void proc_on_exit();

protected:
	std::vector<acl::string> url_list_;
};
