/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Thu 04 Jan 2018 11:19:42 AM CST
 */

#pragma once

class guard_action : public acl::thread_job
{
public:
	guard_action(const char* ip, const char* data);

	// @override
	void* run(void);

private:
	~guard_action(void);

private:
	acl::string ip_;
	acl::string data_;

	bool do_run(void);

	bool on_service_list(acl::json& json);
	bool on_service_dead(acl::json& json);
};
