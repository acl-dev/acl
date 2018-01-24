/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Wed 24 Jan 2018 12:18:34 PM CST
 */

#pragma once

class tools
{
public:
	tools(void) {}
	~tools(void) {}

	static bool get_line(ACL_VSTREAM* fp, acl::string& out);
	static bool get_version(const char* path, acl::string& out);
	static int get_fds(pid_t pid);
};
