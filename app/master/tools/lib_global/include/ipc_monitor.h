/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Mon 29 Jan 2018 05:14:03 PM CST
 */

class ipc_monitor : public acl::thread
{
public:
	ipc_monitor(acl::tcp_ipc& ipc, int ttl, bool& service_exit);
	~ipc_monitor(void) {}

private:
	// @override
	void* run(void);

	void check_idle(void);

private:
	acl::tcp_ipc& ipc_;
	int ttl_;
	bool& service_exit_;
};
